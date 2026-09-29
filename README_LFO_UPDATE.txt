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

FX envelope: Filter Envelope Attack (s) + Filter Envelope Decay (s) form a shared exponential AD envelope for filter depth, LFO Env Rate, and future FX-envelope destinations. Attack defaults to the previous fixed 4 ms behavior.

FX envelope destinations: the single visible Filter Envelope Depth has been replaced by bipolar Env Depth Volume, Env Depth Filter LP, Env Depth Filter HP, and Env Depth Pan. Negative depth reverses the modulation trajectory while keeping the shared exponential Attack/Decay timing. Architecture v13 migrates legacy filter depth to +LP/+HP values with the old 2:1 range.
