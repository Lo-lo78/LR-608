# LR-608 0.99.34 TEST36.3 — Ctrl+Shift+A keyboard fix

Starting point: TEST36.2 (same 0.99.34 VST3 version; this is **not** a public release).

## Root cause

JUCE `ModifierKeys::isCommandDown()` checks the Ctrl modifier on Windows. The TEST36.2 keyboard condition required both `isCtrlDown()` and `!isCommandDown()`, so **Ctrl+Shift+A could never match** on Windows. There was also a potential focus-handling difference: the Alt+D and Alt+L ComboBoxes were ordinary JUCE controls, unlike LSampler-24's keyboard-aware parameter ComboBox.

## Fix

- Recognise **Ctrl+Shift+A** by Ctrl + Shift, with Alt excluded, without rejecting Command on Windows. Check the physical A keycode or the text character.
- Forward the shortcut directly to the editor from the focus-owning Alt+D ComboBox, Alt+L ComboBox, Alt+D Value button, and Alt+L Value slider. Non-shortcut keyboard behaviour is delegated unchanged to the corresponding JUCE base control.
- Leave automation host operations, `ACTIVE`, `ARM`, `VISIBLE` handling, stored envelope points, slot parameter IDs, NVDA `, Auto` indicator, focus layout, and DSP unchanged.
- Update existing static CI checks to recognise the derived controls and to reject the Windows modifier contradiction.

## Quick REAPER/NVDA test

1. Select Alt+D -> Slot 1 -> Roll Division. Press Ctrl+Shift+A. NVDA should say `Roll Division, <value>, Auto` and REAPER should show an active envelope for Slot 1 Roll Division.
2. Press Ctrl+Shift+A again. NVDA should no longer say `Auto`; the existing envelope points must remain in the project.
3. Repeat on Alt+L for a Sound/FX parameter and in both Value controls.
4. Select Slot, Layer or Output in Alt+D and press Ctrl+Shift+A: NVDA should say `No automatable parameter selected` (no envelope is created).
5. Check normal arrows, search, Alt+frecce, Enter/Value, Esc, and other shortcuts to ensure no regression.

Local Python integration checks pass. Windows VST3 compilation and live NVDA/REAPER tests require GitHub Actions + REAPER.
