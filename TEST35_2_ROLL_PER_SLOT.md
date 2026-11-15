# LR-608 0.99.33 TEST35.2 — 128 independent slot rolls

- Each of 128 slots has its own Roll Division, Roll Shuffle and Roll Second Hit Velocity Reduction, persisted by APVTS in REAPER projects and presets.
- Alt+D is grouped in four visual columns: Slot/Engine/Layer, MIDI Note/Choke Trigger/Choke Target, three Roll controls, Output. Ten keyboard-focusable controls retain NVDA/OSARA names and left/right navigation.
- Global no longer exposes the three Roll controls. Legacy global slider IDs remain in the parameter catalog for compatibility with older project states, but do not control the new rolls.
- Each slot has a separate timing engine. Slots sharing one MIDI Note can run different roll divisions and shuffles.
- The reduction value uses 0.1% steps; default 1.2%.

## Test checklist
1. Alt+D and left/right: 10 controls. Change Slot and confirm own roll settings return.
2. Enable two layers with the same MIDI Note; choose different divisions and hold the note while playing from REAPER.
3. Save/reopen project and preset; confirm per-slot Roll values persist.
4. Compare Roll Off vs On, shuffle and reduction, pitch bend; try changing BPM while playing.
5. MIDI output when simultaneous layers of the same MIDI Note have independent rolls should be verified for overlapping Note Off messages.

*Not locally compiled:* the JUCE build is provided by GitHub Actions on Windows.
