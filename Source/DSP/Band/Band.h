/*
  ==============================================================================

    Band.h
    Created: 30 May 2026 7:22:24pm
    Author:  Pedro

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "../Midside/MidSide.h"

class Band {
public:
    Band();
    ~Band();

    // por convecnion el metodo process se encarga de procesar el audio,y se ubica en PluginProcessor.cpp especialemente en processBlock,
    void prepare(juce::dsp::ProcessSpec spec);
    void process(juce::AudioBuffer<float>& buffer);

    void setWidth(float width);
    void setGain(float gainDb);
    void setMute(bool shouldMute);
    void setSolo(bool shouldSolo);
    bool isSolo() const;
    bool isMuted() const;

    void setLevelTarget(float gain);

private:
    Midside midSide;
    juce::SmoothedValue<float> bandGain;
    juce::SmoothedValue<float> levelGain;
    bool muted = false;
    bool solo = false;
};