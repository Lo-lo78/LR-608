// SPDX-License-Identifier: AGPL-3.0-or-later
#include "HatCymbalEngineBanks.h"
namespace lr608 { namespace {
constexpr const char* hatIds[]{"slider005","slider001","slider002","slider003","slider004","slider114","slider218","slider115","slider090","slider091"};
constexpr float hatFactory[7][10]{{.72f,.038f,1,0,.72f,0,.5f,.5f,0,80},{.68f,.036f,1,.03f,.7f,.1f,.62f,.5f,.1f,32},{1.02f,.038f,1,.13f,.7f,0,.26f,.5f,0,80},{.72f,.038f,1,0,.72f,0,.5f,.5f,0,80},{1,.105f,1.5f,.65f,1,.1f,0,.541f,.3f,80},{1,.055f,1.5f,.5f,.72f,0,.42f,.5f,0,80},{1,.102f,1.5f,.6f,.72f,0,0,.43f,.5f,80}};
constexpr const char* cymIds[]{"slider009","slider010","slider006","slider007","slider008","slider112","slider113"};
constexpr float cymFactory[4][7]{{3.5f,4,.3f,.1f,1,0,0},{1.4f,2,.7f,.8f,.55f,.48f,.35f},{2,3,1,.1f,.55f,.48f,.35f},{1.5f,2.4f,1,.1f,1,0,0}};
template<size_t N>bool owns(const char*const(&a)[N],juce::StringRef id){for(auto*x:a)if(id==juce::StringRef(x))return true;return false;}
}
HatCymbalEngineBanks::HatCymbalEngineBanks(juce::AudioProcessorValueTreeState&s):state(s){resetToFactory();}
void HatCymbalEngineBanks::resetFromCurrentState(){for(int e=0;e<7;++e)for(int p=0;p<10;++p)hats[e][p]=hatFactory[e][p];for(int e=0;e<4;++e)for(int p=0;p<7;++p)cymbals[e][p]=cymFactory[e][p];hatEngine=juce::jlimit(0,6,juce::roundToInt(state.getRawParameterValue("slider217")->load()));cymbalEngine=juce::jlimit(0,3,juce::roundToInt(state.getRawParameterValue("slider214")->load()));captureHat();captureCymbal();}
void HatCymbalEngineBanks::resetToFactory(){for(int e=0;e<7;++e)for(int p=0;p<10;++p)hats[e][p]=hatFactory[e][p];for(int e=0;e<4;++e)for(int p=0;p<7;++p)cymbals[e][p]=cymFactory[e][p];hatEngine=juce::jlimit(0,6,juce::roundToInt(state.getRawParameterValue("slider217")->load()));cymbalEngine=juce::jlimit(0,3,juce::roundToInt(state.getRawParameterValue("slider214")->load()));loadHat(hatEngine);loadCymbal(cymbalEngine);}
bool HatCymbalEngineBanks::ownsHat(juce::StringRef id){return owns(hatIds,id);}bool HatCymbalEngineBanks::ownsCymbal(juce::StringRef id){return owns(cymIds,id);}
void HatCymbalEngineBanks::captureHat(){if(applying)return;for(int p=0;p<10;++p)hats[hatEngine][p]=state.getRawParameterValue(hatIds[p])->load();}void HatCymbalEngineBanks::captureCymbal(){if(applying)return;for(int p=0;p<7;++p)cymbals[cymbalEngine][p]=state.getRawParameterValue(cymIds[p])->load();}
void HatCymbalEngineBanks::loadHat(int e){const juce::ScopedValueSetter g(applying,true);for(int p=0;p<10;++p)if(auto*x=state.getParameter(hatIds[p]))x->setValueNotifyingHost(x->convertTo0to1(hats[e][p]));}void HatCymbalEngineBanks::loadCymbal(int e){const juce::ScopedValueSetter g(applying,true);for(int p=0;p<7;++p)if(auto*x=state.getParameter(cymIds[p]))x->setValueNotifyingHost(x->convertTo0to1(cymbals[e][p]));}
bool HatCymbalEngineBanks::switchHat(int e){e=juce::jlimit(0,6,e);if(e==hatEngine||applying)return false;captureHat();hatEngine=e;loadHat(e);return true;}bool HatCymbalEngineBanks::switchCymbal(int e){e=juce::jlimit(0,3,e);if(e==cymbalEngine||applying)return false;captureCymbal();cymbalEngine=e;loadCymbal(e);return true;}
}
