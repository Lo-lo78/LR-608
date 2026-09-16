// SPDX-License-Identifier: AGPL-3.0-or-later
#include "MaracasEngineBanks.h"
namespace lr608 { namespace {
constexpr const char* ids[]{"slider168","slider169","slider170","slider171","slider172","slider173","slider205","slider206"};
constexpr float factory[4][8]{
 {5,.003f,.005f,.45f,180,.55f,.3f,.2f},
 {6,.01f,.18f,.5f,180,.55f,.2f,.2f},
 {4,.01f,.18f,.5f,180,.55f,.2f,.2f},
 {5,.01f,.18f,.5f,180,.55f,.2f,.2f}
}; }
MaracasEngineBanks::MaracasEngineBanks(juce::AudioProcessorValueTreeState&s):state(s){resetToFactory();}
void MaracasEngineBanks::resetFromCurrentState(){for(int e=0;e<4;++e)for(int p=0;p<8;++p)banks[e][p]=factory[e][p];currentEngine=juce::jlimit(0,3,juce::roundToInt(state.getRawParameterValue("slider216")->load()));captureCurrent();}
void MaracasEngineBanks::resetToFactory(){for(int e=0;e<4;++e)for(int p=0;p<8;++p)banks[e][p]=factory[e][p];currentEngine=juce::jlimit(0,3,juce::roundToInt(state.getRawParameterValue("slider216")->load()));load(currentEngine);}
bool MaracasEngineBanks::ownsParameter(juce::StringRef id){for(auto*x:ids)if(id==juce::StringRef(x))return true;return false;}
void MaracasEngineBanks::captureCurrent(){if(applying)return;for(int p=0;p<8;++p)banks[currentEngine][p]=state.getRawParameterValue(ids[p])->load();}
void MaracasEngineBanks::load(int e){const juce::ScopedValueSetter guard(applying,true);for(int p=0;p<8;++p)if(auto*x=state.getParameter(ids[p]))x->setValueNotifyingHost(x->convertTo0to1(banks[e][p]));}
bool MaracasEngineBanks::switchTo(int e){e=juce::jlimit(0,3,e);if(e==currentEngine||applying)return false;captureCurrent();currentEngine=e;load(e);return true;}
}
