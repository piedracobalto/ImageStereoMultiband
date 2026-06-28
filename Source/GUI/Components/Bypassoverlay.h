/*
  ==============================================================================

    Bypassoverlay.h
    Created: 27 Jun 2026 8:04:40pm
    Author:  Pedro

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

/**
 * Transparent overlay that sits on top of the entire editor when bypassed.
 * It intercepts all mouse events (so the UI is not clickable while bypassed)
 * and paints a dark semi-opaque veil + "BYPASSED" label.
 */
class BypassOverlay : public juce::Component
{
public:
    BypassOverlay()
    {
        // Intercept all mouse events so controls underneath are unreachable
        setInterceptsMouseClicks(true, true);
        setRepaintsOnMouseActivity(false);
    }

    void paint(juce::Graphics& g) override
    {
        // Dark veil — opaque enough to hide all graphics underneath
        g.setColour(juce::Colour(0xd0141820));   // ~82 % opacity dark navy
        g.fillRect(getLocalBounds());

        // "BYPASSED" label centred
        g.setColour(juce::Colour(0xccffffff));
        g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
        g.drawFittedText("BYPASSED",
                         getLocalBounds(),
                         juce::Justification::centred,
                         1);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BypassOverlay)
};