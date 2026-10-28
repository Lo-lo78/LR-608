# LR-608 — Add Layer, experimental source update

In the Slot area, the new **Layer** ComboBox is immediately after Engine.
Navigate with the Up/Down arrows, then press Enter on **Add Layer** to
allocate a new layer on the same MIDI note. Newly allocated layers have Engine
**Off** and appear in the ComboBox even before a sound engine is selected.
The slot reservation is saved in `SlotStates/Slot@reservedLayer` (old presets
without that property continue loading). Maximum eight layers per MIDI note.

The first Off Slot in a completely cleared kit is treated as an implicit
Layer 1; selecting Add Layer allocates Layer 2 directly. Reset/Initialize,
Clear All, and layer deletion clear explicit reservations.

Alt shortcuts and host keyboard interception have **not** been modified.

This archive contains source code only. It has not been compiled or tested
inside REAPER in this environment; build and runtime validation are required.

Layer ordering is saved separately from physical Slot IDs so each added layer
appends to the end even if its allocated Slot number is lower.
