# LR-608 0.99.34 TEST36.2 - Windows CI Python encoding fix

Base: TEST36.1 (Ctrl+Shift+A REAPER automation and NVDA Auto suffix).

## Fix

GitHub Actions stopped at `Verify Ctrl+Shift+A REAPER automation integration` before attempting to compile the VST3. Windows Python 3.12 used its default cp1252 encoding in `Tests/verify_host_automation_integration.py`, which could not decode UTF-8 text in `Source/PluginEditor.cpp` (`UnicodeDecodeError`, byte 0x81 at position 159729).

All four text file reads in that test now explicitly use `read_text(encoding="utf-8")`.

This change does not modify the VST3/audio code, REAPER envelope handling, keyboard shortcuts, NVDA output, or plugin version. The official version remains 0.99.34.

## Locally verified

- `python Tests/verify_host_automation_integration.py` PASS
- `python Tests/verify_alt_d_single_accessible_grid.py` PASS
- `python Tests/verify_incremental_search_integration.py` PASS

The VST3 Windows build and REAPER/NVDA runtime behavior still require GitHub Actions and REAPER testing.
