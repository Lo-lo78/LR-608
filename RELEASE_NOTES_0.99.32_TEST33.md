# LR-608 0.99.32 — TEST33: Alt+T Slot Audition

- **Alt+T** now plays the sound configured in the currently selected Slot at fixed **MIDI velocity 115** instead of announcing the Engine name to the screen reader.
- Auditions only that physical Slot, not every layer that shares its MIDI note; an Engine set to Off remains silent.
- Does not alter keyboard focus, selected Slot, preset data, MIDI output, or parameter values.
- Playback is queued from the UI to the audio thread; it uses the existing 128-voice synthesis, Slot output routing, Voice Overlap, ping-pong pan, and Slot delay.
- Normal incoming MIDI triggers and layering are unchanged. The voice-preview request bypasses the MIDI roll scheduler and does not update the last-received MIDI navigation note.
- Eight help languages and GitHub Actions checks updated.

No new parameters or preset state changes. Windows VST3 compilation and real REAPER/NVDA testing must be completed using GitHub Actions.
