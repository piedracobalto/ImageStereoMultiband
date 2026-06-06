/*
  ==============================================================================

    MidSide.cpp
    Created: 30 May 2026 7:06:04pm
    Author:  Pedro

  ==============================================================================
*/

#include "MidSide.h"

Midside::Midside() {};
Midside::~Midside() {};

void Midside::prepare(juce::dsp::ProcessSpec spec) {
	sampleRate = spec.sampleRate;
}

void Midside::process(juce::AudioBuffer<float>& buffer)
{
    auto numSamples = buffer.getNumSamples();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        auto left  = buffer.getSample(0, sample);
        auto right = buffer.getSample(1, sample);

        smoothMid = smoothMid - (0.002f * (smoothMid - midGain));
        smoothSide = smoothSide - (0.002f * (smoothSide - sideGain));

        auto mid  = (left + right) * smoothMid / std::sqrt(2.0f);
        auto side = (left - right) * smoothSide / std::sqrt(2.0f);

        auto newLeft  = (mid + side) / std::sqrt(2.0f);
        auto newRight = (mid - side) / std::sqrt(2.0f);

        buffer.setSample(0, sample, newLeft);
        buffer.setSample(1, sample, newRight);
    }
}

void Midside::setMidGain(float gain)
{
    midGain = gain;
}

void Midside::setSideGain(float gain)
{
    sideGain = gain;
}