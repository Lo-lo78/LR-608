# LR-608 0.99.34 — Independent Slot Rolls, Accessible Grids and Transient Designer

**Release date:** 16 November 2026  
**Repository:** https://github.com/Lo-lo78/LR-608

## New and improved

- **128 independent Slot Rolls:** Roll Division, Roll Shuffle and Roll Second Hit Velocity Reduction now belong to each Slot and are edited in Alt+D rather than Global. Each Slot can use its own Roll settings.
- **Reorganized Alt+D:** four vertical columns contain Slot/Engine/Layer, MIDI Note/Choke Trigger/Choke Target, the three Roll parameters, and Output. Navigation follows the Alt+L keyboard-grid approach, with Alt+arrows for parameter adjustment. Alt+D uses a non-numeric Value mode.
- **Improved NVDA navigation:** Alt+D exposes one accessible ComboBox for all ten entries, like Alt+L, rather than shifting focus through multiple ComboBoxes.
- **Incremental keyboard search:** find parameters in Alt+D and Alt+L by consecutive letters or numbers; Shift reverses the direction.
- **Escape handling:** Escape in the two main grids and their Value modes is forwarded to REAPER to close the FX window; preset and save dialogs still use Escape for cancellation.
- **Output Warmth:** a global harmonic body control independently processing each stereo output pair; 0% disables the enhancement.
- **Global Transient Designer:** Attack, Sustain, Speed and Focus (Low/Full) for shaping impact and tail. With Attack and Sustain at zero the stage is inactive.

## Preserved

128 Slots, layering, 90 playable sound engines plus Off, 32 stereo output pairs, complete kit presets, Sound/FX pages, MIDI layer tools and Alt+T engine audition.

## Installation and checks

Download and unzip the compiled Windows VST3 archive. Copy `LR-608.vst3` to your Windows VST3 folder and rescan plug-ins in REAPER. The source archive is **not** a compiled plugin.

Source-side integration checks can pass locally, but the final Windows VST3 build and behavior in REAPER/NVDA must be confirmed via GitHub Actions and hands-on testing. Exercise the independent Rolls on two Slots sharing the same MIDI note, Alt+D focus announcements and Escape handling.
