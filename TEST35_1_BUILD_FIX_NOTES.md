# LR-608 0.99.33 — TEST35.1 CI build fix

Correzione mirata del controllo GitHub Actions emersa dal log `logs_118194386600.zip`.

## Causa

`Tests/verify_incremental_search_integration.py` richiedeva che il workflow contenesse la stringa `TEST33`, ma l’artefatto ora si chiama `LR-608-0.99.33-TEST35-Windows-VST3`. L’asserzione bloccava il workflow **prima** della compilazione Windows.

## Modifica

Rimosso il controllo fragile del numero TEST e sostituito con controlli effettivi della pipeline: checkout, verifica della ricerca incrementale, build e test, packaging ZIP e upload dell’artefatto. Aggiornati docstring e messaggio del test.

**Unico file di codice modificato:** `Tests/verify_incremental_search_integration.py`. Il DSP, i parametri, l’About, i manuali e il workflow restano identici alla TEST35.

## Verifiche

- Test Python di integrazione (comprende otto manuali, ricerca incrementale e Alt+T).
- Test DSP standalone per Output Warmth e Transient Designer.
- Resta necessaria la compilazione VST3 Windows completa su GitHub Actions.

## Comando Git

```bash
git add . && git commit -m "LR-608 TEST35.1 - Fix version-dependent CI integration check" && git push
```
