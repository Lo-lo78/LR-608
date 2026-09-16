# LR-608 VST3 migration audit

This directory records the pre-port analysis required before changing the DSP.
The files under `LR-608 JSFX` remain the authoritative sound source; the
`LJuno-116` project remains the architectural and accessibility reference.

## Stable parameter identity

The VST3 parameter ID is `sliderNNN`, preserving the original one-based JSFX
number with three digits. All 256 declarations are retained, including Free
Slider 200 through 203. Those four declarations are currently not musical DSP
controls: 200 is mentioned only in a historical comment and 201-203 only in
their declarations. They remain present for automation/state compatibility.

`parameter-map.csv` is the generated slider-to-VST map. It includes declaration
ranges, steps, choices, page assignment, and a source-occurrence count so that
apparently unused controls remain visible during review.

Fourteen VST3-only routing parameters use stable descriptive IDs (`routeKick`
through `routeZap`). Each independently selects one of the fifteen stereo
pairs, defaults to the main Stereo 1/2 pair, and permits multiple instruments
to share a destination. Toms exposes three routing controls and Cymbal exposes
two; every routing control precedes the source-profile parameters on its page.

One authoritative declaration is internally inconsistent: slider 77, Zap Click
Level, has default 3.04 but maximum 3.0. A VST3 normalized parameter cannot
represent a default outside its legal range, so the VST3 default is clamped to
3.0 while the generated audit retains the literal JSFX declaration for review.

## Native pages and grid

`page-map.csv` is generated directly from `[GroupSlots]` in
`JS - LR - 608.txt`. Profile indices are zero-based, so profile index 0 maps to
JSFX `slider1` and VST3 `slider001`. The pseudo-parameter `Compare` is excluded
because it belongs to Speak FX Params rather than LR-608. Page-specific grid
heights are preserved (4 to 11 rows per column), rather than forcing LJuno's
fixed eight-row layout.

## Engine families and contextual state

The JSFX exposes and captures the engine selection at Note On, which prevents a
live engine change from reinterpreting a ringing voice. Engine counts are:

| Family | Selector | Engines | Context bank stride |
|---|---:|---:|---:|
| Kick | 247 | 7 | 25 |
| Snare 1 | 248 | 8 | 31, independent slot |
| Snare 2 | 198 | 8 | 31, independent slot |
| Clap | 207 | 6 | 11 |
| Rim | 209 | 4 | 18 |
| Toms | 219 | 5 | 54 |
| HiHat | 217 | 7 | 10 |
| Cymbal | 214 | 4 | 7 |
| Maracas | 216 | 4 | 8 |
| Clave/Cowbell | 215 | 7 | 11 |
| Zap | 199 | 11 | 22 |

Engine banks are runtime contextual memory, deliberately not serialized by the
JSFX. Host preset/state restores win. The VST3 must preserve this switching
rule and must keep Snare 1 and Snare 2 state completely independent.

## MIDI map

| Note/control | Action |
|---|---|
| 36 | Kick |
| 37 | Rim |
| 38 | Snare 1 |
| 39 | Clap |
| 40 | Snare 2 |
| 41 or 43 | Low Tom |
| 42 or 44 | Closed/Pedal HiHat |
| 45 or 47 | Mid Tom |
| 46 | Open HiHat |
| 48 or 50 | High Tom |
| 49 | Crash |
| 51 | Ride |
| 52 | Zap |
| 56 | Clave/Cowbell (also the drag-enabled timbale gesture) |
| 58 | Maracas |
| CC1 | Binary drag gesture: 0 off, any non-zero value on |
| Pitch bend | Temporary roll-division selection; never overwrites slider249 |

Incoming MIDI is passed through. Note events use their event offset. Roll and
drag scheduling continue on per-sample clocks; translating events only at block
start would be a behavioural regression.

## Audio outputs

The JSFX uses 30 channels: the main stereo pair plus fourteen stereo stems.

| JSFX channels | VST3 bus |
|---|---|
| 0/1 | Main stereo output |
| 2/3 | Kick |
| 4/5 | Snare 1 |
| 6/7 | Snare 2 |
| 8/9 | Clap |
| 10/11 | Rim |
| 12/13 | HiHat |
| 14/15 | Crash |
| 16/17 | Ride |
| 18/19 | Low Tom |
| 20/21 | Mid Tom |
| 22/23 | High Tom |
| 24/25 | Zap |
| 26/27 | Clave/Cowbell |
| 28/29 | Maracas |

Stereo mode outputs the drum mix on the main bus and clears all stems.
Multichannel mode clears the main bus and outputs the fourteen processed stems.
Glue gain, limiter gain, engine-change guards, Zap level matching, and master
gain are part of this routing and must be applied before the bus split.

## Presets and persistent state

