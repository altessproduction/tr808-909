#pragma once
// ============================================================================
//  DistortionStage.h — disto/saturation douce tanh sur le bus de sortie
// ============================================================================

namespace zlabs
{

class DistortionStage
{
public:
    void prepare(double /*sampleRate*/) {}

    void setAmount(float a) noexcept
    {
        // 0..1 → drive 1..10
        drive = juce::jmap(a, 0.0f, 1.0f, 1.0f, 10.0f);
    }

    void setMix(float m) noexcept
    {
        mix = juce::jlimit(0.0f, 1.0f, m);
    }

    inline float process(float x) noexcept
    {
        float distorted = std::tanh(drive * x) / std::tanh(drive);
        return mix * distorted + (1.0f - mix) * x;
    }

private:
    float drive = 1.0f;
    float mix = 0.0f;
};

} // namespace zlabs
