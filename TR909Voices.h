#pragma once
// ============================================================================
//  TR909Voices.h — Synthèse DSP des voix de la TR-909
//
//  Le 909 a :
//    - Kick & toms analogiques (synthèse sinus + env)
//    - Snare analogique (deux osc + bruit)
//    - Hi-hat/Ride/Crash numériques (samples 1-bit) — ici synthétisés
//    - Clap numérique
//  On conserve l'aspect "percussif et sec" du 909.
// ============================================================================

#include "DspCommon.h"

namespace zlabs
{

// ─────────────── TR-909 : Kick ───────────────
// Plus punchy que la 808. Sinus avec glide très court + distortion
// de transistor + enveloppe exponentielle ~0.3s.
class TR909Kick
{
public:
    void prepare(double sr, int blockSize)
    {
        sampleRate = sr;
        osc.prepare(sr, blockSize);
        env.setDecay(0.3f);
    }
    void setTune(float semitones){ tune = semitones; }
    void setDecay(float secs){ env.setDecay(secs); }
    void setAccentGain(float g){ env.setAccentGain(g); }
    void setLevel(float l){ level = l; }
    void setTone(float t){ tone = t; }

    inline void trigger(float accent)
    {
        osc.reset();
        phase = 0.0f;
        startFreq = 220.0f;
        endFreq = 55.0f * std::pow(2.0f, tune / 12.0f);
        env.trigger(accent);
        click = 1.0f;
    }

    inline float processSample() noexcept
    {
        if (!env.isActive()) return 0.0f;
        float envVal = env.process();
        phase += 0.025f; // glide court
        float freq = startFreq + (endFreq - startFreq) * std::exp(-phase * 6.0f);
        osc.setFrequency(freq);
        float s = osc.processSample();
        // distorsion douce pour le "punch" 909
        s = std::tanh(2.5f * s) * 0.55f;
        click *= 0.0005f;
        return (s * envVal + click * (std::rand() / (float)RAND_MAX - 0.5f)) * level;
    }

private:
    AnalogOsc osc;
    PercussionEnvelope env;
    double sampleRate = 44100.0;
    float tune = 0.0f;
    float level = 1.0f;
    float tone = 0.5f;
    float phase = 0.0f;
    float startFreq = 220.0f, endFreq = 55.0f;
    float click = 0.0f;
};

// ─────────────── TR-909 : Snare ───────────────
// Deux oscillateurs (180Hz et 330Hz) + bruit BP.
class TR909Snare
{
public:
    void prepare(double sr, int blockSize)
    {
        osc1.prepare(sr, blockSize); osc2.prepare(sr, blockSize);
        noise.prepare(sr, blockSize);
        bp.prepare(sr, blockSize);
        bp.setCutoff(2000.0f); bp.setResonance(0.5f);
        osc1.setFrequency(180.0f); osc2.setFrequency(330.0f);
        toneEnv.setDecay(0.08f); noiseEnv.setDecay(0.15f);
    }
    void setTune(float t){
        osc1.setFrequency(180.0f * std::pow(2.0f, t / 12.0f));
        osc2.setFrequency(330.0f * std::pow(2.0f, t / 12.0f));
    }
    void setDecay(float d){ noiseEnv.setDecay(d); }
    void setAccentGain(float g){ toneEnv.setAccentGain(g); noiseEnv.setAccentGain(g); }
    void setLevel(float l){ level = l; }
    void setTone(float t){ toneLevel = t; }
    void setSnappy(float s){ noiseLevel = s; }

