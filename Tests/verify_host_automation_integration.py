"""Static API/mapping regression for TEST36.1; the REAPER-host runtime test is manual."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
processor = (root/'Source/PluginProcessor.cpp').read_text(encoding="utf-8")
header = (root/'Source/PluginProcessor.h').read_text(encoding="utf-8")
editor_header = (root/'Source/PluginEditor.h').read_text(encoding="utf-8")
editor = (root/'Source/PluginEditor.cpp').read_text(encoding="utf-8")
manual = (root/'Documentation/LR608Help.html').read_text(encoding="utf-8")
for term in ('getVST3ClientExtensions()', 'getReaperEnvelopeActive(', 'toggleReaperEnvelopeActive('):
    assert term in header and term in processor, term
for term in ('0x79655E36', 'GetTouchedOrFocusedFX', 'getReaperParent(1)', 'TrackFX_GetParamName', 'GetFXEnvelope', 'GetSetEnvelopeInfo_String', '"ACTIVE"', '"VISIBLE"', '"ARM"'):
    assert term in processor,term
assert 'getEnvelope(fx.track, fx.index, fx.parameter, true)' in processor
assert 'getEnvelope(fx.track, fx.index, fx.parameter, false)' in processor
for term in ('case 1: id = lr608::slotEngineId', 'case 3: id = lr608::slotNoteId', 'case 4: id = lr608::slotChokeTriggerId', 'case 5: id = lr608::slotChokeTargetId', 'case 6: id = lr608::slotRollDivisionId', 'case 7: id = lr608::slotRollShuffleId', 'case 8: id = lr608::slotRollReductionId'):
    assert term in editor,term
assert 'default: return nullptr;' in editor
# TEST36.3: On Windows JUCE's isCommandDown() is also true for Ctrl.
# The previously guarded combination Ctrl && !Command could NEVER match.
shortcut = editor_header.split('inline bool lr608IsEnvelopeShortcut', 1)[1].split('class LR608ParameterComboBox', 1)[0]
assert 'mods.isCtrlDown() && mods.isShiftDown() && !mods.isAltDown()' in shortcut
assert 'isCommandDown()' not in shortcut
assert 'if (lr608IsEnvelopeShortcut(key))' in editor
for control in ('slotGridSelector', 'parameterSelector', 'slotValue', 'parameterValue'):
    assert f'{control}.setShortcutHandler(' in editor, control
for control_type in ('LR608ParameterComboBox', 'LR608ValueSlider', 'LR608ValueButton'):
    assert f'class {control_type} final' in editor_header
    assert 'if (lr608IsEnvelopeShortcut(key) && shortcut && shortcut(key, this)) return true;' in editor_header
assert 'LR608ParameterComboBox slotGridSelector' in editor_header
assert 'LR608ParameterComboBox parameterSelector' in editor_header
assert 'LR608ValueButton slotValue' in editor_header
assert 'LR608ValueSlider parameterValue' in editor_header
assert 'getHostAutomationEnvelopeState' in editor
assert 'syncSlotAutomationIndicator' in editor and 'syncGridAutomationIndicator' in editor
assert 'selectedSlotAutomationLine()' in editor and 'selectedGridAutomationLine()' in editor
assert 'return true; // Do not fall back to Hide' in editor
assert '"No automatable parameter selected"' in editor
assert 'if (kind != HostEnvelopeCommand::activate && kind != HostEnvelopeCommand::deactivate)' in editor
assert 'observedSlotAutomationState == 1' in editor and 'observedGridAutomationState == 1' in editor
assert manual.count('Ctrl+Shift+A') >= 8
print('PASS TEST36.3: REAPER envelope, Windows Ctrl+Shift+A, direct focus-control dispatch, NVDA Auto, 8 manuals')
