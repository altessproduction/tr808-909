#pragma once
// ============================================================================
//  ParameterManager.h — Paramètres exposés au DAW via APVTS
//
//  Tous les paramètres ici sont persistés automatiquement dans le state du
//  plugin (preset/DAW recall) et peuvent être mappés via MIDI Learn.
// ============================================================================

#include <juce_audio_processors/juce_audio_processors.h>

namespace zlabs
{

struct ParameterIDs
{
    static constexpr const char* kMachineMode      = "machineMode";
    static constexpr const char* kMasterLevel      = "masterLevel";
    static constexpr const char* kDistoAmount      = "distoAmount";
    static constexpr const char* kDistoMix         = "distoMix";

    static constexpr const char* kKickTune         = "kickTune";
    static constexpr const char* kKickDecay        = "kickDecay";
    static constexpr const char* kKickLevel        = "kickLevel";

    static constexpr const char* kSnareTone        = "snareTone";
    static constexpr const char* kSnareSnappy     = "snareSnappy";
    static constexpr const char* kSnareDecay      = "snareDecay";
    static constexpr const char* kSnareLevel      = "snareLevel";

    static constexpr const char* kClapDecay        = "clapDecay";
    static constexpr const char* kClapLevel       = "clapLevel";

    static constexpr const char* kTomDecay         = "tomDecay";
    static constexpr const char* kTomLevel        = "tomLevel";

    static constexpr const char* kHatDecay         = "hatDecay";
    static constexpr const char* kHatLevel        = "hatLevel";

    static constexpr const char* kCymbalDecay      = "cymbalDecay";
    static constexpr const char* kCymbalLevel     = "cymbalLevel";

    static constexpr const char* kCowbellTune     = "cowbellTune";
    static constexpr const char* kCowbellLevel    = "cowbellLevel";

    // Séquenceur
    static constexpr const char* kSeqEnabled       = "seqEnabled";
    static constexpr const char* kSeqBpm           = "seqBpm";
    static constexpr const char* kSeqShuffle       = "seqShuffle";
    static constexpr const char* kSeqPatternIdx    = "seqPatternIdx";
    static constexpr const char* kSeqPatternsData  = "seqPatternsData";
};

class ParameterManager
{
public:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using namespace juce;
        std::vector<std::unique_ptr<RangedAudioParameter>> params;

        using F = AudioParameterFloat;
        auto fr = [](float mn, float mx, float st = 0.01f, float skew = 1.0f, bool symmetric = false) {
            return NormalisableRange<float>(mn, mx, st, skew, symmetric);
        };

        params.push_back(std::make_unique<AudioParameterChoice>(
            ParameterIDs::kMachineMode, "Machine",
            StringArray{"TR-808", "TR-909"}, 0));

        params.push_back(std::make_unique<F>(
            ParameterIDs::kMasterLevel, "Master", fr(0.0f, 1.5f), 0.85f));
        params.push_back(std::make_unique<F>(
            ParameterIDs::kDistoAmount, "Disto", fr(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<F>(
            ParameterIDs::kDistoMix, "Disto Mix", fr(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<F>(
            ParameterIDs::kKickTune, "Kick Tune", fr(-12.0f, 12.0f, 0.1f), 0.0f));
        params.push_back(std::make_unique<F>(
            ParameterIDs::kKickDecay, "Kick Decay", fr(0.05f, 2.0f, 0.01f, 0.4f), 0.5f));
        params.push_back(std::make_unique<F>(
            ParameterIDs::kKickLevel, "Kick Level", fr(0.0f, 1.5f), 1.0f));

        params.push_back(std::make_unique<F>(
            ParameterIDs::kSnareTone, "Snare Tone", fr(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<F>(
            ParameterIDs::kSnareSnappy, "Snare Snappy", fr(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<F>(
            ParameterIDs::kSnareDecay, "Snare Decay", fr(0.05f, 1.5f, 0.01f), 0.2f));
        params.push_back(std::make_unique<F>(
            ParameterIDs::kSnareLevel, "Snare Level", fr(0.0f, 1.5f), 0.9f));

        params.push_back(std::make_unique<F>(
            ParameterIDs::kClapDecay, "Clap Decay", fr(0.05f, 1.0f, 0.01f), 0.18f));
        params.push_back(std::make_unique<F>(
            ParameterIDs::kClapLevel, "Clap Level", fr(0.0f, 1.5f), 0.8f));

        params.push_back(std::make_unique<F>(
            ParameterIDs::kTomDecay, "Tom Decay", fr(0.05f, 1.5f, 0.01f), 0.25f));
        params.push_back(std::make_unique<F>(
            ParameterIDs::kTomLevel, "Tom Level", fr(0.0f, 1.5f), 0.8f));

        params.push_back(std::make_unique<F>(
            ParameterIDs::kHatDecay, "HH Decay", fr(0.02f, 1.0f, 0.01f), 0.05f));
        params.push_back(std::make_unique<F>(
            ParameterIDs::kHatLevel, "HH Level", fr(0.0f, 1.5f), 0.6f));

        params.push_back(std::make_unique<F>(
            ParameterIDs::kCymbalDecay, "Cymbal Decay", fr(0.05f, 3.0f, 0.01f, 0.4f), 1.0f));
        params.push_back(std::make_unique<F>(
            ParameterIDs::kCymbalLevel, "Cymbal Level", fr(0.0f, 1.5f), 0.5f));

        params.push_back(std::make_unique<F>(
            ParameterIDs::kCowbellTune, "Cowbell Tune", fr(-12.0f, 12.0f, 0.1f), 0.0f));
        params.push_back(std::make_unique<F>(
            ParameterIDs::kCowbellLevel, "Cowbell Level", fr(0.0f, 1.5f), 0.5f));

        params.push_back(std::make_unique<AudioParameterBool>(
            ParameterIDs::kSeqEnabled, "Seq On", false));
        params.push_back(std::make_unique<F>(
            ParameterIDs::kSeqBpm, "Seq BPM", fr(60.0f, 200.0f, 0.1f), 120.0f));
        params.push_back(std::make_unique<F>(
            ParameterIDs::kSeqShuffle, "Shuffle", fr(0.0f, 0.75f, 0.01f), 0.0f));
        params.push_back(std::make_unique<AudioParameterInt>(
            ParameterIDs::kSeqPatternIdx, "Pattern", 0, 15, 0));

        return { params.begin(), params.end() };
    }
};

} // namespace zlabs
