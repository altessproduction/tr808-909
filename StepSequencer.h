#pragma once
// ============================================================================
//  StepSequencer.h — Séquenceur 16 steps avec accents et shuffle
//
//  - 16 instruments (kick, snare, etc.) — utilisé mapping GM Drum
//  - Accent par step (bit)
//  - Shuffle paramétrable (50% = droit, 66% = swing moyen, 75% = triplet)
//  - Sync DAW transport via AudioPlayHead + PPQ position
//  - Sauvegarde de patterns dans le state du plugin
// ============================================================================

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

namespace zlabs
{

struct Pattern
{
    // 16 steps × 16 instruments (mapping 0..15 interne)
    std::array<std::array<bool, 16>, 16> steps{};       // gate
    std::array<std::array<bool, 16>, 16> accents{};     // accent per step

    void clear()
    {
        for (auto& row : steps) row.fill(false);
        for (auto& row : accents) row.fill(false);
    }

    juce::String serialize() const
    {
        juce::StringArray arr;
        for (int i = 0; i < 16; ++i)
        {
            juce::String row;
            for (int s = 0; s < 16; ++s)
            {
                int v = steps[i][s] ? 1 : 0;
                if (accents[i][s]) v |= 2;
                row += juce::String(v);
                if (s < 15) row += ",";
            }
            arr.add(row);
        }
        return arr.joinIntoString(";");
    }

    void deserialize(const juce::String& s)
    {
        clear();
        auto rows = juce::StringArray::fromTokens(s, ";", "");
        for (int i = 0; i < juce::jmin(16, rows.size()); ++i)
        {
            auto steps_ = juce::StringArray::fromTokens(rows[i], ",", "");
            for (int s = 0; s < juce::jmin(16, steps_.size()); ++s)
            {
                int v = steps_[s].getIntValue();
                steps[i][s] = (v & 1) != 0;
                accents[i][s] = (v & 2) != 0;
            }
        }
    }
};

// Mapping interne (0..15) → note MIDI GM Drum pour le moteur.
static constexpr int kNumInstruments = 11; // 808/909 instruments principaux
static constexpr int kInstrumentMidiNotes[kNumInstruments] =
{
    35, // kick
    38, // snare
    40, // rimshot
    39, // clap
    41, // low tom
    43, // mid tom
    48, // hi tom
    42, // closed HH
    46, // open HH
    49, // crash
    51  // ride
};

class StepSequencer
{
public:
    void prepare(double sampleRate, int blockSize)
    {
        sr_ = sampleRate;
        bs_ = blockSize;
    }

    void setBpm(double bpm) noexcept { bpm_ = bpm; }
    void setShuffle(float s) noexcept { shuffle_ = juce::jlimit(0.0f, 0.75f, s); }
    void setNumSteps(int n) noexcept { numSteps_ = juce::jlimit(1, 16, n); }
    void setEnabled(bool e) noexcept { enabled_ = e; }
    bool isEnabled() const noexcept { return enabled_; }

    Pattern& getCurrentPattern() { return patterns_[currentPatternIdx_]; }
    int getCurrentPatternIdx() const noexcept { return currentPatternIdx_; }
    void setCurrentPattern(int idx)
    {
        currentPatternIdx_ = juce::jlimit(0, (int)patterns_.size() - 1, idx);
    }

    Pattern& getPattern(int idx) { return patterns_[idx]; }

    // ─────── Synchronisation au transport du DAW ───────
    // On utilise le AudioPlayHead::PositionInfo::ppqPosition pour suivre la
    // position actuelle en PPQ (16th = 0.25 PPQ).
    void syncToHost(juce::AudioPlayHead* playHead, double fallbackBpm)
    {
        if (playHead == nullptr)
        {
            bpm_ = fallbackBpm;
            hostSyncActive_ = false;
            return;
        }
        auto info = playHead->getPosition();
        if (info.hasValue())
        {
            auto pos = *info;
            if (pos.getBpm().hasValue())
                bpm_ = *pos.getBpm();
            else
                bpm_ = fallbackBpm;
            if (pos.getPpqPosition().hasValue())
            {
                hostPpq_ = *pos.getPpqPosition();
                hostSyncActive_ = true;
                isPlaying_ = pos.getIsPlaying() || pos.getIsRecording();
            }
            else
            {
                hostSyncActive_ = false;
            }
        }
        else
        {
            hostSyncActive_ = false;
            bpm_ = fallbackBpm;
        }
    }

    // ─────── Avance d'un bloc ───────
    // Déclenche les notes qui correspondent aux steps tombant dans ce bloc.
    // `triggerFn` est appelé pour chaque note: (noteMidi, velocity)
    void processBlock(int numSamples, std::function<void(int, float)> triggerFn)
    {
        if (!enabled_) return;

        const double ppqPerSample = (bpm_ / 60.0) / 4.0 / sr_; // 16th = 0.25 ppq
        double startPpq = hostSyncActive_ ? hostPpq_ : internalPpq_;
        const int kStart = 0;
        for (int i = 0; i < numSamples; ++i)
        {
            double cur = startPpq + i * ppqPerSample;
            // On cherche le step courant
            int bar = (int)(cur / 4.0);
            double barStartPpq = bar * 4.0;
            double offset = cur - barStartPpq;
            int step16 = (int)(offset * 4.0); // 0..15
            if (step16 >= 16) step16 = 15;
            if (step16 < 0) step16 = 0;

            // shuffle: décale les steps impairs
            double stepPpq = step16 * 0.25;
            double nextStepPpq = (step16 + 1) * 0.25;
            // pour les steps impairs, on applique le swing (délai)
            bool isOdd = (step16 & 1) != 0;
            double swingDelay = isOdd ? (shuffle_ * 0.25) : 0.0;
            double realStepPpq = stepPpq + swingDelay;

            // Si on a passé ce step (et qu'on n'a pas encore joué ce step dans ce bar)
            if (cur >= realStepPpq && lastStepProcessed_ != step16 + bar * 16)
            {
                lastStepProcessed_ = step16 + bar * 16;
                auto& pat = patterns_[currentPatternIdx_];
                for (int inst = 0; inst < kNumInstruments; ++inst)
                {
                    if (pat.steps[inst][step16])
                    {
                        float vel = pat.accents[inst][step16] ? 1.4f : 0.9f;
                        triggerFn(kInstrumentMidiNotes[inst], vel);
                    }
                }
            }
        }

        // Avance le compteur interne si pas synchro host
        if (!hostSyncActive_)
        {
            internalPpq_ += numSamples * ppqPerSample;
        }
    }

    void reset() noexcept
    {
        internalPpq_ = 0.0;
        lastStepProcessed_ = -1;
    }

private:
    double sr_ = 44100.0;
    int bs_ = 512;
    double bpm_ = 120.0;
    float shuffle_ = 0.0f;
    int numSteps_ = 16;
    bool enabled_ = false;
    bool hostSyncActive_ = false;
    bool isPlaying_ = false;
    double hostPpq_ = 0.0;
    double internalPpq_ = 0.0;
    long long lastStepProcessed_ = -1;
    int currentPatternIdx_ = 0;
    std::array<Pattern, 16> patterns_{};
};

} // namespace zlabs
