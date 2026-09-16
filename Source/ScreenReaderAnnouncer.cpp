// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ScreenReaderAnnouncer.h"

#if JUCE_WINDOWS
 #include <Windows.h>
 #include <UIAutomation.h>
 #include <oleauto.h>
#endif

namespace lr608
{
void announceToActiveScreenReader (juce::Component& source, const juce::String& message, bool retrigger)
{
    source.setDescription (message);
#if JUCE_WINDOWS
    if (! UiaClientsAreListening())
        return;
    auto* handler = source.getAccessibilityHandler();
    if (handler == nullptr)
        return;
    auto* unknown = reinterpret_cast<IUnknown*> (handler->getNativeImplementation());
    if (unknown == nullptr)
        return;
    IRawElementProviderSimple* provider = nullptr;
    if (FAILED (unknown->QueryInterface (__uuidof (IRawElementProviderSimple),
                                         reinterpret_cast<void**> (&provider))))
        return;
    auto* text = SysAllocString (message.toWideCharPointer());
    auto* activity = SysAllocString (retrigger ? L"LR608MidiNavigation" : L"LR608");
    UiaRaiseNotificationEvent (provider, NotificationKind_Other,
                               retrigger ? NotificationProcessing_MostRecent
                                         : NotificationProcessing_ImportantMostRecent,
                               text, activity);
    SysFreeString (text);
    SysFreeString (activity);
    provider->Release();
#else
    if (auto* handler = source.getAccessibilityHandler())
        handler->notifyAccessibilityEvent (juce::AccessibilityEvent::titleChanged);
#endif
}
}
