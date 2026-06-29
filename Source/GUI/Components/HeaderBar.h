#pragma once

#include <JuceHeader.h>

class HeaderBar : public juce::Component
{
public:
    HeaderBar();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label titleLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderBar)
};
