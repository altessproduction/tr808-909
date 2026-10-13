#pragma once
// ============================================================================
//  MidiLearn.h — Mapping MIDI CC → paramètre APVTS (MIDI Learn)
//
//  Principe : un bouton "Learn" est exposé dans le GUI. Quand l'utilisateur
//  appuie dessus et bouge un knob/CC, on enregistre l'assignation.
// ============================================================================

#include <juce_audio_processors/juce_audio_processors.h>
#include <map>

namespace zlabs
{

class MidiLearn
{
public:
    struct Mapping
    {
        int cc = -1;
        int channel = -1;
        juce::String paramID;
    };

    void startLearn(const juce::String& paramID) { learning = paramID; }
    void stopLearn() { learning = juce::String(); }
    bool isLearning() const { return learning.isNotEmpty(); }
    juce::String getLearningParam() const { return learning; }

    void clearAll() { mappings.clear(); }

    void handleCC(int channel, int ccNumber, float normalizedValue,
                 juce::AudioProcessorValueTreeState& apvts)
    {
        // Si on est en mode learn, on enregistre l'assignation
        if (learning.isNotEmpty())
        {
            mappings.push_back({ccNumber, channel, learning});
            learning = juce::String();
        }

        // Applique le CC à tous les params associés
        for (auto& m : mappings)
        {
            if (m.cc == ccNumber && (m.channel == -1 || m.channel == channel))
            {
                if (auto* p = apvts.getParameter(m.paramID))
                {
                    auto range = p->getNormalisableRange();
                    float value = range.convertFrom0to1(normalizedValue);
                    apvts.getParameter(m.paramID)->setValueNotifyingHost(normalizedValue);
                }
            }
        }
    }

    juce::String serializeMappings() const
    {
        juce::StringArray arr;
        for (auto& m : mappings)
            arr.add(juce::String(m.cc) + "|" + juce::String(m.channel) + "|" + m.paramID);
        return arr.joinIntoString(";");
    }

    void deserializeMappings(const juce::String& s)
    {
        mappings.clear();
        auto tokens = juce::StringArray::fromTokens(s, ";", "");
        for (auto& t : tokens)
        {
            auto parts = juce::StringArray::fromTokens(t, "|", "");
            if (parts.size() == 3)
                mappings.push_back({parts[0].getIntValue(), parts[1].getIntValue(), parts[2]});
        }
    }

    const std::vector<Mapping>& getMappings() const { return mappings; }

private:
    juce::String learning;
    std::vector<Mapping> mappings;
};

} // namespace zlabs
