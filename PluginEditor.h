#pragma once
// ============================================================================
//  PluginEditor.h — Interface graphique du plugin
//
//  UI vintage fidèle : reprise de la façade des vraies 808/909.
//  - Layout 16×11 grid de pads (instruments × steps)
//  - Boutons rotatifs pour les paramètres (kick, snare, etc.)
//  - Switch machine (808 / 909) avec repaint thème (crème vs gris/rouge)
// ============================================================================

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace zlabs
{

class PluginEditor  : public juce::AudioProcessorEditor,
                     private juce::Timer,
                     private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit PluginEditor(TR808_909AudioProcessor&);
    ~PluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    void parameterChanged(const juce::String& paramID, float newValue) override;

    TR808_909AudioProcessor& proc;

    // ─────── Controls ───────
    juce::Label titleLabel;
    juce::TextButton btn808 {"TR-808"}, btn909 {"TR-909"};

    // Master section
    juce::Slider sMaster, sDisto, sDistoMix;
    juce::Label  lMaster {"", "Master"}, lDisto {"", "Saturation"}, lDistoMix {"", "Mix Satu."};

    // Voice params
    struct Knob {
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::Label>  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
        Knob(const char* id, const char* txt, juce::AudioProcessorValueTreeState& apvts)
        {
            slider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                                    juce::Slider::TextBoxBelow);
            slider->setTextBoxIsEditable(true);
            slider->setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
            slider->setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff3a2a1a));
            label  = std::make_unique<juce::Label>("", txt);
            label->setJustificationType(juce::Justification::centred);
            label->setColour(juce::Label::textColourId, juce::Colours::white);
            attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, id, *slider);
        }
    };

    std::vector<std::unique_ptr<Knob>> knobs;

    // ─────── Step sequencer pads ───────
    struct Pad : public juce::Button
    {
        int instrument = 0;
        int step = 0;
        bool accent = false;
        Pad() : juce::Button("") {}
        void paintButton(juce::Graphics& g, bool /*isHighlighted*/, bool /*isDown*/) override
        {
            auto r = getLocalBounds().reduced(2);
            juce::Colour on  = accent ? juce::Colour(0xffe07020) : juce::Colour(0xffe8c040);
            juce::Colour off = juce::Colour(0xff3a2a1a);
            g.setColour(getToggleState() ? on : off);
            g.fillRoundedRectangle(r.toFloat(), 3.0f);
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            g.drawRoundedRectangle(r.toFloat(), 3.0f, 1.0f);
        }
    };
    std::array<std::array<std::unique_ptr<Pad>, 16>, 11> pads;
    juce::Label stepNumbers;
    juce::Label instrumentLabels;

    // ─────── Sequencer controls ───────
    juce::ToggleButton btnSeqEnable {"Seq On"};
    juce::Slider sBpm, sShuffle;
    juce::Slider sPattern;
    juce::Label  lBpm {"", "BPM"}, lShuffle {"", "Shuffle"}, lPattern {"", "Pattern"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seqEnableAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bpmAtt, shuffleAtt, patternAtt;

    // ─────── MIDI learn ───────
    juce::TextButton btnLearn {"Apprendre MIDI"};
    juce::Label     learnStatus {"", "—"};

    // Theme colors
    juce::Colour bg808  = juce::Colour(0xffd4c0a0);   // crème 808
    juce::Colour bg909  = juce::Colour(0xff8a8a8a);   // gris 909
    juce::Colour accent808 = juce::Colour(0xffe07020);
    juce::Colour accent909 = juce::Colour(0xffc02020);

    bool current808 = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};

} // namespace zlabs
