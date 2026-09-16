// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SnareVoice.h"
#include "CowbellVoice.h"
#include "PolyphonicVoice.h"
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>

namespace
{
constexpr int sampleRate = 48000;
constexpr int renderSamples = sampleRate * 8;
constexpr int retriggerSamples = 2400;
constexpr int voiceCount = 32;

template <typename Voice, typename Parameters, typename Trigger, typename Render>
double measure (const char* name, Parameters parameters, Trigger trigger, Render render)
{
    std::array<Voice, voiceCount> voices;
    for (auto& voice : voices) voice.prepare (sampleRate);
    double checksum = 0.0;
    int nextVoice = 0;
    const auto begin = std::chrono::steady_clock::now();
    for (int sample = 0; sample < renderSamples; ++sample)
    {
        if (sample % retriggerSamples == 0)
        {
            trigger (voices[std::size_t(nextVoice)], parameters);
            nextVoice = (nextVoice + 1) % voiceCount;
        }
        for (auto& voice : voices)
            if (voice.isActive()) checksum += render (voice, parameters);
    }
    const auto milliseconds = std::chrono::duration<double, std::milli>
        (std::chrono::steady_clock::now() - begin).count();
    std::cout << name << '=' << milliseconds << " ms checksum=" << checksum << '\n';
    return milliseconds;
}
}

int main()
{
    std::cout << "PolyphonicVoice bytes=" << sizeof(lr608::PolyphonicVoice) << '\n';
    lr608::SnareParameters snare;
    constexpr double acoustic[]{43,1,.25,.05,.5,.5,.5,.4,.1,.06,.1,.4,6.9,.4,.5,.05,.5,500,.5,100,0,1,.35,.2,.31,-18,1,14,4,20,80};
    std::copy(std::begin(acoustic),std::end(acoustic),snare.v.begin());
    measure<lr608::SnareVoice>("Snare Acoustic",snare,
        [](auto& voice,const auto& p){voice.trigger(2,127,p);},
        [](auto& voice,const auto& p){return double(voice.render(p));});

    lr608::CowbellParameters timbal;
    constexpr double values[]{5,.8,2.75,1,4.5,5,.7,.3,3.5,.18,.16};
    std::copy(std::begin(values),std::end(values),timbal.v.begin());
    measure<lr608::CowbellVoice>("Timbales Physical",timbal,
        [](auto& voice,const auto& p){voice.trigger(5,127,p);},
        [](auto& voice,const auto& p){return voice.render(p);});
    measure<lr608::CowbellVoice>("Timbales Wave Mesh",timbal,
        [](auto& voice,const auto& p){voice.trigger(6,127,p);},
        [](auto& voice,const auto& p){return voice.render(p);});
}
