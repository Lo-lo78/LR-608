// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SnareEngineBanks.h"

namespace lr608
{
namespace
{
constexpr const char* ids[2][31] {
    { "slider020", "slider021", "slider022", "slider023", "slider024", "slider029",
      "slider116", "slider117", "slider026", "slider025", "slider027", "slider028",
      "slider019", "slider092", "slider094", "slider096", "slider098", "slider108",
      "slider139", "slider132", "slider133", "slider134", "slider135", "slider136",
      "slider137", "slider156", "slider157", "slider158", "slider159", "slider160", "slider161" },
    { "slider221", "slider222", "slider223", "slider224", "slider225", "slider230",
      "slider232", "slider233", "slider227", "slider226", "slider228", "slider229",
      "slider220", "slider093", "slider095", "slider097", "slider099", "slider231",
      "slider240", "slider234", "slider235", "slider236", "slider237", "slider238",
      "slider239", "slider241", "slider242", "slider243", "slider244", "slider245", "slider246" }
};

constexpr float factory[9][31] {
    { 20.2f,.8f,.014f,.05f,.6f,.25f,0,0,.94f,.03f,.66f,.55f,.2f,0,.1f,.2f,1,50,.7f,120,.1f,0,0,0,.5f,-18,1,0,8,90,10 },
    { 12.45f,1.1f,.04f,-1,.52f,.37f,.3f,1,1.25f,.06f,.75f,.55f,.35f,0,.5f,.32f,1,7,.25f,500,.2f,1,.62f,.04f,.5f,-18,1,0,8,90,0 },
    { 43,1,.25f,.05f,.5f,.5f,.5f,.4f,.1f,.06f,.1f,.4f,6.9f,.4f,.5f,.05f,.5f,500,.5f,100,0,1,.35f,.2f,.31f,-18,1,14,4,20,80 },
    { 40,1.5f,.11f,-1.3f,1,.79f,.8f,0,1,0,0,1,1.6f,.003f,1,.7f,.65f,0,.5f,1,1,1,0,.08f,.55f,-18,1,7,8,90,40 },
    { 50,1.5f,.4f,-2,1,.8f,.6f,1,.8f,1,.74f,.4f,1.6f,.5f,1,1,0,100,.5f,80,-.95f,0,0,.08f,.51f,-18,1,7,8,90,50 },
    { 30.5f,1.5f,.4f,-1,1,.8f,.6f,1,.8f,1,.74f,.4f,1.6f,.5f,1,1,0,100,.5f,80,-.8f,0,0,.08f,.51f,-18,1,7,8,90,22 },
    { 19.4f,2,.12f,-1.3f,1,0,.03f,.1f,.5f,.85f,1,.48f,.14f,.1f,1,0,.6f,180,.46f,80,-.95f,0,0,0,.1f,-18,1,7,8,90,50 },
    { 50,1.5f,.6f,-2.4f,.9f,.8f,.6f,1,.7f,.7f,.7f,.4f,1.6f,.5f,1,1,0,100,.5f,80,-.95f,0,0,.08f,.51f,-18,1,7,8,90,50 },
    { 18,1.6f,.06f,0,.95f,.56f,0,1,1.61f,0,.34f,.34f,2.2f,.56f,.54f,.18f,.74f,55,.28f,120,0,0,0,0,.52f,-18,1,0,8,90,0 }
};
}

SnareEngineBanks::SnareEngineBanks (juce::AudioProcessorValueTreeState& valueTree) : state (valueTree)
{
    resetFromCurrentState();
}

void SnareEngineBanks::fillFactory()
{
    for (int slot = 0; slot < slotCount; ++slot)
        for (int engine = 0; engine < engineCount; ++engine)
            for (int parameter = 0; parameter < parameterCount; ++parameter)
                banks[slot][engine][parameter] = factory[engine][parameter];
}

void SnareEngineBanks::resetFromCurrentState()
{
    fillFactory();
    for (int slot = 0; slot < slotCount; ++slot)
    {
        const auto* selector = slot == 0 ? "slider248" : "slider198";
        currentEngine[slot] = juce::jlimit (0, engineCount - 1,
            juce::roundToInt (state.getRawParameterValue (selector)->load()));
        captureCurrent (slot);
    }
}

bool SnareEngineBanks::ownsParameter (int slot, juce::StringRef id)
{
    if (slot < 0 || slot >= slotCount) return false;
    for (const auto* candidate : ids[slot])
        if (id == juce::StringRef (candidate)) return true;
    return false;
}

void SnareEngineBanks::captureCurrent (int slot)
{
    if (applying || slot < 0 || slot >= slotCount) return;
    for (int parameter = 0; parameter < parameterCount; ++parameter)
        banks[slot][currentEngine[slot]][parameter] = state.getRawParameterValue (ids[slot][parameter])->load();
}

void SnareEngineBanks::load (int slot, int engine)
{
    const juce::ScopedValueSetter guard (applying, true);
    for (int parameter = 0; parameter < parameterCount; ++parameter)
        if (auto* target = state.getParameter (ids[slot][parameter]))
        {
            target->beginChangeGesture();
            target->setValueNotifyingHost (target->convertTo0to1 (banks[slot][engine][parameter]));
            target->endChangeGesture();
        }
}

bool SnareEngineBanks::switchTo (int slot, int engine)
{
    if (slot < 0 || slot >= slotCount || applying) return false;
    engine = juce::jlimit (0, engineCount - 1, engine);
    if (engine == currentEngine[slot]) return false;
    captureCurrent (slot);
    currentEngine[slot] = engine;
    load (slot, engine);
    return true;
}
}
