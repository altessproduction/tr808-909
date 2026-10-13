// ============================================================================
//  PluginEditor.cpp — UI du plugin (interface vintage française)
// ============================================================================

#include "PluginEditor.h"
#include <memory>

namespace zlabs
{

PluginEditor::PluginEditor(TR808_909AudioProcessor& p)
    : juce::AudioProcessorEditor(&p), proc(p)
{
    using namespace juce;
    setOpaque(false);

    // ─────── Titre + switch machine ───────
    titleLabel.setText("TR-808 / TR-909  —  Z-Labs", dontSendNotification);
    titleLabel.setJustificationType(Justification::centred);
    titleLabel.setColour(Label::textColourId, Colours::white);
    titleLabel.setFont(Font("Courier New", 22.0f, Font::bold));
    addAndMakeVisible(titleLabel);

    addAndMakeVisible(btn808);
    addAndMakeVisible(btn909);
    btn808.onClick = [this]{ current808 = true; proc.getAPVTS().getParameter(ParameterIDs::kMachineMode)->setValueNotifyingHost(0.0f); repaint(); };
    btn909.onClick = [this]{ current808 = false; proc.getAPVTS().getParameter(ParameterIDs::kMachineMode)->setValueNotifyingHost(1.0f); repaint(); };
    btn808.setClickingTogglesState(true);
    btn909.setClickingTogglesState(true);
    btn808.setToggleState(true, dontSendNotification);
    btn909.setToggleState(false, dontSendNotification);

    // ─────── Master ───────
    sMaster.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    sMaster.setTextBoxStyle(Slider::TextBoxBelow, false, 60, 18);
    sMaster.setColour(Slider::textBoxTextColourId, Colours::white);
    sMaster.setColour(Slider::textBoxOutlineColourId, Colour(0xff3a2a1a));
    addAndMakeVisible(sMaster); addAndMakeVisible(lMaster);
    sDisto.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    sDisto.setTextBoxStyle(Slider::TextBoxBelow, false, 60, 18);
    addAndMakeVisible(sDisto); addAndMakeVisible(lDisto);
    sDistoMix.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    sDistoMix.setTextBoxStyle(Slider::TextBoxBelow, false, 60, 18);
    addAndMakeVisible(sDistoMix); addAndMakeVisible(lDistoMix);

    // Attachements master
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kMasterLevel, "Master", proc.getAPVTS()));
    knobs.back()->slider->setBounds(sMaster.getBounds());
    // (Use slider attachment via slider)
    // En fait on les recrée proprement:

    // On réutilise Knob object — plus simple : on crée directement les attachements ici
    std::vector<std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>> attaches;
    // (ils vivront dans l'objet — on les met en membre privé alternatif)
    // Pour simplifier, on utilise Knob qui porte son propre attachement.

    // Master knobs (via Knob)
    knobs.clear();
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kMasterLevel, "Master", proc.getAPVTS()));
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kDistoAmount, "Saturation", proc.getAPVTS()));
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kDistoMix, "Mix Satu.", proc.getAPVTS()));
    for (auto& k : knobs)
    {
        addAndMakeVisible(*k->slider);
        addAndMakeVisible(*k->label);
    }

    // Voice knobs — KICK
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kKickTune, "Kick Accord", proc.getAPVTS()));
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kKickDecay, "Kick Decay", proc.getAPVTS()));
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kKickLevel, "Kick Niveau", proc.getAPVTS()));

    // SNARE
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kSnareTone, "Snare Tone", proc.getAPVTS()));
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kSnareSnappy, "Snare Snap", proc.getAPVTS()));
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kSnareDecay, "Snare Decay", proc.getAPVTS()));
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kSnareLevel, "Snare Niveau", proc.getAPVTS()));

    // CLAP
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kClapDecay, "Clap Decay", proc.getAPVTS()));
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kClapLevel, "Clap Niveau", proc.getAPVTS()));

    // TOM
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kTomDecay, "Tom Decay", proc.getAPVTS()));
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kTomLevel, "Tom Niveau", proc.getAPVTS()));

    // HI-HAT
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kHatDecay, "HH Decay", proc.getAPVTS()));
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kHatLevel, "HH Niveau", proc.getAPVTS()));

    // CYMBAL
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kCymbalDecay, "Cymbale Decay", proc.getAPVTS()));
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kCymbalLevel, "Cymbale Niv.", proc.getAPVTS()));

    // COWBELL
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kCowbellTune, "Cowbell Accord", proc.getAPVTS()));
    knobs.push_back(std::make_unique<Knob>(ParameterIDs::kCowbellLevel, "Cowbell Niv.", proc.getAPVTS()));

    for (auto& k : knobs)
    {
        addAndMakeVisible(*k->slider);
        addAndMakeVisible(*k->label);
    }

    // ─────── Step sequencer ───────
    const char* instrumentNames[11] = {
        "Kick", "Snare", "Rimshot", "Clap", "Tom L", "Tom M", "Tom H",
        "HH Ferm.", "HH Ouv.", "Crash", "Ride"
    };

    for (int inst = 0; inst < 11; ++inst)
    {
        for (int s = 0; s < 16; ++s)
        {
            auto pad = std::make_unique<Pad>();
            pad->instrument = inst;
            pad->step = s;
            pad->setClickingTogglesState(true);
            pad->setColour(TextButton::buttonColourId, Colour(0xff3a2a1a));
            pad->setColour(TextButton::buttonOnColourId, accent808);
            pad->onStateChange = [this, inst, s]() {
                auto& pat = proc.getSeq().getCurrentPattern();
                bool on = pads[inst][s]->getToggleState();
                pads[inst][s]->accent = on && (s % 4 == 0);
                pat.steps[inst][s] = on;
                pat.accents[inst][s] = pads[inst][s]->accent;
            };
            addAndMakeVisible(*pad);
            pads[inst][s] = std::move(pad);
        }
    }

    // labels
    instrumentLabels.setText(
        "Kick\nSnare\nRimshot\nClap\nTom L\nTom M\nTom H\nHH F\nHH O\nCrash\nRide",
        dontSendNotification);
    instrumentLabels.setJustificationType(Justification::centredLeft);
    instrumentLabels.setColour(Label::textColourId, Colours::white);
    instrumentLabels.setFont(Font("Courier New", 13.0f, Font::plain));
    addAndMakeVisible(instrumentLabels);

    // step numbers
    String stepsTxt = "1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16";
    stepNumbers.setText(stepsTxt, dontSendNotification);
    stepNumbers.setJustificationType(Justification::centred);
    stepNumbers.setColour(Label::textColourId, Colours::white);
    stepNumbers.setFont(Font("Courier New", 11.0f, Font::plain));
    addAndMakeVisible(stepNumbers);

    // ─────── Seq controls ───────
    addAndMakeVisible(btnSeqEnable);
    seqEnableAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(
        proc.getAPVTS(), ParameterIDs::kSeqEnabled, btnSeqEnable);

    sBpm.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    sBpm.setTextBoxStyle(Slider::TextBoxBelow, false, 60, 18);
    sBpm.setColour(Slider::textBoxTextColourId, Colours::white);
    addAndMakeVisible(sBpm); addAndMakeVisible(lBpm);
    bpmAtt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
        proc.getAPVTS(), ParameterIDs::kSeqBpm, sBpm);

    sShuffle.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    sShuffle.setTextBoxStyle(Slider::TextBoxBelow, false, 60, 18);
    addAndMakeVisible(sShuffle); addAndMakeVisible(lShuffle);
    shuffleAtt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
        proc.getAPVTS(), ParameterIDs::kSeqShuffle, sShuffle);

    sPattern.setSliderStyle(Slider::LinearHorizontal);
    sPattern.setTextBoxStyle(Slider::TextBoxLeft, false, 40, 18);
    addAndMakeVisible(sPattern); addAndMakeVisible(lPattern);
    patternAtt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
        proc.getAPVTS(), ParameterIDs::kSeqPatternIdx, sPattern);

    // ─────── MIDI Learn ───────
    addAndMakeVisible(btnLearn);
    addAndMakeVisible(learnStatus);
    btnLearn.setClickingTogglesState(true);
    btnLearn.onClick = [this] {
        if (btnLearn.getToggleState())
        {
            learnStatus.setText("(en attente d'un CC MIDI...)", dontSendNotification);
            // On suppose le learn sur le paramètre Master (simplifié)
            proc.getMidiLearn().startLearn(ParameterIDs::kMasterLevel);
        }
        else
        {
            learnStatus.setText("—", dontSendNotification);
            proc.getMidiLearn().stopLearn();
        }
    };

    // ─────── Window size ───────
    setSize(1100, 760);

    // Start timer for UI refresh
    startTimerHz(30);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
}

