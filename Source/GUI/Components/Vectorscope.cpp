#include "Vectorscope.h"

Vectorscope::Vectorscope() {}

void Vectorscope::pushScopeData(const float* left, const float* right, int count)
{
    count = juce::jlimit(0, maxSamples, count);
    scopeCount = count;
    for (int i = 0; i < count; ++i)
    {
        auto l = left[i];
        auto r = right[i];
        scopeMid[i] = (l + r) * 0.5f;
        scopeSide[i] = (l - r) * 0.5f;
    }
}

void Vectorscope::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(juce::Colour(0xff20242a));
    g.fillRoundedRectangle(bounds, 8.0f);

    g.setColour(juce::Colour(0xff343941));
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

    auto area = getLocalBounds().reduced(12);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(14.0f));
    g.drawFittedText("Vectorscope", area.removeFromTop(22), juce::Justification::centredLeft, 1);

    auto scope = area.reduced(4, 6).toFloat();
    auto size = juce::jmin(scope.getWidth(), scope.getHeight()) * 0.45f;
    auto cx = scope.getCentreX();
    auto cy = scope.getCentreY();

    g.setColour(juce::Colour(0xff14181d));
    g.fillRoundedRectangle(scope, 6.0f);

    g.saveState();
    g.reduceClipRegion(scope.toNearestInt());

    // Crosshair
    g.setColour(juce::Colour(0xff252a31));
    g.drawLine(cx - size, cy, cx + size, cy, 0.8f);
    g.drawLine(cx, cy - size, cx, cy + size, 0.8f);

    // Reference squares (correlation rings in mid/side space)
    for (float r : { 0.5f, 1.0f })
    {
        auto radius = r * size;
        g.setColour(juce::Colour(0xff303944));
        g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 0.8f);
    }

    // Mid reference (mono) line
    g.setColour(juce::Colour(0xff303944).withAlpha(0.5f));
    g.drawLine(cx, cy - size * 0.707f, cx, cy + size * 0.707f, 0.5f);

    // Scope dots
    if (scopeCount > 0)
    {
        auto dotSize = 1.8f;
        auto colour = juce::Colour(0xff58c7d9);

        for (int i = 0; i < scopeCount; ++i)
        {
            auto x = cx + scopeSide[i] * size * 1.414f;
            auto y = cy - scopeMid[i] * size * 1.414f;

            if (x >= scope.getX() && x <= scope.getRight() &&
                y >= scope.getY() && y <= scope.getBottom())
            {
                auto alpha = juce::jmap<float>(i, 0, scopeCount - 1, 0.06f, 0.92f);
                g.setColour(colour.withAlpha(alpha));
                g.fillEllipse(x - dotSize, y - dotSize, dotSize * 2.0f, dotSize * 2.0f);
            }
        }
    }

    g.restoreState();

    g.setColour(juce::Colour(0xff6f7b87));
    g.setFont(juce::FontOptions(10.0f));
    g.drawFittedText("G", cx + 4, scope.getY() + 4, 16, 14, juce::Justification::centredLeft, 1);
    g.drawFittedText("W", scope.getX() + 4, cy + 4, 16, 14, juce::Justification::centredLeft, 1);
}
