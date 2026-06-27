#pragma once

#include <JuceHeader.h>

class MultibandSplitter
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);

    void setFrequency(int index, float frequency);

    void setNumBands(int n) { activeBands = n; }

    void process(
        const juce::AudioBuffer<float>& input,
        std::array<juce::AudioBuffer<float>, 5>& outputs);

private:
    int activeBands = 5;

    // Un par L/R de filtros LR4 por cada crossover
    struct CrossoverPair
    {
        juce::dsp::LinkwitzRileyFilter<float> lowL, lowR;
        juce::dsp::LinkwitzRileyFilter<float> highL, highR;

        juce::SmoothedValue<float> frequency;

        void prepare(const juce::dsp::ProcessSpec& spec)
        {
            lowL.prepare(spec);
            lowR.prepare(spec);

            highL.prepare(spec);
            highR.prepare(spec);

            lowL.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
            lowR.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);

            highL.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
            highR.setType(juce::dsp::LinkwitzRileyFilterType::highpass);

            frequency.reset(spec.sampleRate, 0.05); // 50 ms
            frequency.setCurrentAndTargetValue(1000.0f);
        }

        void setFrequency(float f)
        {
            frequency.setTargetValue(f);
        }

        void updateFrequency()
        {
            auto f = frequency.getNextValue();

            lowL.setCutoffFrequency(f);
            lowR.setCutoffFrequency(f);

            highL.setCutoffFrequency(f);
            highR.setCutoffFrequency(f);
        }

        std::pair<float, float> processLow(float l, float r)
        {
            return {
                lowL.processSample(0, l),
                lowR.processSample(1, r)
            };
        }

        std::pair<float, float> processHigh(float l, float r)
        {
            return {
                highL.processSample(0, l),
                highR.processSample(1, r)
            };
        }
    };

    std::array<float, 4> frequencies{ 120.f, 500.f, 2000.f, 8000.f };
    std::array<CrossoverPair, 4> crossovers;
};
