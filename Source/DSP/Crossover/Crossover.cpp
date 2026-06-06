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

    frequencySmoothed.reset(spec.sampleRate, 0.05); // 50 ms
    frequencySmoothed.setCurrentAndTargetValue(crossoverFrequency);

    setFrequency(crossoverFrequency);
}

void Crossover::setFrequency(float frequency)
{
    crossoverFrequency = frequency;
    frequencySmoothed.setTargetValue(frequency);
}

void Crossover::process(
    const juce::AudioBuffer<float>& input,
    juce::AudioBuffer<float>& lowBand,
    juce::AudioBuffer<float>& highBand)
{
    lowBand.makeCopyOf(input);
    highBand.makeCopyOf(input);

    auto numSamples = input.getNumSamples();

    auto cutoff = frequencySmoothed.getNextValue();

    lowLeft.setCutoffFrequency(cutoff);
    lowRight.setCutoffFrequency(cutoff);
    highLeft.setCutoffFrequency(cutoff);
    highRight.setCutoffFrequency(cutoff);
}