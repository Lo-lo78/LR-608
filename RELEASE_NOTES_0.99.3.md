# LR-608 0.99.3 — Release notes

**Release date:** 7 October 2026  
**Platform:** Windows VST3  
**Project:** https://github.com/Lo-lo78/LR-608

LR-608 is an accessible synthesized drum machine designed for keyboard and screen-reader operation.

The 0.99.3 release includes:

- 128 independent Slots, 128-voice polyphony, up to 8 layers on the same MIDI note, and 32 stereo output pairs (64 channels).
- 90 playable synthesized percussion engines plus the separate `Off - No sound` choice. The engines include 808/909/Simmons, Linn-inspired, Saike, FM, physical/modal and experimental synthesis for kicks, snares, claps, rims, toms, hi-hats, cymbals, maracas, cowbells, timbales and zaps.
- Independent per-Slot MIDI note, choke trigger/target, output assignment, Sound and FX pages.
- FX with resonant LP/HP filtering, Ping Pong Pan, LFO, Attack/Decay envelope with bipolar destinations, Delay with Glide/Pitch per repeat and Left/Right offsets, Degrade, Roll and Accent controls.
- Accessible kit browser, multi-layer MIDI editing, keyboard-driven parameter Grid/Value control, an accessible About dialog (Alt+A) and multilingual Help (Alt+H).

**Documentation-only source refresh, 14 October 2026:** The source archive has been cleaned up for release documentation: the outdated migration README was rewritten, the eight-language Help was supplemented and the CMake project version was aligned with About at 0.99.3. **No new synthesis engines, keyboard commands or DSP changes are claimed after the 0.99.3 release.** This is not a new 0.99.4 release.

Build the source using GitHub Actions; this source archive does not contain a compiled VST3.
