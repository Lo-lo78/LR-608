#!/usr/bin/env python3
"""Guard the Alt+D/Alt+L single-control NVDA focus model and Escape routing."""
from pathlib import Path
p=Path(__file__).resolve().parents[1]
editor=(p/'Source/PluginEditor.cpp').read_text(encoding='utf-8')
header=(p/'Source/PluginEditor.h').read_text(encoding='utf-8')
assert 'LR608ParameterComboBox slotGridSelector;' in header
assert 'class LR608ParameterComboBox final : public juce::ComboBox' in header
assert 'slotGridSelector.setTitle("Slot parameter")' in editor
assert 'slotGridSelector.setSelectedItemIndex(next,juce::sendNotificationSync)' in editor
assert 'slotGridSelector.setSelectedItemIndex(found,juce::sendNotificationSync)' in editor
assert 'box->setAccessible(false)' in editor and 'box->setWantsKeyboardFocus(false)' in editor
assert 'const auto slotBarSource=source==&slotGridSelector;' in editor
assert 'slotGridSelector.hasKeyboardFocus(false)' in editor
assert 'slotGridSelector.changeItemText(i+1,label)' in editor
assert 'if(code==juce::KeyPress::escapeKey && !globalOpen)' in editor
assert 'return false; // let REAPER close the FX window' in editor
# Neither Alt+D nor Alt+L should force focus to the other grid on Escape.
assert 'if(code==juce::KeyPress::escapeKey){focusSlotColumn(selectedSlotColumn);return true;}' not in editor
assert 'if(code==juce::KeyPress::escapeKey){requestShortcutFocus(parameterSelector);return true;}' not in editor
assert 'slotUiNavigate(selectedSlotColumn,rowDelta,columnDelta)' in editor
assert 'LR608ValueButton slotValue' in header
assert 'class LR608ValueButton final : public juce::TextButton' in header
print('PASS TEST35.6: Alt+D single accessible ComboBox; ESC host handling; Alt+L unchanged')
