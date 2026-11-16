# LR-608 0.99.34 — accessible drum synthesizer (Windows VST3)

**Release:** 16 November 2026  
**Source:** https://github.com/Lo-lo78/LR-608  
**Licence:** AGPL-3.0-or-later (see source-file headers).

LR-608 is a synthesised drum instrument designed for keyboard and screen-reader use. It does not depend on a drum sample library. The native JUCE/VST3 implementation is derived from the LR-608 REAPER JSFX and includes **90 playable synthesis engines**, plus **Off - No sound** as a separate engine-selector choice.

## Instruments and architecture

- 128 independent Slots, up to 128 voices of global polyphony.
- Up to 8 Slots on a MIDI note, with copy/cut/paste of complete layered MIDI-note setups.
- 32 stereo output pairs (channels 1/2 through 63/64), selectable per Slot.
- Drum families include Kick, Snare, Clap, Rim, Low/Mid/High Tom, Closed/Open Hi-Hat, Crash, Ride, Maracas, Clave/Cowbell/Timbales and Zap.
- Synthesis families include 808/909, Simmons, Linn-inspired, Saike variants, FM, physical and modal modelling, and experimental percussion.
- Per-Slot engine, note, choke trigger/target, three independent Roll settings, output routing and Sound/FX settings. Complete kits are saved as `.LR608` presets.

## Sound and FX

Each Slot has **Sound** parameters for its current synthesis engine and **FX** parameters shared across engines. The FX section includes:

- Stereo Pan and Ping Pong Pan depth.
- LFO with wave, one-shot behaviour, phase, frequency, smoothing, PWM/squash controls and bipolar modulation depths for Volume, Low Pass, High Pass and Pan.
- Attack/Decay FX Envelope with bipolar modulation of Volume, Low Pass, High Pass and Pan.
- Resonant Low Pass and High Pass filtering.
- Delay with Wet, Time, Feedback, Speed Glide, repeat Pitch, feedback filtering/resonance and independent Left/Right offsets.
- Degrade (Amount, Bits, Hold and Jitter), Voice Overlap, Roll and Accent controls. Roll Division, Shuffle and Second Hit Velocity Reduction are adjusted per Slot in Alt+D, not in Global.

**Global Transient Designer (introduced in TEST35):** `Alt+G` adds four controls after Output Warmth: **Transient Attack (%)** (-100 to +100, default 0), **Transient Sustain (%)** (-100 to +100, default 0), **Transient Speed (ms)** (8 to 120, default 40), and **Transient Focus** (Low/Full, default Low). Attack raises/softens the initial impact; Sustain boosts/reduces the post-hit tail; Speed adjusts the envelope detector; Focus Low shapes mainly bass/kick body while Full shapes the entire spectrum. Processing is per stereo bus (32 separate L/R-linked detectors); 0 Attack + 0 Sustain reproduces TEST34.1 exactly. Higher positive Attack can raise output peaks: watch host meters and allow headroom. Check output peaks in your host when using strong positive Attack or Sustain values.

**Global output stage (introduced in TEST34):** `Alt+G` includes `Output Warmth (%)`, an optional harmonic body enhancement that processes each stereo output pair separately. 0% is identical to the historical output, while 25-35% is a cautious starting point for listening comparisons. This adds low-order harmonics generated from the bass; it is not a low-shelf EQ, and it does not remove the existing output dynamics.

## Keyboard access

The interface is designed to work without a mouse:

| Shortcut | Action |
| --- | --- |
| `Alt+D` | Four-column Slot grid: Slot/Engine/Layer; MIDI Note/Choke; per-Slot Roll; Output |
| `Alt+L` or `Tab` | Parameter grid |
| `Ctrl+Shift+A` | Toggle the real active automation envelope for the selected VST3 parameter (Alt+D or Alt+L). NVDA says `Auto` when active; no points are deleted on deactivation. |
| Letters/digits in either grid | Incremental search by parameter name or word beginning (Alt+D and Alt+L, including Sound/FX/Global; 1-second prefix) |
| `Shift`+letters/digits in Grid | Search backwards; Shift+1 searches digit 1 rather than the symbol |
| `Alt+F` | Toggle Sound/FX pages |
| `Alt+V` / `Enter` | Value mode; Enter returns from Value to the grid |
| `Alt+E` | Numeric editing in Alt+L only (not Alt+D) |
| `Alt+P` / `Alt+N` | Previous/next Slot |
| `Alt+M` | MIDI-key editing |
| `Alt+C` / `Alt+X` / `Alt+Shift+V` | Copy/cut/paste MIDI layers while MIDI editing is enabled |
| `Alt+Delete` | Remove the selected MIDI layer (MIDI editing mode) |
| `Alt+Shift+Delete` | Remove all layers from the captured MIDI note (MIDI editing mode) |
| `Ctrl+Alt+Shift+Delete` | Clear all 128 Slots (same as Clear All Slots button) |
| `Alt+B` / `Alt+S` | Preset browser / Save kit |
| `Alt+G` | Global controls (including Output Warmth and Transient Designer) |
| `Alt+R` | Slot totals; press R again while Alt is held for a grouped report |
| `Alt+T` | Audition only the current Slot Engine at velocity 115 (no screen-reader announcement) |
| `Alt+H` | Choose one of eight Help languages |
| `Alt+A` | Accessible About dialog |
| `Ctrl+A` / `Ctrl+C` (inside About text) | Select all / Copy selected text to the clipboard |

