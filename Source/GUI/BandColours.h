#pragma once

#include <JuceHeader.h>

namespace BandColours
{
    inline constexpr int numBands = 5;

    inline juce::Colour getBandColour(int bandIndex)
    {
        static const std::array<juce::Colour, numBands> colours
        {
            juce::Colour(0xffee6677), // red/pink
            juce::Colour(0xff4477aa), // blue
            juce::Colour(0xff228833), // green
            juce::Colour(0xffccbb44), // yellow
            juce::Colour(0xffaa3377)  // purple
        };

        return colours[static_cast<size_t>(juce::jlimit(0, numBands - 1, bandIndex))];
    }

    inline juce::Colour getBandFillColour(int bandIndex)
    {
        return getBandColour(bandIndex).withAlpha(0.20f);
    }
}
