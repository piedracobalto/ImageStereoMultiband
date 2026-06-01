/*
  ==============================================================================

    MidSide.h
    Created: 30 May 2026 7:06:04pm
    Author:  Pedro

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class Midside {
public:
    Midside();
    ~Midside();

    // por convecnion el metodo process se encarga de procesar el audio,y se ubica en PluginProcessor.cpp especialemente en processBlock,
	void prepare(juce::dsp::ProcessSpec spec);
    void process(juce::AudioBuffer<float>& buffer);
    void setMidGain(float midGain);
    void setSideGain(float sideGain);

private:
	double sampleRate{ 44100.0 };
	float midGain  = 1.0f;
    float sideGain = 1.0f;
};