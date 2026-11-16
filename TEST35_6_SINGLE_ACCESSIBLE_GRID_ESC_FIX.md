# LR-608 0.99.33 TEST35.6 — Alt+D single accessible ComboBox and Escape

Base: TEST35.5. The original ten Slot/MIDI/Roll/Output ComboBoxes continue to display values in four vertical columns (3/3/3/1). They are deliberately non-focusable and not separately exposed to accessibility APIs. New Slot grid selector is ONE keyboard-focusable ComboBox just like the existing Alt+L parameterSelector. Its ten items show live parameter name and value. Navigating Alt+D updates selection, *not keyboard focus*, so NVDA should announce the ComboBox role only on entry. Value changes, incremental search, slot navigation, 128 individual Rolls, preset formats and DSP remain unchanged.

Escape from Alt+D/Alt+L (also their Value views) is returned unconsumed for REAPER to close the host FX window instead of moving to the other grid. Other popup dialogs keep their Escape-to-cancel behaviour. Actual closing depends on the host keyboard forwarding and needs testing in REAPER.

Checks: static integration script and standalone incremental search + slot geometry C++ tests. Full JUCE/Windows VST3 compilation and NVDA/REAPER testing are not available locally.
