// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ZapEngineBanks.h"
namespace lr608 { namespace {
constexpr const char* ids[]{"slider072","slider073","slider074","slider075","slider076","slider077","slider126","slider127","slider128","slider129","slider130","slider131","slider210","slider211","slider212","slider213","slider192","slider193","slider194","slider195","slider196","slider197"};
constexpr float factory[11][22]{
 {1.6f,.02f,1.5f,10,-.06f,3,128,.1f,3,0,.2f,.5f,0,180,0,0,-16,1,0,2,70,0},
 {1.5f,.05f,.7f,6.5f,.8f,.8f,64,4,5,.15f,0,.32f,.25f,920,4,.8f,-16,1,2,2,70,20},
 {1.3f,.08f,.75f,4,.2f,.4f,256,2,5,.7f,0,.58f,.15f,640,5,.35f,-18,1,3,3,120,18},
 {2,.12f,.9f,7,-.4f,.8f,8,3,0,.4f,0,.5f,.35f,480,0,.6f,-18,1,4,4,160,25},
 {1.4f,.32f,.5f,10,-1,2,32,1,5,.2f,.06f,.81f,.2f,350,4,.25f,-18,1,2,2,120,18},
 {2,.2f,1.2f,8.5f,-.2f,.3f,4,8,3,.05f,.2f,.7f,.4f,1200,0,.9f,-20,2,3,1,180,35},
 {1.4f,.04f,.65f,5,1.2f,.7f,512,5,5,.25f,0,.65f,.3f,2400,5,.5f,-18,1,2,1,80,22},
 {1.8f,.06f,.8f,9,0,1.2f,128,12,4,.02f,0,.23f,.5f,666,4,1,-22,3,5,1,140,40},
 {2,.08f,1.55f,7.5f,-.3f,1.7f,8,.35f,5,.7f,.1f,.46f,.15f,220,0,.35f,-18,1,3,4,180,22},
 {1.2f,.06f,1.25f,7.2f,.15f,1.55f,32,8,0,.18f,.2f,.58f,.08f,410,0,.25f,-18,1,3,2,90,20},
 {1.4f,.03f,2,2.5f,.5f,1.5f,6,1,1,.2f,0,.42f,.2f,350,4,.25f,-18,1,2,2,120,18}
}; }
ZapEngineBanks::ZapEngineBanks(juce::AudioProcessorValueTreeState&s):state(s){resetToFactory();}
void ZapEngineBanks::resetFromCurrentState(){for(int e=0;e<11;++e)for(int p=0;p<22;++p)banks[e][p]=factory[e][p];currentEngine=juce::jlimit(0,10,juce::roundToInt(state.getRawParameterValue("slider199")->load()));captureCurrent();}
void ZapEngineBanks::resetToFactory(){for(int e=0;e<11;++e)for(int p=0;p<22;++p)banks[e][p]=factory[e][p];currentEngine=juce::jlimit(0,10,juce::roundToInt(state.getRawParameterValue("slider199")->load()));load(currentEngine);}
bool ZapEngineBanks::ownsParameter(juce::StringRef id){for(auto*x:ids)if(id==juce::StringRef(x))return true;return false;}
void ZapEngineBanks::captureCurrent(){if(applying)return;for(int p=0;p<22;++p)banks[currentEngine][p]=state.getRawParameterValue(ids[p])->load();}
void ZapEngineBanks::load(int e){const juce::ScopedValueSetter g(applying,true);for(int p=0;p<22;++p)if(auto*x=state.getParameter(ids[p]))x->setValueNotifyingHost(x->convertTo0to1(banks[e][p]));}
bool ZapEngineBanks::switchTo(int e){e=juce::jlimit(0,10,e);if(e==currentEngine||applying)return false;captureCurrent();currentEngine=e;load(e);return true;}
}
