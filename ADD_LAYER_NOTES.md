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

## Keyboard navigation and Clear All — follow-up fix

- Up increments Layer 1 -> Layer 2 -> Add Layer, matching Engine and the
  neighbouring MIDI ComboBoxes; Down decrements. Add Layer still requires Enter.
- Alt+Delete removes just the selected layer (MIDI editing enabled).
- Alt+Shift+Delete removes every layer of the captured MIDI note
  (MIDI editing enabled).
- Ctrl+Alt+Shift+Delete invokes Clear All Slots for all 128 Slots,
  with the same action and accessible button description. It does not require Alt+M.
- No changes to Ctrl+X/C/V or GitHub Actions.
- These are source changes only; REAPER may intercept key combinations before
  JUCE receives them. Verify all three Delete combinations in a freshly built VST3.
