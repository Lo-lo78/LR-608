#!/usr/bin/env python3
"""Static integration guard for LR-608 0.99.33 / TEST33.

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
assert '0.99.33' in editor and '9 November 2026' in editor
assert 'LR608SearchTest' in cmake and 'LR608IncrementalSearch' in cmake
assert 'verify_incremental_search_integration.py' in workflow
assert 'TEST33' in workflow
processor = (root / 'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
processor_header = (root / 'Source/PluginProcessor.h').read_text(encoding='utf-8')
assert 'void requestSlotAudition(int slot) noexcept;' in processor_header
assert 'std::atomic<int> pendingSlotAudition{-1};' in processor_header
assert 'pendingSlotAudition.store(slot, std::memory_order_release);' in processor
assert processor.count('pendingSlotAudition.load(std::memory_order_acquire)<0') == 2
assert 'pendingSlotAudition.exchange(-1,std::memory_order_acq_rel)' in processor
assert 'triggerSlot(auditionSlot,115,slotMidiNotes[auditionSlot]' in processor
assert 'auto triggerSlot = [&]' in processor
assert 'triggerSlot(slot, hit.velocity, hit.note, hitGeneration)' in processor
assert 'processor.requestSlotAudition(selectedSlot);' in editor
alt_t = editor.split("if (alt && character == 't')", 1)[1].split("if (alt &&", 1)[0]
assert 'requestSlotAudition(selectedSlot)' in alt_t and 'announce' not in alt_t
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
    assert '0.99.33' in body, lang
    assert 'TEST32' in body and 'Shift+1' in body and '115' in body, lang
assert '0.99.32' not in help_html
print('PASS LR-608 TEST33: incremental search + Alt+T slot audition, shortcuts, eight translated manuals, workflow')
