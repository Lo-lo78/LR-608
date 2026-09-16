// SPDX-License-Identifier: AGPL-3.0-or-later
#include "DrumTimingEngine.h"

#include <algorithm>
#include <climits>
#include <cmath>

namespace lr608
{
void DrumTimingEngine::reset()
{
    held.fill (false); pending.fill (false); velocities.fill (0); due.fill (never);
    inputChannels.fill(0);inputSuppressed.fill(false);midiChannels.fill(0);midiActive.fill(false);midiOffDue.fill(INT64_MAX);midiNextOffDue=INT64_MAX;
    rollClock = dragClock = 0;
    gridActive = false; gridPhase = 0; gridDue = never;
    pitchBend = 0; dragOn = false;
    dragSnare1 = {}; dragSnare2 = {}; dragCowbell = {};
    userDivision = std::clamp (settings.rollDivision, 0, 16);
    effectiveDivision = userDivision;
    rollEnabled = effectiveDivision > 0;
}

void DrumTimingEngine::setSettings (const TimingSettings& next)
{
    const auto oldDivision = effectiveDivision;
    settings = next;
    settings.sampleRate = std::max (1.0, settings.sampleRate);
    settings.tempo = std::max (1.0, settings.tempo);
    userDivision = std::clamp (settings.rollDivision, 0, 16);
    effectiveDivision = pitchBendDivision (userDivision, pitchBend);
    dragFixedGap = std::max (1, static_cast<int> (std::floor (0.004 * settings.sampleRate + 0.5)));
    if (effectiveDivision != oldDivision || rollEnabled != (effectiveDivision > 0))
        rescheduleAll();
    if (rollEnabled && settings.hostTimelineValid && settings.hostPlaying)
        syncToHostGrid();
}

void DrumTimingEngine::syncToHostGrid()
{
    const auto hits = hitsPerBar (effectiveDivision);
    if (hits <= 0) return;
    const auto numerator = settings.timeSignatureNumerator > 0 ? settings.timeSignatureNumerator : 4;
    const auto denominator = settings.timeSignatureDenominator > 0 ? settings.timeSignatureDenominator : 4;
    const auto barQuarterNotes = numerator * 4.0 / denominator;
    const auto step = barQuarterNotes / hits;
    const auto swingDelay = step * std::clamp (settings.shufflePercent / 100.0, 0.0, 1.0) / 3.0;
    const auto positionInBar = std::max (0.0, settings.hostPpqPosition - settings.hostBarStartPpq);
    auto pointIndex = std::max<std::int64_t> (0, static_cast<std::int64_t> (std::floor (positionInBar / step)));
    auto pointPpq = settings.hostBarStartPpq + pointIndex * step + ((pointIndex & 1) != 0 ? swingDelay : 0.0);
    const auto ppqPerSample = settings.tempo / (60.0 * settings.sampleRate);
    const auto ppqTolerance = ppqPerSample * 0.51;
    while (pointPpq < settings.hostPpqPosition - ppqTolerance)
    {
        ++pointIndex;
        pointPpq = settings.hostBarStartPpq + pointIndex * step + ((pointIndex & 1) != 0 ? swingDelay : 0.0);
    }
    const auto samplesUntilPoint = std::max (0.0, (pointPpq - settings.hostPpqPosition) / std::max (1.0e-12, ppqPerSample));
    gridActive = true;
    gridPhase = int (pointIndex & 1);
    gridDue = static_cast<double> (rollClock) + samplesUntilPoint + 1.0;
    for (int note = 0; note < 128; ++note)
        if (held[note] || pending[note]) due[note] = gridDue;
}

bool DrumTimingEngine::isSupportedNote (int n)
{
    return n >= 0 && n < 128;
}

int DrumTimingEngine::hitsPerBar (int division)
{
    static constexpr int values[] { 0, 4, 6, 8, 12, 16, 24, 32, 48, 64,
                                    96, 128, 192, 256, 384, 512, 768 };
    return values[std::clamp (division, 0, 16)];
}

int DrumTimingEngine::pitchBendDivision (int base, int pb)
{
    base = std::clamp (base, 0, 16);
    if (base == 0 || std::abs (pb) <= 384)
        return base;
    const auto direction = pb > 0 ? 1 : -1;
    const auto available = direction > 0 ? 16 - base : base - 1;
    if (available <= 0)
        return base;
    const auto norm = std::clamp ((std::abs (pb) - 384) / double (8191 - 384), 0.0, 1.0);
    const auto steps = std::clamp (static_cast<int> (std::ceil (norm * available)), 1, available);
    return std::clamp (base + direction * steps, 1, 16);
}

double DrumTimingEngine::gapSamples (int nextPhase) const
{
    const auto hits = hitsPerBar (effectiveDivision);
    if (hits <= 0) return never;
    const auto numerator = settings.timeSignatureNumerator > 0 ? settings.timeSignatureNumerator : 4;
    const auto denominator = settings.timeSignatureDenominator > 0 ? settings.timeSignatureDenominator : 4;
    const auto barBeats = numerator * 4.0 / denominator;
    const auto base = settings.sampleRate * (60.0 / settings.tempo) * barBeats / hits;
    const auto delay = base * std::clamp (settings.shufflePercent / 100.0, 0.0, 1.0) / 3.0;
    return std::max (1.0, nextPhase != 0 ? base + delay : base - delay);
}

bool DrumTimingEngine::anyHeld() const
{
    return std::any_of (held.begin(), held.end(), [] (bool value) { return value; });
}

bool DrumTimingEngine::anyPending() const
{
    return std::any_of (pending.begin(), pending.end(), [] (bool value) { return value; });
}

void DrumTimingEngine::findJoinPoint (double eventClock, double& resultDue, int& resultPhase) const
{
    resultDue = gridDue; resultPhase = gridPhase;
    for (int guard = 0; resultDue + 0.5 < eventClock && guard < 2048; ++guard)
    {
        resultPhase = 1 - resultPhase;
        resultDue += gapSamples (resultPhase);
    }
}

void DrumTimingEngine::press (int note, int velocity, int channel)
{
    held[note] = true; velocities[note] = velocity; inputChannels[note]=std::clamp(channel,0,15); pending[note] = true;
    if (! rollEnabled)
    {
        pending[note] = false; due[note] = never;
        return;
    }
    if (! gridActive)
    {
        gridActive = true; gridPhase = 0; gridDue = static_cast<double> (rollClock);
        due[note] = gridDue;
    }
    else
    {
        int phase = 0;
        findJoinPoint (static_cast<double> (rollClock), due[note], phase);
    }
}

void DrumTimingEngine::release (int note)
{
    held[note] = false;
    if (! pending[note]) due[note] = never;
}

void DrumTimingEngine::rescheduleAll()
{
    rollEnabled = hitsPerBar (effectiveDivision) > 0;
    if (! rollEnabled)
    {
        gridActive = false; gridPhase = 0; gridDue = never;
        pending.fill (false); due.fill (never);
    }
    else if (! gridActive && anyHeld())
    {
        gridActive = true; gridPhase = 0; gridDue = static_cast<double> (rollClock);
        for (int note = 0; note < 128; ++note)
            if (held[note]) { pending[note] = true; due[note] = gridDue; }
    }
}

bool DrumTimingEngine::dragIsSupported (int note) const
{
    return note == 38 || note == 40 || (note == 56 && settings.cowbellEngine >= 5);
}

void DrumTimingEngine::armDrag (int note, int velocity, std::int64_t firstDue, int gap, int channel, bool emitMidi)
{
    auto* state = note == 38 ? &dragSnare1 : note == 40 ? &dragSnare2 : &dragCowbell;
    state->velocity = velocity; state->due = firstDue; state->gap = std::max (1, gap); state->channel=std::clamp(channel,0,15);state->emitMidi=emitMidi;state->stage = 1;
}

bool DrumTimingEngine::handleMidi (std::uint8_t statusByte, std::uint8_t data1,
                                   std::uint8_t data2, const TriggerCallback& trigger,const MidiCallback&)
{
    const auto status = statusByte & 0xf0;
    const auto channel=statusByte&0x0f;
    if (status == 0xb0 && data1 == 1) dragOn = data2 > 0;
    if (status == 0xe0)
    {
        pitchBend = data1 + data2 * 128 - 8192;
        const auto target = pitchBendDivision (userDivision, pitchBend);
        if (target != effectiveDivision) { effectiveDivision = target; rescheduleAll(); }
    }
    const auto noteOn = status == 0x90 && data2 > 0;
    const auto noteOff = status == 0x80 || (status == 0x90 && data2 == 0);
    bool consumed=false;
    if(noteOn&&isSupportedNote(data1)&&rollEnabled){inputSuppressed[data1]=true;consumed=true;}
    if(noteOff&&isSupportedNote(data1)&&inputSuppressed[data1]){inputSuppressed[data1]=false;consumed=true;}
    if (noteOn)
    {
        if (isSupportedNote (data1) && rollEnabled)
            press (data1, data2, channel);
        else
        {
            if (isSupportedNote (data1)) press (data1, data2, channel);
            if (dragOn && dragIsSupported (data1)) armDrag (data1, data2, dragClock + 1, dragFixedGap,channel,false);
            else trigger ({ data1, data2 });
        }
    }
    if (noteOff && isSupportedNote (data1)) release (data1);
    return consumed;
}

void DrumTimingEngine::tickDrag (int note, DragState& state, const TriggerCallback& trigger,const MidiCallback&midi)
{
    if (state.stage <= 0 || dragClock < state.due) return;
    if (state.stage == 1)
    {
        const auto v=std::max (1, static_cast<int> (std::floor (state.velocity * 0.35 + 0.5)));trigger ({ note,v});if(state.emitMidi)midiHit(note,v,state.channel,std::max(1,int(std::floor(state.gap*.45+.5))),midi);
        state.stage = 2; state.due += state.gap;
    }
    else if (state.stage == 2)
    {
        const auto v=std::max (1, static_cast<int> (std::floor (state.velocity * 0.55 + 0.5)));trigger ({ note,v});if(state.emitMidi)midiHit(note,v,state.channel,std::max(1,int(std::floor(state.gap*.45+.5))),midi);
        state.stage = 3; state.due += state.gap;
    }
    else { trigger ({ note, state.velocity });if(state.emitMidi)midiHit(note,state.velocity,state.channel,std::max(1,int(std::floor(state.gap*.45+.5))),midi); state.stage = 0;state.emitMidi=false; }
}

void DrumTimingEngine::tick (const TriggerCallback& trigger,const MidiCallback&midi)
{
    ++dragClock;
    flushMidiDue(midi);
    if (rollEnabled)
    {
        ++rollClock;
        if (gridActive && rollClock + 0.5 >= gridDue)
        {
            for (int guard = 0; rollClock + 0.5 >= gridDue && guard < 16; ++guard)
            {
                for (int note = 0; note < 128; ++note)
                {
                    if ((held[note] || pending[note]) && due[note] <= gridDue + 0.5)
                    {
                        auto velocity = velocities[note];
                        if (gridPhase != 0)
                            velocity = std::max (1, static_cast<int> (std::floor (velocity *
                                (1.0 - std::clamp (settings.secondHitReductionPercent / 100.0, 0.0, 1.0)) + 0.5)));
                        if (dragOn && dragIsSupported (note))
                        {
                            const auto nextGap = gapSamples (1 - gridPhase);
                            const auto gap = std::min (dragFixedGap,
                                std::max (1, static_cast<int> (std::floor ((nextGap - 2.0) / 3.0))));
                            armDrag (note, velocity, dragClock, gap,inputChannels[note],true);
                        }
                        else{trigger ({ note, velocity });const auto gate=std::min(std::max(1,int(std::floor(.020*settings.sampleRate+.5))),std::max(1,int(std::floor(gapSamples(1-gridPhase)*.45+.5))));midiHit(note,velocity,inputChannels[note],gate,midi);}
                        pending[note] = false;
                        if (! held[note]) due[note] = never;
                    }
                }
                const auto nextPhase = 1 - gridPhase;
                gridDue += gapSamples (nextPhase); gridPhase = nextPhase;
            }
        }
    }
    tickDrag (38, dragSnare1, trigger,midi);
    tickDrag (40, dragSnare2, trigger,midi);
    tickDrag (56, dragCowbell, trigger,midi);
}

bool DrumTimingEngine::anyMidiActive()const noexcept{return std::any_of(midiActive.begin(),midiActive.end(),[](bool x){return x;});}
void DrumTimingEngine::midiHit(int note,int velocity,int channel,int gate,const MidiCallback&midi){if(!midi)return;note=std::clamp(note,0,127);channel=std::clamp(channel,0,15);velocity=std::clamp(velocity,1,127);if(midiActive[note])midi({midiChannels[note],note,0,false});midi({channel,note,velocity,true});midiActive[note]=true;midiChannels[note]=channel;midiOffDue[note]=dragClock+std::max(1,gate);midiNextOffDue=std::min(midiNextOffDue,midiOffDue[note]);}
void DrumTimingEngine::flushMidiDue(const MidiCallback&midi){if(dragClock<midiNextOffDue)return;midiNextOffDue=INT64_MAX;for(int note=0;note<128;++note)if(midiActive[note]){if(dragClock>=midiOffDue[note]){if(midi)midi({midiChannels[note],note,0,false});midiActive[note]=false;midiOffDue[note]=INT64_MAX;}else midiNextOffDue=std::min(midiNextOffDue,midiOffDue[note]);}}
}
