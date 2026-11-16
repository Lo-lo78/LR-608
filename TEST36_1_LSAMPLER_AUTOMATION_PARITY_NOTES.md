# LR-608 TEST36.1 — Ctrl+Shift+A / REAPER active envelope automation

Base: published LR-608 0.99.34 (TEST35.6 UI). This is a test source, **not** a new public release.

- Replicates LSampler-24 TEST187 direct REAPER VST3 host extension: validate owning track/FX/parameter name before touching an envelope; query `ACTIVE` state through `GetSetEnvelopeInfo_String` and create a missing envelope with `GetFXEnvelope(..., true)` only on activation.
- Deactivation sets ACTIVE=0, ARM=0 and VISIBLE=0; preserves automation points. Activation sets ACTIVE=1, VISIBLE=1 and ARM=1. Never substitutes Hide for actual deactivation.
- Ctrl+Shift+A on Alt+D or Alt+L grid and Value toggles the currently selected automatable VST3 parameter; modal dialogs and numeric text input are excluded. On hosts other than REAPER, only unambiguous activate/deactivate host menu items are executed; no hide-only fallback.
- NVDA announces `Parameter, Value, Auto` for ACTIVE envelopes and `Parameter, Value` for inactive. Both ComboBoxes retain focus. The indicator refreshes periodically when editor is focused so changes from REAPER are reflected without false claims.
- Alt+D per-Slot automatable VST3 parameters: Engine; MIDI Note; MIDI Choke Trigger/Target; Roll Division/Shuffle/Second Hit Velocity Reduction. Slot, Layer, Output are UI-only/internal and announce `No automatable parameter selected`.
- Alt+L Sound/FX uses existing shared host proxy parameters. Automation of those proxies is *not* inherently independent per physical Slot (limitation inherited from v0.99.34). Global parameters retain their own host IDs.
- All existing VST3 IDs, version, processor state schema, presets and DSP are unchanged.

## Manual checks in REAPER with NVDA

1. In Alt+D select Slot 1, Roll Division and press Ctrl+Shift+A. Confirm REAPER creates an active envelope for **Slot 1 Roll Division** and NVDA adds `Auto`.
2. Press Ctrl+Shift+A again. Confirm `Auto` disappears, the envelope is inactive (not just hidden), and points remain intact.
3. Repeat for Slot 2 Roll Division: this is a separate VST3 parameter and a separate envelope; verify other Slots are unaffected.
4. Test Engine, Choke and MIDI Note. Slot/Layer/Output should announce not automatable.
5. Test a Global and a Sound/FX Alt+L parameter. Remember that Sound/FX uses shared host proxy IDs.
6. Press Enter to focus Value and test Ctrl+Shift+A. Check the announcements and that no new ComboBox focus noise occurs.
7. In the REAPER envelope window, toggle ACTIVE externally; wait a moment, navigate back, and check the `Auto` suffix tracks actual state.
8. Test two LR-608 instances on separate tracks: toggling on one must not change the other.
9. Check Alt+D/Alt+L incremental search, Esc from FX window, save/load project and preset, and that factory sounds are unchanged.

Local validations pass; complete VST3 compilation and REAPER/NVDA interaction require Windows GitHub Actions and an on-host test.
