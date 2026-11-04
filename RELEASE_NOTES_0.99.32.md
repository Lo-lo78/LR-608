# LR-608 0.99.32 — Incremental Parameter Search (TEST32)

**Date:** 4 November 2026

This release adds incremental keyboard search to the Sound, FX and Global parameter grids (Alt+L). Type consecutive letters or digits (within 1 second) to narrow the search. Matches at the beginning of the parameter name take priority over the beginning of internal words. Unmodified input searches forwards; Shift reverses the direction, including Shift+digits. A repeated single letter cycles matching parameters with wraparound. Movement/navigation resets the prefix, and a failed multi-character prefix falls back to its last character.

The preset browser retains its existing initial-character navigation. No changes are made to audio DSP, MIDI layers, engine mappings, presets, parameter IDs or automation. All eight Help languages, About and README are updated.

**Tests:** `LR608SearchTest` is included in CTest for GitHub Actions. Local stand-alone C++ search tests can be run with a C++20 compiler. Windows VST3 and screen-reader testing remain required prior to publishing a stable binary release.
