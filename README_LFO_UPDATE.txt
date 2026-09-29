LR-608 Triggered LFO - LJuno parameter completion

Added intrinsic LJuno-style LFO controls while preserving one trigger-only LFO per voice/engine:
- LFO Wave
- LFO One Shot
- LFO One Shot Percent
- LFO Phase
- LFO Frequency (BPM)
- LFO Env Rate
- LFO Delay
- LFO Smooth
- LFO Square PWM
- LFO Upper Squash
- LFO Lower Squash
- LFO Depth Volume
- LFO Depth Filter LP
- LFO Depth Filter HP
- LFO Depth Pan

Mode is intentionally omitted: LR-608 LFO is always Triggered.
Cross-LFO controls are intentionally omitted: LR-608 uses one LFO per voice/engine.
All new parameter IDs are appended after existing parameters to preserve earlier indices.
When all four destination depths are zero, the LFO DSP path is skipped.

CPU optimization pass:
- The four LFO depths are read first at trigger time. If all are zero, no other LFO parameter is read or initialized.
- A zero destination mask bypasses all LFO phase, waveform, smoothing, one-shot, delay, envelope-rate and filter-modulation work.
- LFO state is voice-local and is cleared as soon as the source engine stops. Filter tails continue with static coefficients only.
- If only LP/HP destinations are enabled, the LFO is evaluated only on the existing 16-sample filter-coefficient update cadence; phase advances by the equivalent elapsed sample count.
- Volume/Pan destinations use audio-rate evaluation, with trigger-precomputed coefficients and no pow() calls in the normal one-sample path.
