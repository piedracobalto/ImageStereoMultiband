#pragma once

#include <JuceHeader.h>

namespace BandColours
{
    inline constexpr int numBands = 5;

    inline juce::Colour getBandColour(int bandIndex)
    {
        static const std::array<juce::Colour, numBands> colours
        {
            juce::Colour(0xffff3b30), // infrared / lows
            juce::Colour(0xffff9500),
            juce::Colour(0xffffd60a),
            juce::Colour(0xff00c7ff),
            juce::Colour(0xff9b5cff)  // ultraviolet / highs
        };

        return colours[static_cast<size_t>(juce::jlimit(0, numBands - 1, bandIndex))];
    }

    inline juce::Colour getBandFillColour(int bandIndex)
    {
        return getBandColour(bandIndex).withAlpha(0.20f);
    }
}
