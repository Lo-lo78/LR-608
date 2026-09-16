// SPDX-License-Identifier: AGPL-3.0-or-later
#include "TomEngineBanks.h"

namespace lr608 { namespace {
constexpr const char* ids[] {
 "slider064","slider065","slider066","slider040","slider048","slider056",
 "slider041","slider049","slider057","slider042","slider050","slider058",
 "slider043","slider051","slider059","slider044","slider052","slider060",
 "slider045","slider053","slider061","slider109","slider110","slider111",
 "slider118","slider119","slider120","slider046","slider054","slider062",
 "slider047","slider055","slider063","slider121","slider122","slider123",
 "slider174","slider175","slider176","slider177","slider178","slider179",
 "slider180","slider181","slider182","slider183","slider184","slider185",
 "slider186","slider187","slider188","slider189","slider190","slider191" };
constexpr float factory[5][54] {
 {7,7,7,-.5f,0,.5f,.57f,.65f,.82f,0,.06f,0,.041f,.041f,.016f,.3f,.16f,.15f,.1f,.1f,.1f,0,0,0,0,0,0,.3f,.3f,.3f,.15f,.15f,.15f,.4f,.4f,.4f,-18,1,0,6,100,0,-18,1,0,6,100,0,-18,1,0,5,90,0},
 {13,8.4f,10,-.5f,0,.5f,1,1,1,0,-.06f,-.38f,.12f,.09f,.15f,.51f,.35f,.32f,.04f,.03f,.025f,.6f,.7f,.4f,100,100,100,1.3f,1.3f,1.3f,1,1,1,.75f,.76f,.9f,-18,1,7,6,100,50,-18,1,7,6,100,50,-18,1,7,6,100,50},
 {4.2f,4.4f,4.3f,-.5f,0,.5f,.45f,.65f,1,.92f,1,2,.028f,.1f,.01f,.55f,.3f,.25f,.1f,.1f,.1f,.5f,.5f,.5f,0,0,0,.2f,.2f,.2f,.4f,.4f,.4f,.5f,.5f,.5f,-18,1,0,6,100,0,-18,1,0,6,100,0,-18,1,0,5,90,0},
 {2.7f,2.6f,2.5f,-.5f,0,.5f,1,1,1,0,.08f,2,.15f,.15f,.15f,.2f,.2f,.2f,.42f,.4f,.36f,.5f,.5f,.5f,0,0,0,.2f,.2f,.2f,.4f,.4f,.4f,.5f,.5f,.5f,-18,1,0,6,100,0,-18,1,0,6,100,0,-18,1,0,5,90,0},
 {3.8f,3.7f,3.6f,-.5f,0,.5f,1,1,1,-.16f,-.04f,1.08f,.15f,.15f,.15f,.2f,.2f,.2f,.48f,.45f,.4f,.5f,.5f,.5f,0,0,0,.2f,.2f,.2f,.4f,.4f,.4f,.5f,.5f,.5f,-18,1,0,6,100,0,-18,1,0,6,100,0,-18,1,0,5,90,0}
};
}
TomEngineBanks::TomEngineBanks (juce::AudioProcessorValueTreeState& s) : state (s) { resetToFactory(); }
void TomEngineBanks::resetFromCurrentState() { for(int e=0;e<5;++e) for(int p=0;p<54;++p) banks[e][p]=factory[e][p]; currentEngine=juce::jlimit(0,4,juce::roundToInt(state.getRawParameterValue("slider219")->load())); captureCurrent(); }
void TomEngineBanks::resetToFactory() { for(int e=0;e<5;++e) for(int p=0;p<54;++p) banks[e][p]=factory[e][p]; currentEngine=juce::jlimit(0,4,juce::roundToInt(state.getRawParameterValue("slider219")->load())); load(currentEngine); }
bool TomEngineBanks::ownsParameter (juce::StringRef id) { for(auto* x:ids) if(id==juce::StringRef(x)) return true; return false; }
void TomEngineBanks::captureCurrent() { if(applying)return; for(int p=0;p<54;++p) banks[currentEngine][p]=state.getRawParameterValue(ids[p])->load(); }
void TomEngineBanks::load(int e) { const juce::ScopedValueSetter guard(applying,true); for(int p=0;p<54;++p) if(auto* x=state.getParameter(ids[p])) x->setValueNotifyingHost(x->convertTo0to1(banks[e][p])); }
bool TomEngineBanks::switchTo(int e) { e=juce::jlimit(0,4,e); if(e==currentEngine||applying)return false; captureCurrent(); currentEngine=e; load(e); return true; }
}
