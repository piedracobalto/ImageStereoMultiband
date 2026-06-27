#include "MultibandSplitter.h"

void MultibandSplitter::prepare(const juce::dsp::ProcessSpec& spec)
{
    for (int i = 0; i < 4; ++i)
    {
        crossovers[i].prepare(spec);
        crossovers[i].setFrequency(frequencies[i]);
    }
}

void MultibandSplitter::setFrequency(int index, float frequency)
{
    frequencies[index] = frequency;
    crossovers[index].setFrequency(frequency);
}

void MultibandSplitter::process(
    const juce::AudioBuffer<float>& input,
    std::array<juce::AudioBuffer<float>, 5>& outputs)
{
    const int numCrossovers = activeBands - 1;

    for (int i = 0; i < activeBands; ++i)
    {
        outputs[i].setSize(
            input.getNumChannels(),
            input.getNumSamples(),
            false,
            false,
            true);
    }

    // Clear unused output buffers
    for (int i = activeBands; i < 5; ++i)
    {
        outputs[i].setSize(
            input.getNumChannels(),
            input.getNumSamples(),
            false,
            false,
            true);
        outputs[i].clear();
    }

    const int numSamples = input.getNumSamples();

    for (int s = 0; s < numSamples; ++s)
    {
        for (int c = 0; c < numCrossovers; ++c)
            crossovers[c].updateFrequency();

        float l = input.getSample(0, s);
        float r = input.getSample(1, s);

        // Cascading crossover processing
        for (int c = 0; c < numCrossovers; ++c)
        {
            auto [lowL, lowR] = crossovers[c].processLow(l, r);
            auto [highL, highR] = crossovers[c].processHigh(l, r);

            outputs[c].setSample(0, s, lowL);
            outputs[c].setSample(1, s, lowR);

            l = highL;
            r = highR;
        }

        // Last band = final highpass
        outputs[numCrossovers].setSample(0, s, l);
        outputs[numCrossovers].setSample(1, s, r);
    }
}