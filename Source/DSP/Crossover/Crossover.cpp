/*
  ==============================================================================

    Crossover.cpp
    Created: 30 May 2026 7:03:38pm
    Author:  Pedro

  ==============================================================================
*/

#include "Crossover.h"

#include "Crossover.h"

void Crossover::prepare(const juce::dsp::ProcessSpec& spec)
{
    lowLeft.prepare(spec);
    lowRight.prepare(spec);

    highLeft.prepare(spec);
    highRight.prepare(spec);

    lowLeft.setType(
        juce::dsp::LinkwitzRileyFilterType::lowpass);

    lowRight.setType(
        juce::dsp::LinkwitzRileyFilterType::lowpass);

    highLeft.setType(
        juce::dsp::LinkwitzRileyFilterType::highpass);

    highRight.setType(
        juce::dsp::LinkwitzRileyFilterType::highpass);

    setFrequency(crossoverFrequency);
}

void Crossover::setFrequency(float frequency)
{
    crossoverFrequency = frequency;

    lowLeft.setCutoffFrequency(frequency);
    lowRight.setCutoffFrequency(frequency);

    highLeft.setCutoffFrequency(frequency);
    highRight.setCutoffFrequency(frequency);
}

void Crossover::process(
    const juce::AudioBuffer<float>& input,
    juce::AudioBuffer<float>& lowBand,
    juce::AudioBuffer<float>& highBand)
{
    lowBand.makeCopyOf(input);
    highBand.makeCopyOf(input);

    auto numSamples = input.getNumSamples();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        lowBand.setSample(
            0,
            sample,
            lowLeft.processSample(
                0,
                lowBand.getSample(0, sample)));

        lowBand.setSample(
            1,
            sample,
            lowRight.processSample(
                1,
                lowBand.getSample(1, sample)));

        highBand.setSample(
            0,
            sample,
            highLeft.processSample(
                0,
                highBand.getSample(0, sample)));

        highBand.setSample(
            1,
            sample,
            highRight.processSample(
                1,
                highBand.getSample(1, sample)));
    }
}