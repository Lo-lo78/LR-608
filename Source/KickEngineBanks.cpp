// SPDX-License-Identifier: AGPL-3.0-or-later
#include "KickEngineBanks.h"

namespace lr608
{
namespace
{
constexpr const char* ids[] {
    "slider011", "slider012", "slider013", "slider014", "slider015", "slider016",
    "slider078", "slider079", "slider017", "slider148", "slider149", "slider101",
    "slider018", "slider100", "slider107", "slider138", "slider140", "slider067",
    "slider102", "slider105", "slider103", "slider104", "slider106", "slider141",
    "slider142"
};

constexpr float factory[8][25] {
    { 10.0f, 0.26f, 0.46f, 0.061f, 0.2f, 0.12f, 0.0f, 1.0f, 0.305f, 3.0f, 0.5f, 0.1f, 0.79f, 15.0f, 40.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 200.0f, 0.0f, 0.5f },
    { 20.37f, 0.13f, 0.56f, 0.1f, 0.04f, 0.25f, 0.08f, 0.5f, 2.0f, 1.0f, 0.15f, 0.4f, 0.35f, 41.0f, 10.0f, 0.16f, 0.0f, 0.04f, -2.0f, 4.0f, 5.125f, 1.0f, 120.0f, 0.62f, 0.5f },
    { 10.0f, 0.18f, 0.7f, 0.021f, 0.071f, 0.12f, 0.0f, 0.03f, 1.5f, 1.2f, 0.15f, 0.2f, 0.72f, 13.0f, 100.0f, 0.5f, -0.5f, 0.0f, -2.0f, 4.0f, 8.0f, 1.0f, 420.0f, 0.0f, 0.55f },
    { 7.0f, 0.15f, 0.6f, 0.14f, 0.3f, 0.3f, 1.0f, 0.33f, 0.0f, 2.48f, 0.79f, 0.78f, 0.5f, 390.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 400.0f, 0.0f, 0.5f },
    { 9.0f, 0.23f, 0.45f, 0.14f, 0.41f, 0.43f, 1.0f, 0.33f, 4.0f, 1.0f, 0.0f, 0.1f, 1.2f, 222.0f, 0.0f, 0.0f, 0.92f, 1.0f, 0.0f, 0.0f, 8.0f, 0.0f, 400.0f, 0.0f, 0.5f },
    { 9.1f, 0.06f, 0.59f, 0.15f, 0.25f, 0.25f, 0.6f, 0.53f, 4.0f, 2.48f, 0.68f, 0.0f, 1.3f, 390.0f, 0.0f, 0.0f, -1.0f, 0.75f, 0.0f, 0.0f, 8.0f, 0.0f, 400.0f, 0.0f, 0.5f },
    { 7.0f, 0.15f, 0.6f, 0.14f, 0.3f, 0.3f, 1.0f, 0.33f, 2.0f, 2.48f, 0.79f, 0.78f, 0.5f, 390.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 8.0f, 0.0f, 400.0f, 0.0f, 0.5f },
    { 15.0f, -1.07f, 0.08f, 0.2f, 0.07f, 0.91f, 1.0f, 0.18f, 2.1f, 4.72f, 0.424f, 0.32f, 0.94f, 31.8f, 102.0f, 0.34f, 0.34f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 100.0f, 0.0f, 0.5f }
};
}

KickEngineBanks::KickEngineBanks (juce::AudioProcessorValueTreeState& valueTree) : state (valueTree)
{
    resetFromCurrentState();
}

void KickEngineBanks::fillFactory()
{
    for (int engine = 0; engine < engineCount; ++engine)
        for (int parameter = 0; parameter < parameterCount; ++parameter)
            banks[engine][parameter] = factory[engine][parameter];
}

void KickEngineBanks::resetFromCurrentState()
{
    fillFactory();
    currentEngine = juce::jlimit (0, engineCount - 1,
        juce::roundToInt (state.getRawParameterValue ("slider247")->load()));
    captureCurrent(); // host/preset state is authoritative for its selected engine
}

bool KickEngineBanks::ownsParameter (juce::StringRef id)
{
    for (const auto* candidate : ids)
        if (id == juce::StringRef (candidate)) return true;
    return false;
}

void KickEngineBanks::captureCurrent()
{
    if (applying) return;
    for (int parameter = 0; parameter < parameterCount; ++parameter)
        banks[currentEngine][parameter] = state.getRawParameterValue (ids[parameter])->load();
}

void KickEngineBanks::load (int engine)
{
    const juce::ScopedValueSetter guard (applying, true);
    for (int parameter = 0; parameter < parameterCount; ++parameter)
        if (auto* target = state.getParameter (ids[parameter]))
        {
            const auto normal = target->convertTo0to1 (banks[engine][parameter]);
            target->beginChangeGesture();
            target->setValueNotifyingHost (normal);
            target->endChangeGesture();
        }
}

bool KickEngineBanks::switchTo (int engine)
{
    engine = juce::jlimit (0, engineCount - 1, engine);
    if (engine == currentEngine || applying) return false;
    captureCurrent();
    currentEngine = engine;
    load (currentEngine);
    return true;
}
}
