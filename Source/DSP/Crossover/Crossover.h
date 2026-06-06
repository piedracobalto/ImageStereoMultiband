/*
  ==============================================================================

    Crossover.h
    Created: 30 May 2026 7:03:38pm
    Author:  Pedro

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class Crossover
{
public:

    void prepare(const juce::dsp::ProcessSpec& spec);

    void setFrequency(float frequency);

    void process(const juce::AudioBuffer<float>& input,
        juce::AudioBuffer<float>& lowBand,
        juce::AudioBuffer<float>& highBand);

private:

    float crossoverFrequency = 1000.0f;
    juce::SmoothedValue<float> frequencySmoothed;

    juce::dsp::LinkwitzRileyFilter<float> lowLeft;
    juce::dsp::LinkwitzRileyFilter<float> lowRight;

    juce::dsp::LinkwitzRileyFilter<float> highLeft;
    juce::dsp::LinkwitzRileyFilter<float> highRight;
};
