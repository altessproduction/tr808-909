#pragma once
// ============================================================================
//  DrumEngine.h — Moteur de synthèse qui combine 808 + 909
//
//  - Sélecteur de machine (0 = 808, 1 = 909)
//  - Distorsion interne (tanh) sur le bus de sortie
//  - Mix stéréo (les voix sont mono, on applique un panorama léger)
// ============================================================================

#include "TR808Voices.h"
#include "TR909Voices.h"
#include "DistortionStage.h"

namespace zlabs
{

enum class MachineMode : int { TR808 = 0, TR909 = 1 };

class DrumEngine
{
public:
    void prepare(double sampleRate, int maxBlockSize)
    {
        voices808.prepare(sampleRate, maxBlockSize);
        voices909.prepare(sampleRate, maxBlockSize);
        distortion.prepare(sampleRate);
        for (int i = 0; i < 16; ++i)
            levelSmoother[i].prepare(sampleRate);
    }

    // ─────── Mapping des notes MIDI GM Drum (avec octave basse) ───────
    // GM: 35 = kick, 38 = snare, 42 = closed HH, 46 = open HH, 39 = clap,
    //     40 = rimshot, 41 = low tom, 45 = mid tom, 48 = hi tom, 49 = crash,
    //     51 = ride, 56 = cowbell, 75 = clave
    void triggerNote(int midiNote, float velocity)
    {
        float accent = juce::jlimit(0.0f, 1.5f, velocity);
        if (mode == MachineMode::TR808)
        {
            switch (midiNote)
            {
            case 35: case 36: voices808.kick.trigger(accent); break;
            case 38: voices808.snare.trigger(accent); break;
            case 40: voices808.rimshot.trigger(accent); break;
            case 39: voices808.clap.trigger(accent); break;
            case 41: voices808.lowTom.trigger(accent); break;
            case 43: voices808.midTom.trigger(accent); break;
            case 45: case 48: voices808.hiTom.trigger(accent); break;
            case 42: voices808.hiHat.trigger(accent, false); break;
            case 46: voices808.hiHat.trigger(accent, true); break;
            case 49: voices808.cymbal.trigger(accent, true); break;
            case 51: voices808.cymbal.trigger(accent, false); break;
            case 56: voices808.cowbell.trigger(accent); break;
            case 75: voices808.clave.trigger(accent); break;
            default: break;
            }
        }
        else
        {
            switch (midiNote)
            {
            case 35: case 36: voices909.kick.trigger(accent); break;
            case 38: voices909.snare.trigger(accent); break;
            case 40: voices909.rimshot.trigger(accent); break;
            case 39: voices909.clap.trigger(accent); break;
            case 41: voices909.lowTom.trigger(accent); break;
            case 43: voices909.midTom.trigger(accent); break;
            case 45: case 48: voices909.hiTom.trigger(accent); break;
            case 42: voices909.hiHat.trigger(accent, false); break;
            case 46: voices909.hiHat.trigger(accent, true); break;
            case 49: voices909.cymbal.trigger(accent, true); break;
            case 51: voices909.cymbal.trigger(accent, false); break;
            default: break;
            }
        }
    }

    // ─────── Rendu bloc stéréo ───────
    void renderBlock(juce::AudioBuffer<float>& buffer, int numSamples)
    {
        const double samplePeriod = 1.0 / 44100.0; // approximatif pour clap
        for (int i = 0; i < numSamples; ++i)
        {
            float mono = 0.0f;
            if (mode == MachineMode::TR808)
            {
                mono += voices808.kick.processSample();
                mono += voices808.snare.processSample();
                mono += voices808.rimshot.processSample();
                mono += voices808.clap.processSample((float)samplePeriod);
                mono += voices808.lowTom.processSample();
                mono += voices808.midTom.processSample();
                mono += voices808.hiTom.processSample();
                mono += voices808.hiHat.processSample();
                mono += voices808.cowbell.processSample();
                mono += voices808.clave.processSample();
                mono += voices808.cymbal.processSample();
            }
            else
            {
                mono += voices909.kick.processSample();
                mono += voices909.snare.processSample();
                mono += voices909.clap.processSample((float)samplePeriod);
                mono += voices909.lowTom.processSample();
                mono += voices909.midTom.processSample();
                mono += voices909.hiTom.processSample();
                mono += voices909.hiHat.processSample();
                mono += voices909.rimshot.processSample();
                mono += voices909.cymbal.processSample();
            }

            // distorsion interne (parfois demandée)
            mono = distortion.process(mono);

            // panoramique léger center
            buffer.setSample(0, i, mono);
            if (buffer.getNumChannels() > 1)
                buffer.setSample(1, i, mono);
        }
    }

    void setMode(MachineMode m) noexcept { mode = m; }
    MachineMode getMode() const noexcept { return mode; }

    void setDistortionAmount(float a) noexcept { distortion.setAmount(a); }
    void setDistortionMix(float m) noexcept { distortion.setMix(m); }
    void setMasterLevel(float l) noexcept { master = l; }

    // Références pour accéder aux paramètres des voix depuis le GUI
    TR808VoiceSet& get808() { return voices808; }
    TR909VoiceSet& get909() { return voices909; }

private:
    TR808VoiceSet voices808;
    TR909VoiceSet voices909;
    DistortionStage distortion;
    MachineMode mode = MachineMode::TR808;
    OnePoleLP levelSmoother[16];
    float master = 1.0f;
};

} // namespace zlabs
