// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace lr608
{
void announceToActiveScreenReader (juce::Component&, const juce::String&, bool retrigger = false);
}
