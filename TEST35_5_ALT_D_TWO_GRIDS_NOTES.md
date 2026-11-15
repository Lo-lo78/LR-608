# LR-608 0.99.33 TEST35.5 — two accessible keyboard grids

Based on the complete TEST35.4 code. No DSP, presets, Roll parameter IDs, output routing, or host automation changed.

Alt+D: four vertical groups of 3 / 3 / 3 / 1 parameters, not a fixed-height parameter matrix. Plain arrows move focus between cells. Page/Home/End move within the current vertical group; Ctrl+Home/End jumps to Slot/Output. Alt+Up/Right increases a value; Alt+Down/Left decreases; Alt+Page makes a larger change and Alt+Home/End selects max/min. The reverse display ordering of Slot and Output is handled explicitly. Roll values therefore increase consistently with Alt+Up.

Enter or Alt+V enters the separate, keyboard-focusable, non-editable Alt+D Value; there is no numeric edit box for Slot/Engine/Layer/MIDI/Choke/Roll/Output. In Value, arrows/Page/Home/End modify the selected parameter and Enter/Escape returns to Alt+D. Selecting Layer > Add Layer then pressing Enter in Value allocates the layer, as before. F2 on Slot still renames the Slot.

Incremental typing searches names (not values) in both Alt+D and Alt+L, with independent search buffers, Shift reverse, digits and 1000 ms reset. Neither the global GUI nor the Audio Engine was altered.

Validation: portable C++ navigation and incremental search checks, static integration checks; full Windows VST3 compilation must still be verified by GitHub Actions.
