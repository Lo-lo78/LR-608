// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>
#include <functional>

namespace lr608
{
struct TimingSettings
{
    double sampleRate = 44100.0;
    double tempo = 120.0;
    int timeSignatureNumerator = 4;
    int timeSignatureDenominator = 4;
    int rollDivision = 0;
    double shufflePercent = 0.0;
    double secondHitReductionPercent = 0.0;
    int cowbellEngine = 0;
    bool hostTimelineValid = false;
    bool hostPlaying = false;
    double hostPpqPosition = 0.0;
    double hostBarStartPpq = 0.0;
};

struct DrumTrigger
{
    int note = 0;
    int velocity = 0;
};

struct GeneratedDrumMidi
{
    int channel = 0;
    int note = 0;
    int velocity = 0;
    bool noteOn = false;
};

// Native port of the JSFX Hold Roll and CC1 Drag state machines. The owner
// feeds MIDI at the event's real sample and calls tick() exactly once per
// rendered sample. This keeps the scheduler independent of block size.
class DrumTimingEngine
{
public:
    using TriggerCallback = std::function<void (DrumTrigger)>;
    using MidiCallback = std::function<void (GeneratedDrumMidi)>;

    void reset();
    void setSettings (const TimingSettings&);
    bool handleMidi (std::uint8_t status, std::uint8_t data1, std::uint8_t data2,
                     const TriggerCallback&, const MidiCallback& = {});
    void tick (const TriggerCallback&, const MidiCallback& = {});
    bool needsSampleClock() const noexcept
    {
        // Merely selecting a Roll division must not keep the audio thread
        // awake. The clock is only needed while a held/pending note, a drag,
        // or a generated MIDI gate can still produce an event.
        return (rollEnabled && (anyHeld() || anyPending()))
            || dragSnare1.stage>0 || dragSnare2.stage>0 || dragCowbell.stage>0
            || anyMidiActive();
    }

    static bool isSupportedNote (int note);
    static int pitchBendDivision (int baseDivision, int bend);
    static int hitsPerBar (int division);

private:
    static constexpr double never = 1.0e15;

    struct DragState
    {
        int stage = 0;
        int velocity = 0;
        std::int64_t due = 0;
        int gap = 1;
        int channel = 0;
        bool emitMidi = false;
    };

    bool dragIsSupported (int note) const;
    bool anyHeld() const;
    bool anyPending() const;
    double gapSamples (int nextPhase) const;
    void findJoinPoint (double eventClock, double& due, int& phase) const;
    void press (int note, int velocity, int channel);
    void release (int note);
    void rescheduleAll();
    void syncToHostGrid();
    void armDrag (int note, int velocity, std::int64_t firstDue, int gap, int channel, bool emitMidi);
    void tickDrag (int note, DragState&, const TriggerCallback&, const MidiCallback&);
    void midiHit (int note, int velocity, int channel, int gate, const MidiCallback&);
    void flushMidiDue (const MidiCallback&);
    bool anyMidiActive() const noexcept;

    TimingSettings settings;
    std::array<bool, 128> held {};
    std::array<bool, 128> pending {};
    std::array<int, 128> velocities {};
    std::array<double, 128> due {};
    std::array<int,128> inputChannels{}, midiChannels{};
    std::array<bool,128> inputSuppressed{}, midiActive{};
    std::array<std::int64_t,128> midiOffDue{};
    std::int64_t midiNextOffDue = 0;
    int userDivision = 0;
    int effectiveDivision = 0;
    int pitchBend = 0;
    bool rollEnabled = false;
    bool gridActive = false;
    int gridPhase = 0;
    double gridDue = never;
    std::int64_t rollClock = 0;
    std::int64_t dragClock = 0;
    bool dragOn = false;
    int dragFixedGap = 176;
    DragState dragSnare1, dragSnare2, dragCowbell;
};
}
