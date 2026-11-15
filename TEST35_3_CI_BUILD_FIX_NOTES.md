# LR-608 0.99.33 TEST35.3 — CI build correction

- Fix Microsoft C++ error C2086 in `Source/PluginEditor.h`: each of the three Roll ComboBox members is declared only once.
- No changes to the 128 independent Roll engines, per-slot parameters, saved states, or four-column Alt+D layout in TEST35.2.
- The previous GitHub Actions job passed the incremental-search and Alt+T audition verification but stopped while compiling `PluginProcessor.cpp` because it includes the duplicated editor header.
- Next step: run GitHub Actions with this archive; Windows JUCE/VST3 compilation has not been run in the local environment.
