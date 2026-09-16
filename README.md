# LR-608 VST3

Native JUCE/VST3 migration of the LR-608 REAPER JSFX drum machine.

## Current status

The foundation is buildable and contains all 256 stable JSFX parameters plus
14 VST3-only output-routing parameters, the 12
native pages from the Speak FX Params profile, page-specific grid navigation,
APVTS state, MIDI capability, and a 32-bus stereo output layout.
Every instrument can independently select any stereo pair from 1/2 to
63/64, and multiple instruments may share a pair. The original RPL is embedded,
audited, and converted on first use into 14 readable `.LR608` files under
`Documents/LR-608/Factory`. Alt++ and Alt+- load the next/previous preset.

DSP migration is now active. The native processor contains the JSFX shared-grid
Hold Roll, pitch-bend division selection, CC1 three-hit Drag gesture, and
sample-offset MIDI dispatch. All seven original Kick engines are connected:
808, Simmons, 909, and Saike Types 0 through 3. Their native branches include
accent, envelopes, oscillator/filter state, click/noise paths, drive and the
common local compressor. MIDI note 36 follows the new Kick output route.

Each Kick engine owns its JSFX Factory Init bank. User edits remain in that
engine's in-memory bank when moving between engines, while a loaded host or
LR-608 preset remains authoritative for its selected engine. The output path
now follows the JSFX DC, glue, gain staging, limiter, level-match and master
order, with an additional native non-finite/catastrophic-value boundary before
samples reach the host.

This remains a migration checkpoint: the other thirteen voices are not yet
ported. Unsupported instruments remain
silent rather than being replaced by approximate synthesis.

The editor focus hand-off follows LJuno-116: its ignored accessibility root
exposes the real controls, and delayed initial focus is allowed only while the
native plug-in peer itself is focused. Selecting LR-608 among several effects
therefore does not let the JUCE editor capture the FX Chain cursor.

See `Documentation/MIGRATION-AUDIT.md` and the generated CSV maps before
changing DSP, MIDI scheduling, contextual engine banks, or routing.

## Generate catalogues

```powershell
python Tools/generate_catalog.py
```

## Build and test on Windows

```powershell
powershell -ExecutionPolicy Bypass -File Tools/build-windows.ps1
```

The VST3 bundle is generated under
`build/nmake-release/LR608_artefacts/Release/VST3/LR-608.vst3`.
