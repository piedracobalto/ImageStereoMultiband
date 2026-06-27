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
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void setHasOtherSolo(bool v);

private:
    int bandNumber = 1;
    juce::Colour accentColour;

    juce::Label titleLabel;
    juce::Label gainLabel;
    juce::Slider widthSlider;
    juce::Slider gainSlider;
    juce::Label gainValueLabel;
    juce::Label widthLabel;
    juce::Label widthValueLabel;
    juce::TextButton muteButton { "M" };
    juce::TextButton soloButton { "S" };

    std::unique_ptr<SliderAttachment> widthAttachment;
    std::unique_ptr<SliderAttachment> gainAttachment;
    std::unique_ptr<ButtonAttachment> muteAttachment;
    std::unique_ptr<ButtonAttachment> soloAttachment;
    bool hasOtherSolo = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BandStrip)
};
