# LR-608 0.99.33 TEST35 — Global Transient Designer

**Sviluppo sperimentale, 15 novembre 2026.** La versione pubblica e l'About restano **0.99.33**. Il sorgente deriva dalla TEST34.1 riuscita con Output Warmth; non sostituisce ancora una release stabile.

## Obiettivo

Aggiungere la regolazione dell'impatto iniziale dei kick e della loro coda, conservando Output Warmth e mantenendo un percorso stereo/multicanale indipendente per ogni coppia 1/2 ... 63/64.

## Parametri — Alt+G, Global

| Parametro | Valori | Default | Effetto |
|---|---|---|---|
| Transient Attack (%) | -100...+100, passo 1 | 0 | Positivo rinforza l'attacco; negativo lo ammorbidisce |
| Transient Sustain (%) | -100...+100, passo 1 | 0 | Positivo valorizza la coda; negativo l'accorcia |
| Transient Speed (ms) | 8...120, passo 1 | 40 | Regola la velocità degli inviluppi di rilevamento |
| Transient Focus | Low / Full | Low | Low: principalmente bassi; Full: tutto lo spettro |

Attack/Sustain a **0/0** disabilitano il trattamento anche se Speed/Focus vengono cambiati. Tutti e quattro sono veri parametri VST3 globali, con automazione e persistenza nel salvataggio stato, non per singolo Slot. Con Enter confermi l'editing Global; Esc ripristina anche questi quattro controlli, insieme agli altri Global.

## Struttura DSP

- Due follower di inviluppo, uno rapido e uno lento, con differenze relative al livello e floor di rumore. I contributi Attack e Sustain sono indipendenti, bipolari e limitati in ampiezza.
- In **Low**, il rilevamento usa una banda grave passa-basso a 2 poli (circa 360 Hz) e la modifica è applicata solo al contributo filtrato in parallelo, lasciando quasi immutati i piatti. In **Full** vengono modificati anche i transienti sulle frequenze acute.
- Coppie stereo elaborate separatamente, ma rivelatore e guadagno condivisi fra L/R della *stessa* coppia, per evitare spostamenti dell'immagine stereo.
- Smoothing dei controlli e del guadagno; nessuna latenza aggiunta e nessuna allocazione nell'audio thread. Il DSP viene applicato **dopo** Output Warmth, senza modificare l'algoritmo Warmth della TEST34.1. Il precedente glue/limiter globale resta invariato e conserva il proprio comportamento originale.
- Nessuna EQ statica sulle basse; quando l'effetto è disattivato, passaggio esattamente uguale alla TEST34.1.

**Attenzione ai picchi:** Attack positivo può aumentarli. Con valore molto alto e canali già vicini a 0 dBFS il DAW può clippare; abbassare Master Volume o il livello del canale. Il Transient Designer non è un limiter.

## Valori di partenza per ascolto

- **Kick naturale:** Output Warmth 20–35%; Transient Attack +15...+25; Sustain 0...-10; Speed 35–50 ms; Focus Low.
- **Kick più secco / punch:** Warmth 25–40%; Attack +30...+50; Sustain -15...-35; Speed 15–35 ms; Focus Low.
- **Kick lungo / corpo:** Warmth 30–50%; Attack +10...+20; Sustain +15...+30; Speed 50–80 ms; Focus Low.
- **Snare / Clap:** Attack +20...+35; Sustain -15...+15; Speed 20–40 ms; Focus Full. Condivide i parametri Global con gli altri strumenti: usare routing su coppie separate se occorre una gestione individuale tramite effetti REAPER.

## Compatibilità e controlli

- I **primi 400** descrittori per Slot mantengono indici e ID. `outputWarmth` resta all'indice 400. I nuovi quattro descrittori sono appesi negli indici 401..404.
- Catalogo: **405** parametri descrittori, **12** pagine, **355** controlli esposti nell'audit.
- Test numerico standalone a 48 kHz, segnale kick a 60 Hz: Attack +60 aumenta l'RMS nei primi 35 ms da circa 0.08746 a 0.10569; Attack -60 lo riduce a 0.07291. Sustain -60 abbassa l'RMS della coda tardiva da 0.01188 a 0.00934; Sustain +60 lo porta a 0.01465. Questi sono test sintetici, **non** misure di resa musicale del VST3.
- Test di Output Warmth ancora superato. Probe del percorso in bypass, con compilatore GCC e condizioni identiche: hash **`baa46810d0a66e0c`** per TEST34.1 e TEST35, con Attack/Sustain a zero. Test `CatalogAudit` superato con stub minimo JUCE; `OutputStageTransientTest` superato su GCC Linux.
- **Non è stata eseguita qui la compilazione VST3 Windows JUCE completa:** GitHub Actions rimane la verifica per l'integrazione e il packaging; una prova d'ascolto in REAPER resta fondamentale.

## Comando Git

Dopo aver sostituito il contenuto del repository con quello dello ZIP:

```bash
git add . && git commit -m "LR-608 TEST35 - Add global stereo and multichannel transient designer" && git push
```
