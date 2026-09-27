# Kick 808 LAB migration checkpoint

Migrated 2026-09-27 from `LR-608_Kick808_LAB_TEST11_ClickBodyCoupling.jsfx`.

Approved changes only:
- Keep original nonlinear Kick Click Resonance attack character, but gate its post-click resonator state to a fixed ~2.5 ms tail so it cannot leave a pitched note. TEST3 topology change and TEST5 variable tail were rejected.
- Replace only the Kick 808 local 1176-style compressor with TEST7: fast peak detector, 6 dB soft knee, user Attack, program-dependent two-stage Release. Same six controls.
- Add `Kick Noise Resonance` 0..1 step .001, default 0.
- Add `Kick Noise Granulation` 0..100 step .1, default 0. TEST10 physical flour/contact mesh: broadband noise keeps its spectrum; micro-contact density slows/thins as the existing noise envelope decays. TEST8 resonant granulator was rejected as filter-like.
- Add `Kick Click Body Coupling` 0..0.5 step .001, default .11. This exposes the old hidden `0.11` coupling constant; default preserves the historical kick.

Catalogue compatibility: the three new VST parameters are appended after all pre-existing/captured parameters, at slot snapshot indices 348..350, so all previous indices remain stable.
