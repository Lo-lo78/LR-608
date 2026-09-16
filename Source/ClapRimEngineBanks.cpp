// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ClapRimEngineBanks.h"
namespace lr608 { namespace {
constexpr const char* clapIds[]{"slider030","slider031","slider032","slider033","slider034","slider208","slider035","slider036","slider037","slider038","slider039"};
constexpr float clapFactory[7][11]{{4,.12f,.5f,1,1.21f,.15f,0,.4f,.9f,1,.4f},{4,.07f,.62f,.8f,.16f,.34f,.38f,.18f,.72f,.34f,.34f},{5,.02f,.78f,.7f,.35f,.52f,.78f,.3f,.9f,.6f,.3f},{2.4f,.165f,.5f,1,0,0,.5f,.5f,.5f,.5f,0},{2.9f,.165f,.5f,1,0,0,.5f,.5f,.5f,.5f,0},{4,.165f,.5f,1,0,0,.5f,.5f,.5f,.5f,0},{3.35f,.02f,.92f,1,1.66f,.70f,2,1,1,1,.18f}};
constexpr const char* rimIds[]{"slider071","slider070","slider069","slider068","slider124","slider125","slider143","slider144","slider145","slider146","slider147","slider252","slider162","slider163","slider164","slider165","slider166","slider167"};
constexpr float rimFactory[6][18]{{7,.004f,0,2,0,1.1f,.05f,0,.5f,.7f,.35f,0,-16,1,0,3,60,50},{4,.5f,.5f,0,1.7f,2.323f,.05f,0,.5f,.7f,.35f,0,-16,1,0,3,60,50},{0,.5f,.5f,0,5,2.629f,.05f,0,.5f,.7f,.35f,0,-16,1,0,3,60,50},{6,.5f,.5f,0,.5f,2.736f,.05f,0,.5f,.7f,.35f,0,-16,1,0,3,60,50},{8.4f,.14f,-.08f,1.83f,1.15f,1.72f,0,.05f,.42f,.7f,.24f,0,-16,1,0,3,60,0},{10.29f,.16f,.87f,2.373333333333333f,4.56f,4,.08f,.05f,.48f,.7f,.35f,0,-16,1,0,3,60,0}};
template<size_t N> bool owns(const char* const(&a)[N],juce::StringRef id){for(auto* x:a)if(id==juce::StringRef(x))return true;return false;}
}
ClapRimEngineBanks::ClapRimEngineBanks(juce::AudioProcessorValueTreeState& s):state(s){resetFromCurrentState();}
void ClapRimEngineBanks::resetFromCurrentState(){for(int e=0;e<7;++e)for(int p=0;p<11;++p)clap[e][p]=clapFactory[e][p];for(int e=0;e<6;++e)for(int p=0;p<18;++p)rim[e][p]=rimFactory[e][p];clapEngine=juce::jlimit(0,6,juce::roundToInt(state.getRawParameterValue("slider207")->load()));rimEngine=juce::jlimit(0,5,juce::roundToInt(state.getRawParameterValue("slider209")->load()));captureClap();captureRim();}
bool ClapRimEngineBanks::ownsClap(juce::StringRef id){return owns(clapIds,id);} bool ClapRimEngineBanks::ownsRim(juce::StringRef id){return owns(rimIds,id);}
void ClapRimEngineBanks::captureClap(){if(applying)return;for(int p=0;p<11;++p)clap[clapEngine][p]=state.getRawParameterValue(clapIds[p])->load();}
void ClapRimEngineBanks::captureRim(){if(applying)return;for(int p=0;p<18;++p)rim[rimEngine][p]=state.getRawParameterValue(rimIds[p])->load();}
void ClapRimEngineBanks::loadClap(int e){const juce::ScopedValueSetter g(applying,true);for(int p=0;p<11;++p)if(auto* x=state.getParameter(clapIds[p]))x->setValueNotifyingHost(x->convertTo0to1(clap[e][p]));}
void ClapRimEngineBanks::loadRim(int e){const juce::ScopedValueSetter g(applying,true);for(int p=0;p<18;++p)if(auto* x=state.getParameter(rimIds[p]))x->setValueNotifyingHost(x->convertTo0to1(rim[e][p]));}
bool ClapRimEngineBanks::switchClap(int e){e=juce::jlimit(0,6,e);if(e==clapEngine||applying)return false;captureClap();clapEngine=e;loadClap(e);return true;}
bool ClapRimEngineBanks::switchRim(int e){e=juce::jlimit(0,5,e);if(e==rimEngine||applying)return false;captureRim();rimEngine=e;loadRim(e);return true;}
}
