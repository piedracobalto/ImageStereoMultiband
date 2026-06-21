#include "Vectorscope.h"

Vectorscope::Vectorscope()
    : zoomInBtn("+"), zoomOutBtn("-"), colorBtn("W")
{
    auto styleBtn = [](juce::TextButton& btn)
    {
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2f36));
        btn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3a414a));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8a9ba8));
        btn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    };

    addAndMakeVisible(zoomInBtn);
    styleBtn(zoomInBtn);
    zoomInBtn.onClick = [this]
    {
        zoomFactor = juce::jmin(8.0f, zoomFactor + 0.5f);
        repaint();
    };

    addAndMakeVisible(zoomOutBtn);
    styleBtn(zoomOutBtn);
    zoomOutBtn.onClick = [this]
    {
        zoomFactor = juce::jmax(0.5f, zoomFactor - 0.5f);
        repaint();
    };

    addAndMakeVisible(colorBtn);
    styleBtn(colorBtn);
    colorBtn.onClick = [this]
    {
        multiColor = !multiColor;
        colorBtn.setButtonText(multiColor ? "C" : "W");
        repaint();
    };
}

Vectorscope::~Vectorscope() {}

void Vectorscope::pushScopeData(const float* left, const float* right, int count)
{
    anyScopeCount = 0;
    count = juce::jlimit(0, maxSamples, count);

    auto& bs = bandScopes[0];
    bs.count = count;
    for (int i = 0; i < count; ++i)
    {
        auto l = left[i];
        auto r = right[i];
        bs.mid[i] = (l + r) * 0.5f;
        bs.side[i] = (l - r) * 0.5f;
    }
    anyScopeCount = count;
}

void Vectorscope::pushBandScope(int bandIndex, const float* left, const float* right, int count)
{
    if (bandIndex < 0 || bandIndex >= numBands)
        return;

    auto& bs = bandScopes[static_cast<size_t>(bandIndex)];
    bs.count = juce::jlimit(0, maxSamples, count);

    for (int i = 0; i < bs.count; ++i)
    {
        auto l = left[i];
        auto r = right[i];
        bs.mid[i] = (l + r) * 0.5f;
        bs.side[i] = (l - r) * 0.5f;
    }

    int maxCount = 0;
    for (auto& b : bandScopes)
        if (b.count > maxCount)
            maxCount = b.count;
    anyScopeCount = maxCount;

}

