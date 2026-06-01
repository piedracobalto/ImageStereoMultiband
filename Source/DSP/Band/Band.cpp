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
}

void Band::process(juce::AudioBuffer<float>& buffer)
{
    if (muted)
    {
        buffer.clear();
        return;
    }

    midSide.process(buffer);
    buffer.applyGain(gainLinear);
}

void Band::setWidth(float width)
{
    midSide.setSideGain(width);
}

void Band::setGain(float gainDb)
{
    gainLinear = juce::Decibels::decibelsToGain(gainDb);
}

void Band::setMute(bool shouldMute)
{
    muted = shouldMute;
}

void Band::setSolo(bool shouldSolo)
{
    solo = shouldSolo;
}

bool Band::isSolo() const
{
    return solo;
}