`preset-audit.csv` verifies every decoded RPL entry and its token count. The RPL
payload has a REAPER state discontinuity after slider 64: matching LJuno's
tested importer, token index is `slider - 1` through slider 64 and `slider`
afterward. The VST3 state must store all 256 APVTS values plus any non-parameter
state needed for the accessible preset browser. Runtime oscillator, filter,
envelope, roll, drag, delay-line, and contextual factory-bank working memory is
reinitialized, not persisted as patch state.

## LJuno systems to adapt

The following LJuno components are reusable in concept and implementation:

- generated stable parameter catalogue and APVTS layout;
- generated native page catalogue;
- Parameter / Value / numeric Edit focus model;
- page, grid, direct-initial, reset, Help, preset, and global Alt shortcuts;
- deferred focus transfer and fresh UI Automation focus events;
- native Windows screen-reader notification helper;
- accessible preset browser with preview/cancel/restore semantics;
- embedded RPL conversion and readable user-preset format;
- APVTS XML state save/restore.

LR-608 must extend the grid descriptor with `rowsPerColumn`, because the source
profile explicitly defines different heights per page.

## Direct-port hazards

- EEL2 variables default to zero and persist by scope; C++ state needs explicit
  initialization and matching reset points.
- EEL2 conditional expressions and chained assignments do not map safely to
  C++ without checking evaluation order and branch return semantics.
- `rand()` distribution/call order affects noise, collision density, phase, and
  per-hit variation. A deterministic test RNG is required for comparisons.
- EEL2 floating modulo, `floor`, implicit truth conversion, and division need
  explicit C++ equivalents; delay indices need bounded integer conversion.
- Every voice captures its engine at trigger time. Reading the live selector
  during rendering would alter existing tails.
- Roll, Note Off, and drag deadlines are absolute sample clocks; JUCE MIDI
  offsets must be dispatched inside the render loop.
- Engine changes run guarded fades and, for Hyper Spring, incremental memory
  clearing. Resetting all state synchronously changes both sound and CPU timing.
- Deep idle still advances drag timing and must preserve stereo input pass-through
  versus multichannel silence.
- Denormal handling, NaN clamps, feedback limits, and sample-rate-dependent
  coefficient caches are audible/stability requirements, not optional cleanup.

## Native DSP checkpoint 1

- `DrumTimingEngine` ports the shared Hold Roll grid, pitch-bend division map,
  shuffle/quiet-hit alternation and CC1 Drag clocks independently of block size.
- Normal Note On messages are intentionally corrected to their JUCE sample
  offset; the JSFX calls `lr_trigger()` during `@block` and can anticipate them.
- `Kick808Voice` ports `kick_engine_voice == 0`; `KickOtherVoices` ports
  Simmons, 909 and the shared Saike Type 0..3 implementation, including its
  SVF, bell filters and optional 7 Hz elliptic frequency shifter.
- Every Kick branch uses the common trigger/accent contract, local DC blocker,
  compressor and JSFX engine-specific output scaling.
- MIDI note 36 renders on `routeKick`; route 0 is Main and route 1..14 selects
  the corresponding additional stereo bus. Multiple voices may share a bus.
- All other voices, engine-change guards, shared glue and limiter remain
  pending and are never substituted with generic synthesis.
- Tests cover roll/drag boundaries, deterministic finite/non-silent rendering
  for all seven Kick engines, sample-offset onset, and routing to a non-main
  stereo pair.

## Accessibility focus checkpoint

The editor now uses the LJuno-116 focus-transfer contract: keyboard focus
container, ignored editor accessibility root, explicit control order, delayed
native-UIA readiness check, child-focus preservation, and—critically—the
`ComponentPeer::isFocused()` guard. Construction alone never requests focus.
Selecting LR-608 in an FX Chain may expose its editor but cannot move focus
until the native plug-in window is actually entered.

## Native DSP checkpoint 2

- All 7 x 25 Kick Factory Init values are transcribed from
  `lr608_factory_kick_value()` in the exact live-parameter order.
- Engine-only moves capture the old editable bank and recall the target bank.
  Preset/state loads rebuild Factory memory and capture the visible selected
  engine, matching the JSFX rule that presets outrank hidden RAM.
- An engine change kills the previous tail before it can render through the
  new parameter map. The JSFX hold/fade guard remains pending; this checkpoint
  uses a safe hard reset instead of risking NaN or a destructive peak.
- `OutputStage` ports per-stem/stereo 5 Hz DC blockers, shared 3/20 ms glue,
  threshold 0.6 and ratio 2, 0.5 and 0.35 gain stages, the 0.98 limiter with
  0.15 excess slope, Zap compensation and `master * 1.2`.
- The VST boundary additionally clears non-finite samples and caps catastrophic
  output to +/-8. This lies outside the musical transfer curve and activates
  only for corrupt or unstable state.