void Vectorscope::clearScopes()
{
    anyScopeCount = 0;
    for (auto& bs : bandScopes)
        bs.count = 0;
    repaint();
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

    // Toolbar: buttons + zoom label
    auto toolbar = area.removeFromTop(20);
    g.setFont(juce::FontOptions(11.0f));
    g.setColour(juce::Colour(0xff8a9ba8));
    auto zoomLabelX = zoomInBtn.getBounds().getRight() + 4;
    g.drawFittedText("x" + juce::String(zoomFactor, 1),
                     zoomLabelX, toolbar.getY(),
                     50, toolbar.getHeight(),
                     juce::Justification::centredLeft, 1);

    auto scope = area.reduced(4, 6).toFloat();
    auto meterHeight = 16.0f;
    auto scopeBox = scope.withTrimmedBottom(meterHeight + 4.0f);
    auto meterBox = juce::Rectangle<float>(scope.getX(), scopeBox.getBottom() + 4,
                                           scope.getWidth(), meterHeight);

    auto size = juce::jmin(scopeBox.getWidth(), scopeBox.getHeight()) * 0.45f;
    auto cx = scopeBox.getCentreX();
    auto cy = scopeBox.getCentreY();

    g.setColour(juce::Colour(0xff14181d));
    g.fillRoundedRectangle(scopeBox, 6.0f);

    g.saveState();
    g.reduceClipRegion(scopeBox.toNearestInt());

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
    auto refLen = size * 0.707f;
    g.drawLine(cx, cy - refLen, cx, cy + refLen, 0.5f);

    // Scope dots colored by band
    if (anyScopeCount > 0)
    {
        auto dotSize = 1.8f;

        for (int b = 0; b < numBands; ++b)
        {
            auto& bs = bandScopes[static_cast<size_t>(b)];
            if (bs.count == 0)
                continue;

            auto colour = multiColor ? BandColours::getBandColour(b)
                                     : juce::Colour(0xff58c7d9);

            for (int i = 0; i < bs.count; ++i)
            {
                auto x = cx + bs.side[i] * size * zoomFactor;
                auto y = cy - bs.mid[i] * size * zoomFactor;

                if (x >= scopeBox.getX() && x <= scopeBox.getRight() &&
                    y >= scopeBox.getY() && y <= scopeBox.getBottom())
                {
                    auto alpha = juce::jmap<float>(i, 0, bs.count - 1, 0.08f, 0.85f);
                    g.setColour(colour.withAlpha(alpha));
                    g.fillEllipse(x - dotSize, y - dotSize, dotSize * 2.0f, dotSize * 2.0f);
                }
            }
        }
    }

    g.restoreState();

    // Labels
    g.setColour(juce::Colour(0xff6f7b87));
    g.setFont(juce::FontOptions(10.0f));
    g.drawFittedText("G", cx + 4, scopeBox.getY() + 4, 16, 14, juce::Justification::centredLeft, 1);
    g.drawFittedText("W", scopeBox.getX() + 4, cy + 4, 16, 14, juce::Justification::centredLeft, 1);

    // Phase correlation meter
    g.setColour(juce::Colour(0xff14181d));
    g.fillRoundedRectangle(meterBox, 4.0f);

    auto meterCenter = meterBox.getCentreX();
    auto meterHalf = meterBox.getWidth() * 0.5f;
    auto meterY = meterBox.getY();
    auto meterH = meterBox.getHeight();

    // Background bar
    g.setColour(juce::Colour(0xff252a31));
    g.fillRoundedRectangle(meterBox.reduced(0, 2), 2.0f);

    // Correlation fill
    auto fillW = (correlation + 1.0f) * 0.5f * meterBox.getWidth();
    auto fillRect = juce::Rectangle<float>(meterBox.getX(), meterY + 2,
                                           fillW, meterH - 4);
    auto corrColour = (correlation > 0.7f) ? juce::Colour(0xff4cd964) :
                      (correlation > 0.3f) ? juce::Colour(0xffffd60a) :
                      (correlation > -0.3f) ? juce::Colour(0xffff9500) :
                      juce::Colour(0xffff3b30);
    g.setColour(corrColour);
    g.fillRoundedRectangle(fillRect, 2.0f);

    // Center marker
    g.setColour(juce::Colour(0xff6f7b87));
    g.drawVerticalLine(static_cast<int>(meterCenter), meterY + 2, meterY + meterH - 2);

    // Correlation text
    g.setFont(juce::FontOptions(10.0f));
    g.drawFittedText(juce::String(correlation, 2),
                     static_cast<int>(meterBox.getX()),
                     static_cast<int>(meterY),
                     static_cast<int>(meterBox.getWidth()),
                     static_cast<int>(meterH),
                     juce::Justification::centred, 1);

    // Phase label
    g.setFont(juce::FontOptions(9.0f));
    g.drawFittedText("Phase", static_cast<int>(scopeBox.getX()),
                     static_cast<int>(meterBox.getBottom() + 2),
                     40, 12, juce::Justification::centredLeft, 1);
}

void Vectorscope::resized()
{
    auto area = getLocalBounds().reduced(12);
    auto titleBar = area.removeFromTop(22);
    auto toolbar = area.removeFromTop(20);

    auto btnW = 22;
    auto btnH = 16;
    auto btnRow = toolbar.withSizeKeepingCentre(toolbar.getWidth(), btnH);
    zoomOutBtn.setBounds(btnRow.removeFromLeft(btnW).reduced(1, 0));
    zoomInBtn.setBounds(btnRow.removeFromLeft(btnW).reduced(1, 0));
    colorBtn.setBounds(btnRow.removeFromRight(btnW).reduced(1, 0));
}

void Vectorscope::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    juce::ignoreUnused(event);
    auto delta = wheel.deltaY * 0.5f;
    zoomFactor = juce::jlimit(0.5f, 8.0f, zoomFactor + delta);
    repaint();
}
