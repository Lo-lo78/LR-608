#!/usr/bin/env python3
"""Static integration guard for LR-608 0.99.32 / TEST32.

The actual search algorithm is tested by LR608SearchTest in CTest.
"""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
editor = (root / 'Source/PluginEditor.cpp').read_text(encoding='utf-8')
header = (root / 'Source/PluginEditor.h').read_text(encoding='utf-8')
key = (root / 'Source/ParameterSearchKey.h').read_text(encoding='utf-8')
core = (root / 'Source/IncrementalParameterSearch.h').read_text(encoding='utf-8')
help_html = (root / 'Documentation/LR608Help.html').read_text(encoding='utf-8')
cmake = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
workflow = (root / '.github/workflows/build-windows.yml').read_text(encoding='utf-8')

assert 'namespace lr608 {' in core
assert 'intervalMs = 1000' in core and 'sameLetterCycle' in core
assert 'CharLowerBuffW' in core and '#define NOMINMAX' in core
assert '#undef min' in core and '#undef max' in core
assert 'lr608::IncrementalParameterSearch parameterSearch;' in header
assert 'parameterSearch.next (' in editor
assert 'std::wstring (visibleNames[' in editor
assert 'lr608ParameterSearchCharacter(key)' in editor
assert 'if (code >= juce::KeyPress::F1Key' in key
assert key.index("if (code >= '0' && code <= '9')") < key.index('const auto typed')
assert 'numberPad0' in key and 'numberPad9' in key
assert 'mods.isCtrlDown() || mods.isAltDown() || mods.isCommandDown()' in key
assert '0.99.32' in editor and '4 November 2026' in editor
assert 'LR608SearchTest' in cmake and 'LR608IncrementalSearch' in cmake
assert 'verify_incremental_search_integration.py' in workflow
assert 'TEST32' in workflow
assert 'parameterSearch.reset();' in editor
assert 'updateParameterList()' in editor
for short in ('alt&&ctrl&&key.getModifiers().isShiftDown()&&code==juce::KeyPress::deleteKey',
              'alt&&!ctrl&&key.getModifiers().isShiftDown()&&code==juce::KeyPress::deleteKey',
              'alt&&!ctrl&&!key.getModifiers().isShiftDown()&&code==juce::KeyPress::deleteKey'):
    assert short in editor, f'MIDI layer shortcut removed: {short}'
articles = re.findall(r'<article id="([a-z]+)"[^>]*>(.*?)</article>', help_html, flags=re.S)
assert len(articles) == 8
assert {lang for lang, _ in articles} == {'en', 'it', 'es', 'pt', 'fr', 'ru', 'zh', 'ja'}
for lang, body in articles:
    assert '0.99.32' in body, lang
    assert 'TEST32' in body and 'Shift+1' in body, lang
assert '0.99.31' not in help_html
print('PASS LR-608 TEST32: code integration, shortcuts, version/About, eight translated manuals, workflow')
