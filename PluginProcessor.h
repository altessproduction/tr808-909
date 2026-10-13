#pragma once
// ============================================================================
//  PluginProcessor.h — Plugin VST3 TR808_909 (processeur audio)
// ============================================================================

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/DrumEngine.h"
#include "sequencer/StepSequencer.h"
#include "params/ParameterManager.h"
#include "midi/MidiLearn.h"

namespace zlabs
{

class TR808_909AudioProcessor  : public juce::AudioProcessor
{
public:
    TR808_909AudioProcessor();
    ~TR808_909AudioProcessor() override = default;

    // ─────── AudioProcessor standard ───────
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const  override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Init"; }
    void changeProgramName(int, const juce::String&) override {}

    // ─────── Préparation / processing ───────
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override
    {
        // pas de traitement spécifique en bypass
    }

    // ─────── Editor ───────
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // ─────── State ───────
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // ─────── Accès publics ───────
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    DrumEngine& getEngine() { return engine; }
    StepSequencer& getSeq() { return sequencer; }
    MidiLearn& getMidiLearn() { return midiLearn; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "PARAMS", ParameterManager::createLayout()};
    DrumEngine engine;
    StepSequencer sequencer;
    MidiLearn midiLearn;

    // Mapping directe des paramètres → knobs de l'engine
    void applyParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TR808_909AudioProcessor)
};

} // namespace zlabs