In the parameter grid, a repeated single key cycles matching parameters with wrap-around. An arrow or other navigation key starts a fresh search. The preset browser retains its original initial-letter navigation. Search affects only navigation, not parameter values or MIDI layer commands.

In the About text panel, keyboard selection is supported (including Shift+arrows). Ctrl+A selects all and Ctrl+C copies the selection.

See **Documentation/LR608Help.html** for the complete shortcut list, navigation conventions and translated manuals (English, Italian, Spanish, Portuguese, French, Russian, Chinese and Japanese).

## Installation

Download the **Windows VST3 release ZIP** from the project's GitHub Releases page. Extract the `LR-608.vst3` plug-in into the Windows VST3 directory (usually `C:\Program Files\Common Files\VST3`), then rescan plug-ins in your DAW. The source ZIP is for development and **does not contain a compiled VST3**.

Factory kit data are embedded in the project and the instrument creates its readable factory presets under `Documents/LR-608/Factory` when needed.

## Building from source

The project uses CMake, JUCE 8.0.15, C++20, and the Windows build script in `Tools/build-windows.ps1`. GitHub Actions uses `.github/workflows/build-windows.yml`; it is recommended for reproducible release builds. The search helper has standalone C++ tests. Run GitHub Actions for the Windows VST3 and its tests before publishing the release. Confirm focus announcements with NVDA in REAPER on the compiled build.

The generated catalogues under `Documentation/` and existing tests under `Tests/` are development references, not evidence of a completed test run for this documentation update.

### Alt+D: Two accessible keyboard grids (introduced in TEST35.5)

Alt+D is a 4-column vertical grid (Slot/Engine/Layer; MIDI Note/Choke Trigger/Choke Target; per-slot Roll Division/Shuffle/Second Hit Velocity Reduction; Output). Navigate with the plain arrows, Page, Home/End; Ctrl+Home/End jumps to the first/last cell. Alt+arrows, Alt+Page, Alt+Home/End change the selected value; Up means increase, Down means decrease. Enter opens the dedicated non-numeric Alt+D Value control; Enter returns to Alt+D, while Esc is passed through to REAPER to close the FX window. Layer > Add Layer is confirmed by Enter inside Value. Typing letters or digits performs progressive incremental search in both Alt+D and Alt+L, with Shift reversing direction. The original Alt+L parameter grid and numeric editor remain unchanged.

### Alt+D: Single accessible focus and Escape (introduced in TEST35.6)

Alt+D now exposes **one focused ComboBox**, exactly as Alt+L does. This is a ten-item virtual parameter list over the four visual columns, not ten separate accessible ComboBoxes. The column selectors remain visible for mouse users but are excluded from accessibility focus and keyboard tab stops. The focus remains on the same Alt+D ComboBox when changing cells, avoiding the repeated NVDA control-role sound. Each selection is the parameter name plus its live value. Enter/Alt+V opens the non-numeric Value control; Enter returns. Plain Escape is no longer consumed by either Alt+D or Alt+L, or by either Value view; REAPER handles its FX window. Dialogs such as Save and Preset Browser still cancel their modal operation on Escape.

## What is new in 0.99.34 (16 November 2026)

- Global Output Warmth adds adjustable low-order harmonic body for each of the 32 stereo output pairs.
- Global Transient Designer adds Attack, Sustain, Speed and Low/Full focus controls.
- All 128 Slots have independent Roll Division, Roll Shuffle and Roll Second Hit Velocity Reduction parameters in Alt+D.
- Alt+D is a four-column vertical parameter grid; it follows Alt+L keyboard navigation conventions, with direct Value controls and no numeric editor.
- Alt+D uses one accessible ComboBox for its ten selections, reducing redundant NVDA focus/role announcements.
- Incremental search, including Shift for reverse search, works in Alt+D and Alt+L.
- Escape in either main grid or Value is passed to REAPER for the host to close the FX window; modal dialogs still use Escape to cancel.


### TEST36.1 — Ctrl+Shift+A automation (LSampler-24 parity)

Like LSampler-24, Ctrl+Shift+A creates/activates or deactivates the selected parameter's REAPER envelope (ACTIVE, not merely VISIBLE). Already-recorded envelope points are kept. The accessible parameter label adds **, Auto** while active, in the Alt+D and Alt+L grids and their Value controls. REAPER's own VST3 host API is used to find the correct FX instance and read/modify the envelope; non-REAPER hosts receive only an unambiguous activation/deactivation menu command, if offered. Ctrl+Shift+A is not active in modal file-name editors or the preset browser. **Slot, Layer, and Output in Alt+D are internal navigation/routing controls, not host-automatable parameters.** Engine, MIDI Note, Choke Trigger/Target, and the three Rolls are host parameters per physical slot. Alt+L Sound/FX uses LR-608's existing shared proxy parameter IDs for the currently selected slot (as in 0.99.34); automation for those controls is therefore shared by the corresponding host proxy, not independent for each slot. No parameter IDs were added, removed or reordered in this test.
