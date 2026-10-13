// ============================================================================
//  PluginProcessor.cpp — Implémentation du Plugin VST3 TR808_909
// ============================================================================

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace zlabs
{

TR808_909AudioProcessor::TR808_909AudioProcessor()
    : juce::AudioProcessor (BusesProperties()
        .withInput ("MainIn", juce::AudioChannelSet::stereo(), false)
        .withOutput ("MainOut", juce::AudioChannelSet::stereo(), true))
{
}

bool TR808_909AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return true;
}

void TR808_909AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    engine.prepare(sampleRate, samplesPerBlock);
    sequencer.prepare(sampleRate, samplesPerBlock);
    applyParameters();
    sequencer.reset();
}

void TR808_909AudioProcessor::applyParameters()
{
    using P = ParameterIDs;
    auto& apvts = this->apvts;

    auto get = [&](const char* id) -> float
    { return apvts.getRawParameterValue(id)->load(); };

    auto getChoice = [&](const char* id) -> int
    { return (int)apvts.getParameter(id)->getValue() * (apvts.getParameter(id)->getNumSteps() - 1); };

    // Machine mode
    int mode = apvts.getParameter(P::kMachineMode)->getValue() > 0.5f ? 1 : 0;
    engine.setMode(mode == 0 ? MachineMode::TR808 : MachineMode::TR909);

    engine.setMasterLevel(get(P::kMasterLevel));
    engine.setDistortionAmount(get(P::kDistoAmount));
    engine.setDistortionMix(get(P::kDistoMix));

    // Voix 808
    auto& v8 = engine.get808();
    v8.kick.setTune(get(P::kKickTune));
    v8.kick.setDecay(get(P::kKickDecay));
    v8.kick.setLevel(get(P::kKickLevel));
    v8.snare.setTone(get(P::kSnareTone));
    v8.snare.setNoise(get(P::kSnareSnappy));
    v8.snare.setDecay(get(P::kSnareDecay));
    v8.snare.setLevel(get(P::kSnareLevel));
    v8.clap.setAccentGain(get(P::kClapLevel));
    v8.lowTom.setDecay(get(P::kTomDecay)); v8.lowTom.setLevel(get(P::kTomLevel));
    v8.midTom.setDecay(get(P::kTomDecay)); v8.midTom.setLevel(get(P::kTomLevel));
    v8.hiTom.setDecay(get(P::kTomDecay));  v8.hiTom.setLevel(get(P::kTomLevel));
    v8.hiHat.setLevel(get(P::kHatLevel));
    v8.cymbal.setLevel(get(P::kCymbalLevel));
    v8.cowbell.setLevel(get(P::kCowbellLevel));
    v8.cowbell.setTune(get(P::kCowbellTune));

    // Voix 909
    auto& v9 = engine.get909();
    v9.kick.setTune(get(P::kKickTune));
    v9.kick.setDecay(get(P::kKickDecay));
    v9.kick.setLevel(get(P::kKickLevel));
    v9.snare.setTone(get(P::kSnareTone));
    v9.snare.setSnappy(get(P::kSnareSnappy));
    v9.snare.setDecay(get(P::kSnareDecay));
    v9.snare.setLevel(get(P::kSnareLevel));
    v9.clap.setAccentGain(get(P::kClapLevel));
    v9.lowTom.setDecay(get(P::kTomDecay)); v9.lowTom.setLevel(get(P::kTomLevel));
    v9.midTom.setDecay(get(P::kTomDecay)); v9.midTom.setLevel(get(P::kTomLevel));
    v9.hiTom.setDecay(get(P::kTomDecay));  v9.hiTom.setLevel(get(P::kTomLevel));
    v9.hiHat.setLevel(get(P::kHatLevel));
    v9.cymbal.setLevel(get(P::kCymbalLevel));

    // Séquenceur
    bool seqOn = get(P::kSeqEnabled) > 0.5f;
    sequencer.setEnabled(seqOn);
    sequencer.setBpm(get(P::kSeqBpm));
    sequencer.setShuffle(get(P::kSeqShuffle));
    int patternIdx = get(P::kSeqPatternIdx);
    sequencer.setCurrentPattern(patternIdx);
}

void TR808_909AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // Si on doit synchroniser via transport du DAW
    sequencer.syncToHost(getPlayHead(), apvts.getRawParameterValue(ParameterIDs::kSeqBpm)->load());

    // Déclenche les notes venant du MIDI input (mode "module sonore")
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
        {
            engine.triggerNote(msg.getNoteNumber(), msg.getVelocity() / 127.0f);
        }
        else if (msg.isController())
        {
            midiLearn.handleCC(msg.getChannel(), msg.getControllerNumber(),
                              msg.getControllerValue() / 127.0f, apvts);
            applyParameters(); // réapplique après CC
        }
    }

    // Si séquenceur activé : génère des triggers internes
    if (sequencer.isEnabled())
    {
        sequencer.processBlock(buffer.getNumSamples(),
            [this](int note, float vel)
            {
                engine.triggerNote(note, vel);
            });
    }

    // Applique les paramètres (smooth des params)
    applyParameters();

    // Rendu audio
    engine.renderBlock(buffer, buffer.getNumSamples());

    // Clip douceur + master
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* w = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // soft clip
            w[i] = std::tanh(2.0f * w[i]) * 0.5f;
        }
    }

    // Vide le buffer MIDI (on consomme les notes)
    midi.clear();
}

void TR808_909AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Sérialise l'état APVTS + mappings MIDI + patterns séquenceur
    juce::ValueTree vt("TR808_909_State");
    vt.appendChild(apvts.copyState(), nullptr);
    vt.setProperty("midiMappings", midiLearn.serializeMappings(), nullptr);
    vt.setProperty("patterns", sequencer.getCurrentPattern().serialize(), nullptr);
    juce::MemoryOutputStream stream(destData, false);
    vt.writeToStream(stream);
}

void TR808_909AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto vt = juce::ValueTree::readFromData(data, sizeInBytes);
    if (!vt.isValid()) return;

    // Restaurer APVTS
    auto params = vt.getChildWithName("PARAMS");
    if (params.isValid())
        apvts.replaceState(params);

    // Restaurer mappings MIDI
    auto mm = vt.getProperty("midiMappings");
    if (mm.toString().isNotEmpty())
        midiLearn.deserializeMappings(mm.toString());

    // Restaurer patterns
    auto patternsData = vt.getProperty("patterns");
    if (patternsData.toString().isNotEmpty())
        sequencer.getCurrentPattern().deserialize(patternsData.toString());

    applyParameters();
}

juce::AudioProcessorEditor* TR808_909AudioProcessor::createEditor()
{
    return new PluginEditor(*this);
}

} // namespace zlabs

// ============================================================================
//  Boilerplate JUCE
// ============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new zlabs::TR808_909AudioProcessor();
}
