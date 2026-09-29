# Trigger-only per-slot LFO

The slot FX LFO is intentionally not free-running. Each polyphonic voice starts its LFO at phase zero when the drum is triggered, and the LFO stops as soon as that source voice ends.

CPU rules:
- If all four destination depths are zero, `lfoActive` is false and render skips phase, waveform, smoothing and LFO filter work entirely.
- Volume/Pan-only modulation does not recalculate filter coefficients.
- LP/HP coefficient updates are shared with the existing filter-envelope control-rate update (once every 16 samples).
- Filter tails may continue after the drum source ends, but the LFO is disabled and coefficients return to their static/envelope values.

Controls:
- LFO Wave: Sine, Triangle, Saw Up, Saw Down, Square, Sample Hold.
- LFO One Shot: Off/On.
- LFO One Shot Percent: 0..100.
- LFO Phase: 0..1 cycle.
- LFO Frequency (BPM): 0.125..128, tempo scaled as `rate * BPM / 240`, matching LJuno-116.
- LFO Env Rate: -10..+10. LR-608 uses a voice-local trigger envelope because there is no single shared synth ADSR.
- LFO Delay: 0..20 seconds, same gradual fade-in law as LJuno-116.
- LFO Smooth: 0..100, using the LJuno-116 smoothing law.
- LFO Square PWM: 0.05..0.95.
- LFO Upper Squash / Lower Squash: 0..1, independently scale the positive and negative LFO halves.
- LFO Depth Volume: -1..1.
- LFO Depth Filter LP: -8..8 octaves.
- LFO Depth Filter HP: -8..8 octaves.
- LFO Depth Pan: -1..1.

Tom voices use their historical Tom pan as the LFO pan centre; no second static Tom pan stage is introduced.
