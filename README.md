# LR-608 0.99.31 — accessible drum synthesizer (Windows VST3)

**Release:** 14 October 2026  
**Source:** https://github.com/Lo-lo78/LR-608  
**Licence:** AGPL-3.0-or-later (see source-file headers).

LR-608 is a synthesised drum instrument designed for keyboard and screen-reader use. It does not depend on a drum sample library. The native JUCE/VST3 implementation is derived from the LR-608 REAPER JSFX and includes **90 playable synthesis engines**, plus **Off - No sound** as a separate engine-selector choice.

## Instruments and architecture

- 128 independent Slots, up to 128 voices of global polyphony.
- Up to 8 Slots on a MIDI note, with copy/cut/paste of complete layered MIDI-note setups.
- 32 stereo output pairs (channels 1/2 through 63/64), selectable per Slot.
- Drum families include Kick, Snare, Clap, Rim, Low/Mid/High Tom, Closed/Open Hi-Hat, Crash, Ride, Maracas, Clave/Cowbell/Timbales and Zap.
- Synthesis families include 808/909, Simmons, Linn-inspired, Saike variants, FM, physical and modal modelling, and experimental percussion.
- Per-Slot engine, note, choke trigger/target, output routing and Sound/FX settings. Complete kits are saved as `.LR608` presets.

## Sound and FX

Each Slot has **Sound** parameters for its current synthesis engine and **FX** parameters shared across engines. The FX section includes:

- Stereo Pan and Ping Pong Pan depth.
- LFO with wave, one-shot behaviour, phase, frequency, smoothing, PWM/squash controls and bipolar modulation depths for Volume, Low Pass, High Pass and Pan.
- Attack/Decay FX Envelope with bipolar modulation of Volume, Low Pass, High Pass and Pan.
- Resonant Low Pass and High Pass filtering.
- Delay with Wet, Time, Feedback, Speed Glide, repeat Pitch, feedback filtering/resonance and independent Left/Right offsets.
- Degrade (Amount, Bits, Hold and Jitter), Voice Overlap, Roll and Accent controls.

## Keyboard access

The interface is designed to work without a mouse:

| Shortcut | Action |
| --- | --- |
| `Alt+D` | Main Slot area |
| `Alt+L` or `Tab` | Parameter grid |
| `Alt+F` | Toggle Sound/FX pages |
| `Alt+V` | Value mode |
| `Alt+E` | Numeric editing |
| `Alt+P` / `Alt+N` | Previous/next Slot |
| `Alt+M` | MIDI-key editing |
| `Alt+C` / `Alt+X` / `Alt+Shift+V` | Copy/cut/paste MIDI layers while MIDI editing is enabled |
| `Alt+Delete` | Remove the selected MIDI layer (MIDI editing mode) |
| `Alt+Shift+Delete` | Remove all layers from the captured MIDI note (MIDI editing mode) |
| `Ctrl+Alt+Shift+Delete` | Clear all 128 Slots (same as Clear All Slots button) |
| `Alt+B` / `Alt+S` | Preset browser / Save kit |
| `Alt+G` | Global controls |
| `Alt+R` | Slot totals; press R again while Alt is held for a grouped report |
| `Alt+T` | Announce the selected engine |
| `Alt+H` | Choose one of eight Help languages |
| `Alt+A` | Accessible About dialog |
| `Ctrl+A` / `Ctrl+C` (inside About text) | Select all / Copy selected text to the clipboard |

In the About text panel, keyboard selection is supported (including Shift+arrows). Ctrl+A selects all and Ctrl+C copies the selection.

See **Documentation/LR608Help.html** for the complete shortcut list, navigation conventions and translated manuals (English, Italian, Spanish, Portuguese, French, Russian, Chinese and Japanese).

## Installation

Download the **Windows VST3 release ZIP** from the project's GitHub Releases page. Extract the `LR-608.vst3` plug-in into the Windows VST3 directory (usually `C:\Program Files\Common Files\VST3`), then rescan plug-ins in your DAW. The source ZIP is for development and **does not contain a compiled VST3**.

Factory kit data are embedded in the project and the instrument creates its readable factory presets under `Documents/LR-608/Factory` when needed.

## Building from source

The project uses CMake, JUCE 8.0.15, C++20, and the Windows build script in `Tools/build-windows.ps1`. GitHub Actions uses `.github/workflows/build-windows.yml`; it is recommended for reproducible release builds. This source package has **not** been compiled locally as part of the documentation update.

The generated catalogues under `Documentation/` and existing tests under `Tests/` are development references, not evidence of a completed test run for this documentation update.