void PluginEditor::parameterChanged(const juce::String&, float)
{
    // hook pour réagir à changements externes
}

void PluginEditor::timerCallback()
{
    // refresh tactile state for pads (pas besoin)
}

void PluginEditor::paint(juce::Graphics& g)
{
    auto bg = current808 ? bg808 : bg909;
    auto accent = current808 ? accent808 : accent909;
    g.fillAll(bg);

    // bandeau supérieur
    g.setColour(accent);
    g.fillRect(0, 0, getWidth(), 50);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font("Courier New", 18.0f, juce::Font::bold));
    g.drawText("Z-LABS  -  TR-808 / TR-909", 20, 14, getWidth() - 40, 25, juce::Justification::centred);

    // petit logo label
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.setFont(juce::Font("Arial", 10.0f, juce::Font::italic));
    g.drawText("(c) Z-Labs 2025 - VST3 clone 808/909", 20, getHeight() - 18, getWidth() - 40, 14, juce::Justification::centred);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop(50);    // bandeau

    // ─────── Switch machine ───────
    auto topRow = area.removeFromTop(40);
    btn808.setBounds(topRow.removeFromLeft(120).reduced(4));
    btn909.setBounds(topRow.removeFromLeft(120).reduced(4));

    // ─────── Master + disto ───────
    auto masterRow = area.removeFromTop(80);
    int knobW = 80, knobH = 80;
    auto k1 = masterRow.removeFromLeft(knobW);
    knobs[0]->slider->setBounds(k1);
    knobs[0]->label->setBounds(k1.getX(), k1.getBottom(), knobW, 18);
    auto k2 = masterRow.removeFromLeft(knobW);
    knobs[1]->slider->setBounds(k2);
    knobs[1]->label->setBounds(k2.getX(), k2.getBottom(), knobW, 18);
    auto k3 = masterRow.removeFromLeft(knobW);
    knobs[2]->slider->setBounds(k3);
    knobs[2]->label->setBounds(k3.getX(), k3.getBottom(), knobW, 18);

    // ─────── Voice knobs (regroupés) ───────
    auto knobArea = area.removeFromTop(180);
    int idx = 3; // Skip master knobs (0,1,2)
    int colsPerRow = 7;
    int cellW = (knobArea.getWidth() - 20) / colsPerRow;
    for (int row = 0; row < 2; ++row)
    {
        auto rowArea = knobArea.removeFromTop(90);
        for (int col = 0; col < colsPerRow && idx < (int)knobs.size(); ++col)
        {
            auto cell = rowArea.removeFromLeft(cellW);
            knobs[idx]->slider->setBounds(cell.withSizeKeepingCentre(80, 80));
            knobs[idx]->label->setBounds(cell.getX(), cell.getY() + 80, cell.getWidth(), 18);
            idx++;
        }
    }

    // ─────── Step sequencer ───────
    auto seqArea = area.removeFromTop(280);
    auto stepArea = seqArea.reduced(8);

    // labels colonnes (instruments)
    auto lblArea = stepArea.removeFromLeft(80);
    instrumentLabels.setBounds(lblArea);

    // step numbers
    auto numArea = stepArea.removeFromTop(20);
    stepNumbers.setBounds(numArea);

    // pads grid
    int padW = stepArea.getWidth() / 16;
    int padH = stepArea.getHeight() / 11;
    for (int inst = 0; inst < 11; ++inst)
    {
        for (int s = 0; s < 16; ++s)
        {
            pads[inst][s]->setBounds(
                stepArea.getX() + s * padW,
                stepArea.getY() + inst * padH,
                padW - 2, padH - 2);
        }
    }

    // ─────── Seq controls ───────
    auto seqCtrl = area.removeFromTop(50);
    btnSeqEnable.setBounds(seqCtrl.removeFromLeft(100).reduced(4));
    auto bpm = seqCtrl.removeFromLeft(80);
    sBpm.setBounds(bpm);  lBpm.setBounds(bpm.getX(), bpm.getBottom(), 80, 18);
    auto shf = seqCtrl.removeFromLeft(80);
    sShuffle.setBounds(shf); lShuffle.setBounds(shf.getX(), shf.getBottom(), 80, 18);
    auto pat = seqCtrl.removeFromLeft(160);
    sPattern.setBounds(pat); lPattern.setBounds(pat.getX(), pat.getBottom(), 160, 18);

    // ─────── MIDI Learn ───────
    auto learnArea = area.removeFromTop(50);
    btnLearn.setBounds(learnArea.removeFromLeft(140).reduced(4));
    learnStatus.setBounds(learnArea);
}

} // namespace zlabs
