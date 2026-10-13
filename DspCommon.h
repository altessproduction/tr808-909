#pragma once
// ============================================================================
//  DspCommon.h — blocs DSP de base partagés par les moteurs 808/909
// ============================================================================
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <random>

namespace zlabs
{

// ─────────────────────── Oscillateur pseudo-analogique ──────────────────────
// Sur la TR-808, le kick/snare/tom sont générés par un oscillateur quasi-sinusoïdal
// basé sur un transistor. La distorsion douce (tanh) imite le transistor saturé.

class AnalogOsc
{
public:
    void prepare(double sampleRate, int blockSize)
    {
        sampleRate_ = sampleRate;
        phase = 0.0f;
    }
    inline void setFrequency(float f) noexcept { freq = f; }
    inline void reset() noexcept { phase = 0.0f; }

    inline float processSample() noexcept
    {
        float x = std::sin(2.0f * 3.14159265358979f * phase);
        // distorsion douce (tanh) pour imiter transistor saturé
        float out = std::tanh(2.0f * x) * 0.7f;
        phase += freq / (float)sampleRate_;
        if (phase >= 1.0f) phase -= 1.0f;
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    float freq = 60.0f;
    float phase = 0.0f;
};

// ─────────────────────── Générateur de bruit (white/pink) ──────────────────
class NoiseGenerator
{
public:
    void prepare(double sampleRate, int blockSize)
    {
        (void)sampleRate; (void)blockSize;
        rng.seed(std::random_device{}());
        pinkState = 0.0f;
    }
    void setMode(bool pink) noexcept { pinkMode = pink; }

    inline float processSample() noexcept
    {
        // bruit blanc uniforme
        std::uniform_real_distribution<float> d(-1.0f, 1.0f);
        float white = d(rng);
        if (!pinkMode) return white;
        // filtre simple 1-pole pour approximation pink-ish
        pinkState = 0.99f * pinkState + 0.05f * white;
        return pinkState * 4.0f;
    }

private:
    std::mt19937 rng;
    bool pinkMode = false;
    float pinkState = 0.0f;
};

// ─────────────────────── Enveloppe de percussion ──────────────────────────
// Tient compte d'un niveau d'accent (accent par step) pour les 808/909.
struct PercussionEnvelope
{
    void setAttack(float a)  noexcept { attack = a;    attackInv = (a > 0.0f) ? 1.0f / a : 1.0f; }
    void setDecay(float d)   noexcept { decay = d;     decayInv  = (d > 0.0f) ? 1.0f / d : 1.0f; }
    void setAccentGain(float g)noexcept { accentGain = g; }

    void trigger(float accentAmount)
    {
        currentAccent = juce::jlimit(0.0f, 2.0f, accentAmount * accentGain);
        value = 1.0f;             // attaque quasi-instantanée (percussion)
        state = State::Decay;
    }

    inline float process() noexcept
    {
        switch (state)
        {
        case State::Attack:
            value += attackInv * currentAccent;
            if (value >= 1.0f) { value = 1.0f; state = State::Decay; }
            break;
        case State::Decay:
            value -= decayInv;
            if (value <= 0.0f) { value = 0.0f; state = State::Idle; }
            break;
        case State::Idle:
            value = 0.0f;
            break;
        }
        return value * currentAccent;
    }

    bool isActive() const noexcept { return state != State::Idle; }

private:
    float attack = 0.001f;
    float decay = 0.1f;
    float attackInv = 1000.0f;
    float decayInv = 10.0f;
    float accentGain = 1.0f;
    float currentAccent = 1.0f;
    float value = 0.0f;
    enum class State { Attack, Decay, Idle } state = State::Idle;
};

// ─────────────────────── Filtre analogique (lp SVF) ───────────────────────
class AnalogLowpassFilter
{
public:
    void prepare(double sampleRate, int blockSize)
    {
        (void)blockSize;
        sampleRate_ = sampleRate;
        updateCoeffs();
    }
    void setCutoff(float f) noexcept { cutoff = f; updateCoeffs(); }
    void setResonance(float q) noexcept { q_ = q; updateCoeffs(); }

    inline float processSample(float x) noexcept
    {
        // forme Digital State Variable (Chamberlin) - approximation analogique
        float low = low1 + cutoff * band1;
        float high = x - low - q_ * band1;
        float band = band1 + cutoff * high;
        low1 = low; band1 = band;
        return low;
    }

private:
    void updateCoeffs() noexcept
    {
        // k = 2 * sin(pi * fc / fs) (limité pour stabilité)
        float wc = cutoff / (float)sampleRate_;
        cutoff = juce::jlimit(0.0001f, 0.49f, wc * 2.0f * 3.14159265f);
    }
    double sampleRate_ = 44100.0;
    float cutoff = 1000.0f;
    float q_ = 0.5f;
    float low1 = 0.0f, band1 = 0.0f;
};

// ─────────────────────── Band-pass (snare, clap) ──────────────────────────
class AnalogBandpassFilter
{
public:
    void prepare(double sampleRate, int blockSize)
    {
        (void)blockSize; sampleRate_ = sampleRate;
        updateCoeffs();
    }
    void setCutoff(float f) noexcept { cutoff = f; updateCoeffs(); }
    void setResonance(float q) noexcept { q_ = q; updateCoeffs(); }

    inline float processSample(float x) noexcept
    {
        float low = low1 + cutoff * band1;
        float high = x - low - q_ * band1;
        float band = band1 + cutoff * high;
        low1 = low; band1 = band;
        return band; // BP -> on retourne le band
    }

private:
    void updateCoeffs() noexcept
    {
        float wc = cutoff / (float)sampleRate_;
        cutoff = juce::jlimit(0.0001f, 0.49f, wc * 2.0f * 3.14159265f);
    }
    double sampleRate_ = 44100.0;
    float cutoff = 1000.0f, q_ = 0.5f;
    float low1 = 0.0f, band1 = 0.0f;
};

// ─────────────────────── High-pass (hi-hat) ───────────────────────────────
class AnalogHighpassFilter
{
public:
    void prepare(double sampleRate, int blockSize)
    {
        (void)blockSize; sampleRate_ = sampleRate;
        updateCoeffs();
    }
    void setCutoff(float f) noexcept { cutoff = f; updateCoeffs(); }
    void setResonance(float q) noexcept { q_ = q; updateCoeffs(); }

    inline float processSample(float x) noexcept
    {
        float low = low1 + cutoff * band1;
        float high = x - low - q_ * band1;
        float band = band1 + cutoff * high;
        low1 = low; band1 = band;
        return high;
    }

private:
    void updateCoeffs() noexcept
    {
        float wc = cutoff / (float)sampleRate_;
        cutoff = juce::jlimit(0.0001f, 0.49f, wc * 2.0f * 3.14159265f);
    }
    double sampleRate_ = 44100.0;
    float cutoff = 1000.0f, q_ = 0.5f;
    float low1 = 0.0f, band1 = 0.0f;
};

// ─────────────────────── Conversion Hz/semitones ──────────────────────────
inline float semitonesToHz(float baseHz, float semitones) noexcept
{
    return baseHz * std::pow(2.0f, semitones / 12.0f);
}

// ─────────────────────── One-pole low pass (smoothing) ────────────────────
struct OnePoleLP
{
    void prepare(double sampleRate) { a_ = 1.0f - std::exp(1.0f / (0.020f * sampleRate)); }
    inline float process(float x) noexcept { return y += a_ * (x - y); }
    float y = 0.0f;
    float a_ = 0.001f;
};

} // namespace zlabs
