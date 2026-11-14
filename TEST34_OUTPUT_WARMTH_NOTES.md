# LR-608 0.99.33 TEST34 — Output Warmth (sorgente sperimentale)

Data di sviluppo: 14 novembre 2026. Questa e una TEST, non una release definitiva. Nessuna modifica alla versione ufficiale nell'About o al numero di versione CMake finche non viene ascoltata e verificata su REAPER.

## Scopo

Rendere i kick e gli altri strumenti gravi piu percepibilmente corposi **senza** usare una equalizzazione di rinforzo dei bassi, un incremento forzato del master o un compressore pesante.

## Algoritmo

- Un ramo parallelo per ciascuna delle 32 coppie stereo (L/R con stati distinti).
- Due poli passa-basso a 240 Hz isolano il corpo grave *per pilotare il generatore*, non per filtrare il segnale asciutto.
- Una non linearita di ordine pari con limitazione progressiva, `x^2 / (1 + 0.5*x^2)`, genera principalmente armoniche pari, soprattutto la seconda.
- Rimozione della componente continua a 15 Hz **solo** dal ramo armonico.
- Aggiunta dell'armonico dosata da `Output Warmth (%)`, con ammorbidimento dei cambi valore di circa 15 ms.
- Il percorso asciutto, gli attacchi e il livello della fondamentale non vengono equalizzati o boostati.
- Il nuovo ramo e aggiunto **dopo il calcolo** dei guadagni dello stadio legacy: il calcolo del compressore condiviso non reagisce alle armoniche nuove. L'elaborazione legacy fra le uscite rimane quella di prima; questa TEST non la riscrive.
- Nessuna nuova latenza introdotta. La saturazione agisce solo su un segnale molto limitato in banda, quindi non si usa oversampling in questa prima TEST a bassa intensita. Non e una garanzia di assenza assoluta di aliasing.

## Accessibilita

Alt+G, pagina Global: **Output Warmth (%)**, valore 0...100, passo 1, default 0. La ricerca incrementale della griglia trova il nome come gli altri parametri. Invio conferma; Esc annulla (snapshot aggiornato). Parametro salvato nell'APVTS con ID `outputWarmth`, aggiunto **dopo** i precedenti 400 parametri Slot, per non cambiare gli indici dei kit/engine esistenti.

## Prova d'ascolto consigliata

1. In REAPER suona un kick 808 o 909 isolato e imposta il Master Volume come usi normalmente.
2. Vai su Alt+G, trova `Output Warmth (%)` e alterna 0% / 25% / 35%; come prova piu evidente puoi usare 60-70%.
3. Confronta lo stesso pattern allo stesso volume percepito, senza usare un altro saturatore o compressore esterno durante il primo confronto.
4. Ripeti con kick + snare + piatti e con il kick su un'uscita stereo diversa dal piatto; controlla eventuali click, impastamenti e il punch iniziale.
5. Se il risultato e troppo colorato, resta intorno al 15-25%. Il 100% e un'estensione per test estremi, non un valore consigliato per un kit completo.

## Verifica eseguita sul codice DSP isolato

Compilato con g++ C++17, O2, warning, e ripetuto con UBSan. 48 kHz, tono sinusoidale 60 Hz, ampiezza di input 0.75, 1 secondo, misure sulla seconda meta del segnale:

| Output Warmth | Fondamentale in uscita | Seconda armonica in uscita |
| --- | ---: | ---: |
| 0% | 0.15282684 | 0.00000000 |
| 35% | 0.15282684 | 0.00777589 |
| 80% | 0.15282684 | 0.01777345 |

Risultati coerenti sulle uscite 1/2, 3/4, 31/32, 63/64. A 4 kHz l'ampiezza della fondamentale in questa prova non cambia alle 8 cifre riportate. Con kick e tono acuto su bus diversi la deviazione massima del bus acuto e 0.00000148 a 80%.

**Regression bypass**: algoritmo storico compilato direttamente dall'archivio originale 0.99.33 e sorgente TEST34 a Warmth 0 hanno generato un hash identico con lo stesso compilatore e le stesse condizioni di test (`baa46810d0a66e0c`). Questo supporta la fedelta del percorso disattivato, ma non sostituisce una prova VST3 completa.

**Non eseguiti**: build Windows VST3/JUCE tramite GitHub Actions, test di compatibilita preset nel plugin completo, ascolto reale in REAPER e confronto loudness-matched con una drum machine commerciale.

## File modificati

- `Source/OutputStage.cpp` e `Source/OutputStage.h`
- `Source/PluginProcessor.cpp`
- `Source/PluginEditor.cpp` (snapshot Global / Esc)
- `Source/GeneratedParameters.h`, `Source/GeneratedPages.h`, `Tools/generate_catalog.py`
- `Documentation/LR608Help.html` (sezioni inglese e italiano), `README.md`
- `Tests/OutputStageWarmthTest.cpp`, `Tests/OutputStageBypassRegressionProbe.cpp`

Compilazione test indipendenti da JUCE:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -ISource Tests/OutputStageWarmthTest.cpp Source/OutputStage.cpp -o output-test
./output-test
```
