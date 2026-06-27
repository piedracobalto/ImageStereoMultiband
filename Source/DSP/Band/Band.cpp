/*
  ==============================================================================

    Band.cpp
    Created: 30 May 2026 7:22:24pm
    Author:  Pedro

  ==============================================================================
*/

#include "Band.h"

Band::Band() {};
Band::~Band() {};

void Band::prepare(juce::dsp::ProcessSpec spec)
{
    midSide.prepare(spec);

    bandGain.reset(spec.sampleRate, 0.02);
    bandGain.setCurrentAndTargetValue(1.0f);

    levelGain.reset(spec.sampleRate, 0.02);
    levelGain.setCurrentAndTargetValue(1.0f);
}

void Band::process(juce::AudioBuffer<float>& buffer)
{
    midSide.process(buffer);

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        auto gain =
            bandGain.getNextValue();

        auto level =
            levelGain.getNextValue();

        auto totalGain = gain * level;

        left[sample] *= totalGain;
        right[sample] *= totalGain;
    }
}

void Band::setWidth(float width)
{
    midSide.setSideGain(width * 0.02f);
}

void Band::setGain(float gainDb)
{
    bandGain.setTargetValue(
        juce::Decibels::decibelsToGain(gainDb));
}

void Band::setMute(bool shouldMute)
{
    muted = shouldMute;
}

void Band::setSolo(bool shouldSolo)
{
    solo = shouldSolo;
}

bool Band::isMuted() const
{
    return muted;
}

bool Band::isSolo() const
{
    return solo;
}

void Band::setLevelTarget(float gain)
{
    levelGain.setTargetValue(gain);
}