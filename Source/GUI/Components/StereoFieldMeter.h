#pragma once

#include <JuceHeader.h>
#include "../BandColours.h"

class StereoFieldMeter : public juce::Component,
                         private juce::AudioProcessorValueTreeState::Listener,
                         private juce::AsyncUpdater
{
public:
    explicit StereoFieldMeter(juce::AudioProcessorValueTreeState& apvts);
    ~StereoFieldMeter() override;

    void paint(juce::Graphics& g) override;

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;

    juce::AudioProcessorValueTreeState& valueTreeState;
    std::array<juce::String, BandColours::numBands> widthParameterIDs;
    std::array<std::atomic<float>, BandColours::numBands> widths;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StereoFieldMeter)
};
