#pragma once
// ============================================================================
//  TR808Voices.h — Synthèse DSP des voix de la TR-808
//
//  Voix disponibles (norme GM Drum MIDI) :
//    35/36  Bass Drum      (kick)
//    38     Snare           (accentué/rimshot en 40)
//    39     Hand Clap
//    40     Rimshot
//    41..43 Toms (Low/Mid/High) + 45..48 conga-imitation
//    42/46  Closed/Open Hi-Hat
//    49..51 Crash / Ride (samples hybrides)
//    56     Cowbell
//    75      Clave
// ============================================================================

#include "DspCommon.h"

namespace zlabs
{

// ─────────────── TR-808 : Bass Drum ───────────────
// Une sinusoïde dont la fréquence glisse de ~150Hz à ~50Hz en 5ms,
// enveloppée par une exponentielle ~0.5s. Léger click initial pour le
// "snappy" classic. Pas de bruit (la 808 n'a pas de coussin de bruit).

class TR808Kick
{
public:
    void prepare(double sr, int blockSize)
    {
        sampleRate = sr;
        osc.prepare(sr, blockSize);
        env.setDecay(0.5f);
    }
    void setTune(float semitones) { tune = semitones; }
    void setDecay(float secs)     { decay = secs; env.setDecay(secs); }
    void setLevel(float l)       { level = l; }
    void setAccentGain(float g)  { env.setAccentGain(g); }
    void setToneTone(float t)    { tone = t; }    // brightness 0..1

    inline void trigger(float accent)
    {
        osc.reset();
        phase = 0.0f;
        startFreq = 200.0f;
        endFreq = 50.0f * std::pow(2.0f, tune / 12.0f);
        env.trigger(accent);
        click = 1.0f;
    }

    inline float processSample() noexcept
    {
        if (!env.isActive()) return 0.0f;
        float envVal = env.process();
        // glide de pitch (curve 0..1)
        phase += 0.012f; // ~5ms de glide
        float freq = startFreq + (endFreq - startFreq) * std::exp(-phase * 8.0f);
        if (envVal == 0.0f) return 0.0f;
        osc.setFrequency(freq);
        float s = osc.processSample();
        // click initial
        click *= 0.0005f;
        float out = (s * envVal + click * (std::rand() / (float)RAND_MAX - 0.5f)) * level;
        return out;
    }

private:
    AnalogOsc osc;
    PercussionEnvelope env;
    double sampleRate = 44100.0;
    float tune = 0.0f;
    float decay = 0.5f;
    float level = 1.0f;
    float tone = 0.5f;
    float phase = 0.0f;
    float startFreq = 200.0f, endFreq = 50.0f;
    float click = 0.0f;
};

// ─────────────── TR-808 : Snare Drum ───────────────
// Deux tons (oscillateurs à 200Hz et 330Hz) + bruit filtré BP.
class TR808Snare
{
public:
    void prepare(double sr, int blockSize)
    {
        sampleRate = sr;
        osc1.prepare(sr, blockSize); osc2.prepare(sr, blockSize);
        noise.prepare(sr, blockSize);
        bp.prepare(sr, blockSize);
        bp.setCutoff(1800.0f); bp.setResonance(0.7f);
        osc1.setFrequency(190.0f); osc2.setFrequency(330.0f);
        toneEnv.setDecay(0.1f); noiseEnv.setDecay(0.2f);
        toneEnv.setAccentGain(1.0f); noiseEnv.setAccentGain(1.0f);
    }
    void setTone(float t) { toneLevel = t; }
    void setNoise(float n){ noiseLevel = n; }
    void setDecay(float d){ noiseEnv.setDecay(d); }
    void setAccentGain(float g){ toneEnv.setAccentGain(g); noiseEnv.setAccentGain(g); }
    void setLevel(float l){ level = l; }
    void setTune(float semitones){ osc1.setFrequency(190.0f * std::pow(2.0f, semitones/12.0f));
                                  osc2.setFrequency(330.0f * std::pow(2.0f, semitones/12.0f)); }

