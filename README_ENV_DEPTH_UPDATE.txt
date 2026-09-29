LR-608 FX Envelope destination update

Shared exponential FX Attack/Decay envelope now has four independent bipolar destinations:
- Env Depth Volume: -1..+1
- Env Depth Filter LP: -8..+8 octaves
- Env Depth Filter HP: -8..+8 octaves
- Env Depth Pan: -1..+1

Negative depth reverses the modulation direction while preserving the same shared Attack and Decay timing. Example: positive volume depth rises during Attack and returns during Decay; negative volume depth falls during Attack and returns during Decay.

The legacy Filter Envelope Depth (%) control is hidden but retained internally for preset compatibility. Architecture version 13 migrates old values to the new filter depths using the exact former relationship: 100% = +2 octaves LP and +1 octave HP.