    inline void trigger(float accent){ toneEnv.trigger(accent); noiseEnv.trigger(accent); }
    inline float processSample() noexcept
    {
        if (!toneEnv.isActive() && !noiseEnv.isActive()) return 0.0f;
        float t = osc1.processSample() * 0.5f + osc2.processSample() * 0.5f;
        float n = bp.processSample(noise.processSample());
        return (t * toneEnv.process() * toneLevel + n * noiseEnv.process() * noiseLevel) * level;
    }

private:
    AnalogOsc osc1, osc2;
    NoiseGenerator noise;
    AnalogBandpassFilter bp;
    PercussionEnvelope toneEnv, noiseEnv;
    float toneLevel = 0.5f, noiseLevel = 0.5f, level = 1.0f;
};

// ─────────────── TR-909 : Clap ───────────────
// 4 impulsions + tail comme 808 mais plus court.
class TR909Clap
{
public:
    void prepare(double sr, int blockSize)
    {
        noise.prepare(sr, blockSize);
        bp.prepare(sr, blockSize);
        bp.setCutoff(1500.0f); bp.setResonance(0.7f);
        env.setDecay(0.15f);
        tailEnv.setDecay(0.20f);
    }
    void setAccentGain(float g){ env.setAccentGain(g); tailEnv.setAccentGain(g); }
    inline void trigger(float accent)
    {
        pulses[0] = 0.0f; pulses[1] = 0.0030f; pulses[2] = 0.0060f; pulses[3] = 0.0100f;
        pIdx = 0;
        env.trigger(accent);
        tailEnv.trigger(accent);
    }
    inline float processSample(float samplePeriod) noexcept
    {
        while (pIdx < 4 && pulses[pIdx] <= 0.0f)
        {
            env.trigger(1.0f);
            pIdx++;
        }
        for (int i = pIdx; i < 4; ++i) pulses[i] -= samplePeriod;
        float v = bp.processSample(noise.processSample());
        float tail = bp.processSample(noise.processSample());
        return (v * env.process() * 0.8f + tail * tailEnv.process() * 0.3f);
    }
private:
    NoiseGenerator noise;
    AnalogBandpassFilter bp;
    PercussionEnvelope env, tailEnv;
    float pulses[4] = {0,0,0,0};
    int pIdx = 4;
};

// ─────────────── TR-909 : Toms ───────────────
class TR909Tom
{
public:
    void prepare(double sr, int blockSize)
    {
        osc.prepare(sr, blockSize);
        env.setDecay(0.20f);
    }
    void setTune(float semitones){ osc.setFrequency(70.0f * std::pow(2.0f, semitones / 12.0f)); }
    void setDecay(float d){ env.setDecay(d); }
    void setAccentGain(float g){ env.setAccentGain(g); }
    void setLevel(float l){ level = l; }
    inline void trigger(float accent){ env.trigger(accent); }
    inline float processSample() noexcept
    {
        if (!env.isActive()) return 0.0f;
        return osc.processSample() * env.process() * level;
    }
private:
    AnalogOsc osc;
    PercussionEnvelope env;
    float level = 1.0f;
};

// ─────────────── TR-909 : Hi-Hat ───────────────
class TR909HiHat
{
public:
    void prepare(double sr, int blockSize)
    {
        noise.prepare(sr, blockSize);
        hp.prepare(sr, blockSize);
        hp.setCutoff(7000.0f); hp.setResonance(0.8f);
        closedEnv.setDecay(0.04f);
        openEnv.setDecay(0.35f);
    }
    void setAccentGain(float g){ closedEnv.setAccentGain(g); openEnv.setAccentGain(g); }
    void setLevel(float l){ level = l; }
    inline void trigger(float accent, bool open)
    {
        if (open) openEnv.trigger(accent);
        else closedEnv.trigger(accent);
    }
    inline float processSample() noexcept
    {
        float n = noise.processSample();
        float h = hp.processSample(n);
        return h * (closedEnv.process() + openEnv.process()) * level;
    }
private:
    NoiseGenerator noise;
    AnalogHighpassFilter hp;
    PercussionEnvelope closedEnv, openEnv;
    float level = 1.0f;
};

// ─────────────── TR-909 : Ride / Crash ───────────────
class TR909Cymbal
{
public:
    void prepare(double sr, int blockSize)
    {
        noise.prepare(sr, blockSize);
        hp.prepare(sr, blockSize);
        hp.setCutoff(5500.0f); hp.setResonance(0.6f);
        crashEnv.setDecay(0.7f);
        rideEnv.setDecay(1.8f);
    }
    void setAccentGain(float g){ crashEnv.setAccentGain(g); rideEnv.setAccentGain(g); }
    void setLevel(float l){ level = l; }
    inline void trigger(float accent, bool isCrash)
    {
        if (isCrash) crashEnv.trigger(accent);
        else rideEnv.trigger(accent);
    }
    inline float processSample() noexcept
    {
        float n = noise.processSample();
        float h = hp.processSample(n);
        return h * (crashEnv.process() + rideEnv.process()) * level;
    }
private:
    NoiseGenerator noise;
    AnalogHighpassFilter hp;
    PercussionEnvelope crashEnv, rideEnv;
    float level = 1.0f;
};

// ─────────────── TR-909 : Rimshot ───────────────
class TR909Rimshot
{
public:
    void prepare(double sr, int blockSize)
    {
        noise.prepare(sr, blockSize);
        bp.prepare(sr, blockSize);
        bp.setCutoff(2500.0f); bp.setResonance(1.5f);
        env.setDecay(0.04f);
    }
    void setAccentGain(float g){ env.setAccentGain(g); }
    inline void trigger(float accent){ env.trigger(accent); }
    inline float processSample() noexcept
    {
        if (!env.isActive()) return 0.0f;
        return bp.processSample(noise.processSample()) * env.process() * 0.5f;
    }
private:
    NoiseGenerator noise;
    AnalogBandpassFilter bp;
    PercussionEnvelope env;
};

struct TR909VoiceSet
{
    TR909Kick     kick;
    TR909Snare    snare;
    TR909Clap     clap;
    TR909Tom      lowTom, midTom, hiTom;
    TR909HiHat    hiHat;
    TR909Rimshot  rimshot;
    TR909Cymbal   cymbal;

    void prepare(double sr, int blockSize)
    {
        kick.prepare(sr, blockSize);
        snare.prepare(sr, blockSize);
        clap.prepare(sr, blockSize);
        lowTom.prepare(sr, blockSize);  lowTom.setTune(0.0f);
        midTom.prepare(sr, blockSize);   midTom.setTune(5.0f);
        hiTom.prepare(sr, blockSize);    hiTom.setTune(10.0f);
        hiHat.prepare(sr, blockSize);
        rimshot.prepare(sr, blockSize);
        cymbal.prepare(sr, blockSize);
    }
};

} // namespace zlabs
