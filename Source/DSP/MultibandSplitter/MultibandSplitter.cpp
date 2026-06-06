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
    for (auto& band : outputs)
    {
        band.setSize(
            input.getNumChannels(),
            input.getNumSamples(),
            false,
            false,
            true);
    }


    const int numSamples = input.getNumSamples();

    for (int s = 0; s < numSamples; ++s)
    {
        for (auto& crossover : crossovers)
            crossover.updateFrequency();


        float l = input.getSample(0, s);
        float r = input.getSample(1, s);

        auto [l0, r0] = crossovers[0].processLow(l, r);
        auto [lH0, rH0] = crossovers[0].processHigh(l, r);

        auto [l1, r1] = crossovers[1].processLow(lH0, rH0);
        auto [lH1, rH1] = crossovers[1].processHigh(lH0, rH0);

        auto [l2, r2] = crossovers[2].processLow(lH1, rH1);
        auto [lH2, rH2] = crossovers[2].processHigh(lH1, rH1);

        auto [l3, r3] = crossovers[3].processLow(lH2, rH2);
        auto [l4, r4] = crossovers[3].processHigh(lH2, rH2);

        outputs[0].setSample(0, s, l0);
        outputs[0].setSample(1, s, r0);

        outputs[1].setSample(0, s, l1);
        outputs[1].setSample(1, s, r1);

        outputs[2].setSample(0, s, l2);
        outputs[2].setSample(1, s, r2);

        outputs[3].setSample(0, s, l3);
        outputs[3].setSample(1, s, r3);

        outputs[4].setSample(0, s, l4);
        outputs[4].setSample(1, s, r4);
    }
}