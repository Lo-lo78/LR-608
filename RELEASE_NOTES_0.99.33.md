# LR-608 v0.99.33 — TEST33 Slot Audition and Parameter Search

**Release date:** 9 November 2026  
**Platform:** Windows x64 · VST3

This is a release preparation update of the supplied TEST33 source. No synthesis, DSP, preset architecture or control shortcuts were changed for this version bump.

## Included features

- **Alt+T Slot Engine audition:** audition only the selected physical Slot at MIDI velocity 115 without changing focus, parameters, or the current preset. An Off engine is silent.
- **Incremental parameter search:** typing searches names and word beginnings in Sound, FX and Global parameter grids; Shift reverses search, and repeated letters cycle matches. A one-second pause or navigation resets the search.
- **Accessible workflow:** retains keyboard navigation, MIDI layering and editing, screen-reader announcements, preset browsing, and eight help translations.
- **Publication metadata:** updated CMake version, About/version/date, README, multilingual Help, test guard, and GitHub Actions artifact name to v0.99.33.

## Build and installation

The GitHub Actions Windows build produces `LR-608-Windows-VST3.zip` as artifact `LR-608-0.99.33-TEST33-Windows-VST3`. Attach the compiled VST3 ZIP to the GitHub release tagged `v0.99.33` after a successful build. The source ZIP does not contain a compiled VST3.

Static integration verification is run with `python Tests/verify_incremental_search_integration.py`. Full Windows compilation and runtime screen-reader testing require a Windows build and REAPER.
