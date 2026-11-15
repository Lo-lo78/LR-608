# LR-608 0.99.33 TEST35.4 — Catalog Audit CI correction

- GitHub Actions TEST35.3 **successfully compiled all targets**, including the VST3. CI only failed at CTest `LR608CatalogAudit`: `Unexpected exposed count 352`.
- The previous test expected 355 page controls. Removing the three Global Roll controls correctly produces 352 exposed page controls while retaining the 405 historical catalog descriptors, including the three legacy Roll IDs for older projects.
- Updated `Tests/CatalogAudit.cpp` to expect 352 page controls and to explicitly verify that legacy Global Roll IDs (`slider249`, `slider253`, `slider254`) remain registered but are not present in page grids.
- Added an audit for 384 distinct Roll parameter IDs (3 per Slot, 128 Slots), ensuring no collision with the historical catalog or exposed page IDs.
- No plugin DSP, UI, preset state, or parameter changes compared to TEST35.3.

**Compilation status:** Windows CI build succeeded in TEST35.3; TEST35.4 requires a new GitHub Actions run to confirm CTest completes. The code was checked locally for the catalog layout and static integration, but the complete Windows toolchain was not run locally.
