#pragma once

#include <JuceHeader.h>
#include "../BandColours.h"

class BandStrip : public juce::Component
{
public:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    BandStrip(juce::AudioProcessorValueTreeState& apvts, int bandIndex);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    static void configureSlider(juce::Slider& slider);

    int bandNumber = 1;
    juce::Colour accentColour;

    juce::Label titleLabel;
    juce::Label widthLabel;
    juce::Label gainLabel;
    juce::Slider widthSlider;
    juce::Slider gainSlider;
    juce::TextButton muteButton { "M" };
    juce::TextButton soloButton { "S" };

    std::unique_ptr<SliderAttachment> widthAttachment;
    std::unique_ptr<SliderAttachment> gainAttachment;
    std::unique_ptr<ButtonAttachment> muteAttachment;
    std::unique_ptr<ButtonAttachment> soloAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BandStrip)
};