    inline void trigger(float accent)
    {
        toneEnv.trigger(accent);
        noiseEnv.trigger(accent);
    }
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
    double sampleRate = 44100.0;
    float toneLevel = 0.5f, noiseLevel = 0.5f, level = 1.0f;
};

// ─────────────── TR-808 : Rimshot ───────────────
// Bande-étroite BP sur un pulse court.
class TR808Rimshot
{
public:
    void prepare(double sr, int blockSize)
    {
        noise.prepare(sr, blockSize);
        bp.prepare(sr, blockSize);
        bp.setCutoff(2200.0f); bp.setResonance(1.2f);
        env.setDecay(0.05f);
    }
    void setTune(float t) { tune = t; bp.setCutoff(2200.0f * std::pow(2.0f, t / 12.0f)); }
    void setAccentGain(float g){ env.setAccentGain(g); }
    inline void trigger(float accent){ env.trigger(accent); }
    inline float processSample() noexcept
    {
        if (!env.isActive()) return 0.0f;
        return bp.processSample(noise.processSample()) * env.process() * 0.6f;
    }
private:
    NoiseGenerator noise;
    AnalogBandpassFilter bp;
    PercussionEnvelope env;
    float tune = 0.0f;
};

// ─────────────── TR-808 : Hand Clap ───────────────
// Quatre impulsions de bruit filtrées BP + decay plus long.
class TR808Clap
{
public:
    void prepare(double sr, int blockSize)
    {
        noise.prepare(sr, blockSize);
        bp.prepare(sr, blockSize);
        bp.setCutoff(1200.0f); bp.setResonance(0.8f);
        env.setDecay(0.18f);
        tailEnv.setDecay(0.30f);
    }
    void setAccentGain(float g){ env.setAccentGain(g); tailEnv.setAccentGain(g); }
    inline void trigger(float accent)
    {
        // 4 impulsions étalées sur ~15ms
        pulses[0] = 0.0f; pulses[1] = 0.0035f; pulses[2] = 0.0070f; pulses[3] = 0.0115f;
        pIdx = 0;
        env.trigger(accent);
        tailEnv.trigger(accent);
    }
    inline float processSample(float samplePeriod) noexcept
    {
        if (pIdx < 4 && pulses[pIdx] <= 0.0f)
        {
            env.trigger(1.0f);
            pIdx++;
        }
        pulses[pIdx] -= samplePeriod;
        float v = bp.processSample(noise.processSample());
        float tail = bp.processSample(noise.processSample());
        return (v * env.process() * 0.8f + tail * tailEnv.process() * 0.4f);
    }
private:
    NoiseGenerator noise;
    AnalogBandpassFilter bp;
    PercussionEnvelope env, tailEnv;
    float pulses[4] = {0,0,0,0};
    int pIdx = 4;
};

// ─────────────── TR-808 : Toms (Low/Mid/High) ───────────────
class TR808Tom
{
public:
    void prepare(double sr, int blockSize)
    {
        osc.prepare(sr, blockSize);
        env.setDecay(0.25f);
    }
    void setTune(float semitones) { osc.setFrequency(80.0f * std::pow(2.0f, semitones / 12.0f)); }
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

// ─────────────── TR-808 : Hi-Hat (closed/open) ───────────────
// 6 oscillateurs carrés (square waves) sommés + HP filter + env.
// 808 original: 6 oscillateurs carrés à 808*6/2 Hz etc, on simplifie
// avec un générateur de bruit HP filtré.
class TR808HiHat
{
public:
    void prepare(double sr, int blockSize)
    {
        noise.prepare(sr, blockSize);
        hp.prepare(sr, blockSize);
        bp.prepare(sr, blockSize);
        hp.setCutoff(6000.0f); hp.setResonance(0.6f);
        bp.setCutoff(10000.0f); bp.setResonance(0.4f);
        closedEnv.setDecay(0.04f);
        openEnv.setDecay(0.30f);
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
        float b = bp.processSample(h * 0.6f);
        float v = h * 0.6f + b * 0.4f;
        return v * (closedEnv.process() + openEnv.process()) * level;
    }
private:
    NoiseGenerator noise;
    AnalogHighpassFilter hp, bp;
    PercussionEnvelope closedEnv, openEnv;
    float level = 1.0f;
};

// ─────────────── TR-808 : Cowbell ───────────────
// 2 oscillateurs carrés (547Hz + 800Hz) + enveloppe courte.
class TR808Cowbell
{
public:
    void prepare(double sr, int blockSize)
    {
        osc1.prepare(sr, blockSize); osc2.prepare(sr, blockSize);
        osc1.setFrequency(547.0f); osc2.setFrequency(800.0f);
        env.setDecay(0.15f);
    }
    void setAccentGain(float g){ env.setAccentGain(g); }
    void setLevel(float l){ level = l; }
    void setTune(float semitones)
    {
        osc1.setFrequency(547.0f * std::pow(2.0f, semitones / 12.0f));
        osc2.setFrequency(800.0f * std::pow(2.0f, semitones / 12.0f));
    }
    inline void trigger(float accent){ env.trigger(accent); }
    inline float processSample() noexcept
    {
        if (!env.isActive()) return 0.0f;
        // crude square wave
        float a = osc1.processSample() > 0.0f ? 0.5f : -0.5f;
        float b = osc2.processSample() > 0.0f ? 0.5f : -0.5f;
        return (a + b) * 0.5f * env.process() * level;
    }
private:
    AnalogOsc osc1, osc2;
    PercussionEnvelope env;
    float level = 1.0f;
};

// ─────────────── TR-808 : Clave ───────────────
class TR808Clave
{
public:
    void prepare(double sr, int blockSize)
    {
        osc.prepare(sr, blockSize);
        osc.setFrequency(2500.0f);
        env.setDecay(0.04f);
    }
    void setAccentGain(float g){ env.setAccentGain(g); }
    void setLevel(float l){ level = l; }
    inline void trigger(float accent){ env.trigger(accent); }
    inline float processSample() noexcept
    {
        if (!env.isActive()) return 0.0f;
        float v = osc.processSample() > 0.0f ? 0.6f : -0.6f;
        return v * env.process() * level;
    }
private:
    AnalogOsc osc;
    PercussionEnvelope env;
    float level = 1.0f;
};

// ─────────────── Voix sampler fallback (crash/ride) ───────────────
// On n'a pas les samples ici, on synthétise un HP noise + tail.
class TR808Cymbal
{
public:
    void prepare(double sr, int blockSize)
    {
        noise.prepare(sr, blockSize);
        hp.prepare(sr, blockSize);
        bp.prepare(sr, blockSize);
        hp.setCutoff(5000.0f); hp.setResonance(0.7f);
        bp.setCutoff(7000.0f); bp.setResonance(0.4f);
        crashEnv.setDecay(0.8f);
        rideEnv.setDecay(1.5f);
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
        float b = bp.processSample(h * 0.5f);
        float v = h * 0.5f + b * 0.5f;
        return v * (crashEnv.process() + rideEnv.process()) * level;
    }
private:
    NoiseGenerator noise;
    AnalogHighpassFilter hp, bp;
    PercussionEnvelope crashEnv, rideEnv;
    float level = 1.0f;
};

// ─────────────── Engine 808 collection ───────────────
struct TR808VoiceSet
{
    TR808Kick     kick;
    TR808Snare    snare;
    TR808Rimshot  rimshot;
    TR808Clap     clap;
    TR808Tom      lowTom, midTom, hiTom;
    TR808HiHat    hiHat;
    TR808Cowbell  cowbell;
    TR808Clave    clave;
    TR808Cymbal   cymbal;

    void prepare(double sr, int blockSize)
    {
        kick.prepare(sr, blockSize);
        snare.prepare(sr, blockSize);
        rimshot.prepare(sr, blockSize);
        clap.prepare(sr, blockSize);
        lowTom.prepare(sr, blockSize); lowTom.setTune(0.0f);
        midTom.prepare(sr, blockSize); midTom.setTune(5.0f);
        hiTom.prepare(sr, blockSize);  hiTom.setTune(10.0f);
        hiHat.prepare(sr, blockSize);
        cowbell.prepare(sr, blockSize);
        clave.prepare(sr, blockSize);
        cymbal.prepare(sr, blockSize);
    }
};

} // namespace zlabs
