#!/usr/bin/env python3
"""Static integration guard for LR-608 incremental search and slot audition.

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
# Check the release version consistently instead of hardcoding the previous release.
version_match = re.search(r'project\(LR608 VERSION (\d+\.\d+\.\d+)', cmake)
assert version_match, 'Missing CMake release version'
version = version_match.group(1)
assert f'constexpr auto lr608Version = "{version}";' in editor
assert 'constexpr auto lr608ReleaseDate = "16 November 2026";' in editor
assert 'LR608SearchTest' in cmake and 'LR608IncrementalSearch' in cmake
assert 'verify_incremental_search_integration.py' in workflow
# Validate the actual CI checks and packaging stages, without coupling this
# integration audit to a TEST number in the artifact display name.
assert 'actions/checkout@v4' in workflow
assert 'python Tests/verify_incremental_search_integration.py' in workflow
assert 'powershell -ExecutionPolicy Bypass -File Tools/build-windows.ps1' in workflow
assert 'Compress-Archive -Path $vst3' in workflow
assert 'actions/upload-artifact@v4' in workflow
assert f'LR-608.{version}.vst3.zip' in workflow and f'LR-608-{version}-Windows-VST3' in workflow
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
    assert version in body, lang
    assert ('TEST32' in body or 'TEST35.5' in body) and 'Shift+1' in body and '115' in body, lang
assert '0.99.32' not in help_html
print('PASS LR-608: incremental search + Alt+T slot audition, shortcuts, eight translated manuals, CI workflow')

# TEST35.5: Alt+D is a second grid; both share the same prefix algorithm,
# but only Alt+L has a numerical editor.
geometry = (root / 'Source/SlotGridNavigation.h').read_text(encoding='utf-8')
assert 'slotUiNavigate' in geometry and 'slotUiReverseValueOrder' in geometry
assert 'lr608::slotUiNavigate(selectedSlotColumn,rowDelta,columnDelta)' in editor
assert 'lr608::slotUiReverseValueOrder(column)' in editor
assert 'lr608::IncrementalParameterSearch slotParameterSearch;' in header
assert 'slotParameterSearch.next(' in editor
assert 'selectNextSlotParameterStartingWith(searchKey,shift)' in editor
assert 'LR608ValueButton slotValue' in header
assert 'class LR608ValueButton final : public juce::TextButton' in header
assert 'slotValue.setWantsKeyboardFocus(true)' in editor
assert 'if(code==juce::KeyPress::returnKey){focusSlotValueAndAnnounce();return true;}' in editor
assert 'if(slotBarSource||source==&slotValue){announce("Alt+D Value has no numeric editor");return true;}' in editor
assert 'LR608SlotGridNavigation' in cmake
print('PASS TEST35.5: two grids, Alt+D non-numeric Value, roll direction and incremental search')
