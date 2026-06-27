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

    setOpaque(true);

    addAndMakeVisible(zoomOutBtn);
    styleBtn(zoomOutBtn);
    zoomOutBtn.onClick = [this]
        {
            zoomFactor = juce::jmax(0.5f, zoomFactor - 0.5f);
            repaint();
        };

    addAndMakeVisible(zoomInBtn);
    styleBtn(zoomInBtn);
    zoomInBtn.onClick = [this]
        {
            zoomFactor = juce::jmin(8.0f, zoomFactor + 0.5f);
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
        bs.mid[i] = (left[i] + right[i]) * 0.5f;
        bs.side[i] = (left[i] - right[i]) * 0.5f;
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
        bs.mid[i] = (left[i] + right[i]) * 0.5f;
        bs.side[i] = (left[i] - right[i]) * 0.5f;
    }

    int maxCount = 0;
    for (int i = 0; i < numBands; ++i)
        if (bandScopes[i].count > maxCount)
            maxCount = bandScopes[i].count;
    anyScopeCount = maxCount;
    dirtyScope = true;
}

void Vectorscope::setHasSignal(bool s)
{
    if (s)
        signalHoldCounter = signalHoldFrames;
    else if (signalHoldCounter > 0)
        --signalHoldCounter;
    hasSignal = signalHoldCounter > 0;
}

void Vectorscope::clearScopes()
{
    anyScopeCount = 0;
    for (auto& bs : bandScopes) bs.count = 0;
    hasSignal = false;
    signalHoldCounter = 0;
    targetCorrelation = 0.0f;
    displayCorrelation = 0.0f;
    lastPaintedCorrelation = 0.0f;
    dirtyScope = false;
    repaint();
}

void Vectorscope::tickSmoothing()
{
    auto diff = targetCorrelation - displayCorrelation;
    displayCorrelation += diff * 0.12f;
    if (std::abs(diff) < 0.0001f)
        displayCorrelation = targetCorrelation;

    auto corrDiff = std::abs(displayCorrelation - lastPaintedCorrelation);
    if (dirtyScope || corrDiff > 0.02f)
    {
        lastPaintedCorrelation = displayCorrelation;
        dirtyScope = false;
        repaint();
    }
}

// ─── layout ───────────────────────────────────────────────────────────────────

struct VSLayout
{
    juce::Rectangle<int>   titleBar;
    juce::Rectangle<int>   toolbar;
    juce::Rectangle<float> scopeBox;
    juce::Rectangle<float> meterBox;
    juce::Rectangle<float> tickBox;
};

static VSLayout computeLayout(juce::Rectangle<int> total)
{
    constexpr int   pad = 14;   // increased outer margin
    constexpr int   titleH = 22;
    constexpr int   toolbarH = 26;
    constexpr float meterH = 16.0f;
    constexpr float tickH = 12.0f;
    constexpr float bottomGap = 8.0f;

    auto area = total.reduced(pad);

    VSLayout l;
    l.titleBar = area.removeFromTop(titleH);
    area.removeFromTop(4);
    l.toolbar = area.removeFromTop(toolbarH);
    area.removeFromTop(6);

    auto scope = area.toFloat();

    l.scopeBox = scope.withTrimmedBottom(meterH + tickH + bottomGap);
    l.meterBox = { scope.getX(),
                   l.scopeBox.getBottom() + 4,
                   scope.getWidth(), meterH };
    l.tickBox = { scope.getX(),
                   l.meterBox.getBottom() + 2,
                   scope.getWidth(), tickH };
    return l;
}

// ─── paint ────────────────────────────────────────────────────────────────────

void Vectorscope::paint(juce::Graphics& g)
{
    auto fullBounds = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xff181c21));
    g.fillRect(fullBounds);

    auto bounds = fullBounds.reduced(2.0f);
    g.setColour(juce::Colour(0xff20242a));
    g.fillRoundedRectangle(bounds, 8.0f);
    g.setColour(juce::Colour(0xff343941));
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

    auto lay = computeLayout(getLocalBounds());

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(14.0f));
    g.drawFittedText("Vectorscope", lay.titleBar, juce::Justification::centredLeft, 1);

    // Zoom label
    g.setFont(juce::FontOptions(13.0f));
    g.setColour(juce::Colour(0xff8a9ba8));
    g.drawFittedText("x" + juce::String(zoomFactor, 1),
        zoomInBtn.getRight() + 10, lay.toolbar.getY(),
        50, lay.toolbar.getHeight(),
        juce::Justification::centredLeft, 1);

    // ── Goniometer ────────────────────────────────────────────────────────────
    auto& sb = lay.scopeBox;
    auto  size = juce::jmin(sb.getWidth(), sb.getHeight()) * 0.42f;
    auto  cx = sb.getCentreX();
    auto  cy = sb.getCentreY();

    g.setColour(juce::Colour(0xff14181d));
    g.fillRoundedRectangle(sb, 6.0f);

    g.saveState();
    g.reduceClipRegion(sb.toNearestInt());

    // Crosshair
    g.setColour(juce::Colour(0xff252a31));
    g.drawLine(cx - size, cy, cx + size, cy, 0.8f);
    g.drawLine(cx, cy - size, cx, cy + size, 0.8f);

    // Reference circles
    for (float r : { 0.5f, 1.0f })
    {
        auto radius = r * size;
        g.setColour(juce::Colour(0xff303944));
        g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 0.8f);
    }

    // Mid reference line
    g.setColour(juce::Colour(0xff303944).withAlpha(0.5f));
    g.drawLine(cx, cy - size * 0.707f, cx, cy + size * 0.707f, 0.5f);

    // Scope dots
    if (anyScopeCount > 0 && hasSignal)
    {
        const float dotSize = 1.8f;
        for (int b = 0; b < numBands; ++b)
        {
            auto& bs = bandScopes[static_cast<size_t>(b)];
            if (bs.count == 0) continue;

            auto colour = multiColor ? BandColours::getBandColour(b)
                : juce::Colour(0xff58c7d9);

            for (int i = 0; i < bs.count; ++i)
            {
                auto x = cx + bs.side[i] * size * zoomFactor;
                auto y = cy - bs.mid[i] * size * zoomFactor;
                if (x >= sb.getX() && x <= sb.getRight() &&
                    y >= sb.getY() && y <= sb.getBottom())
                {
                    auto alpha = juce::jmap<float>(i, 0, bs.count - 1, 0.08f, 0.85f);
                    g.setColour(colour.withAlpha(alpha));
                    g.fillEllipse(x - dotSize, y - dotSize, dotSize * 2.0f, dotSize * 2.0f);
                }
            }
        }
    }

    // ── Labels inside scopeBox ─────────────────────────────────────────────
    //   G: top-centre, W: right-centre — both inset so they sit inside the box
    g.setColour(juce::Colour(0xff6f7b87));
    g.setFont(juce::FontOptions(10.0f));

    // G — near the top centre, inset by ~10px from top edge
    g.drawFittedText("G",
        static_cast<int>(cx) - 8,
        static_cast<int>(sb.getY()) + 8,
        16, 14,
        juce::Justification::centred, 1);

    // W — near the right centre, inset from right edge
    g.drawFittedText("W",
        static_cast<int>(sb.getRight()) - 16,
        static_cast<int>(cy) - 7,
        16, 14,
        juce::Justification::centredLeft, 1);

    g.restoreState();

    // ── Phase correlation meter ───────────────────────────────────────────────
    auto& mb = lay.meterBox;
    float meterLeft = mb.getX();
    auto  tickTop = mb.getY() + 2;
    auto  tickBot = mb.getBottom() - 2;

    g.setColour(juce::Colour(0xff14181d));
    g.fillRoundedRectangle(mb, 4.0f);
    g.setColour(juce::Colour(0xff252a31));
    g.fillRoundedRectangle(mb.reduced(0, 2), 2.0f);

    g.setColour(juce::Colour(0xff555555));
    for (float t = -1.0f; t <= 1.0f; t += 0.5f)
    {
        auto x = meterLeft + (t + 1.0f) * 0.5f * mb.getWidth();
        g.drawVerticalLine(static_cast<int>(x), tickTop, tickBot);
    }

    if (hasSignal)
    {
        auto fillW = (displayCorrelation + 1.0f) * 0.5f * mb.getWidth();
        auto fillRect = juce::Rectangle<float>(meterLeft, tickTop, fillW, tickBot - tickTop);
        auto c = juce::jlimit(-1.0f, 1.0f, displayCorrelation);

        static const juce::Colour stops[] = {
            juce::Colour(0xffcc0000), juce::Colour(0xffff3b30),
            juce::Colour(0xffff9500), juce::Colour(0xffffd60a),
            juce::Colour(0xff4cd964),
        };
        static const float thresh[] = { -1.0f, -0.7f, -0.3f, 0.3f, 0.7f, 1.0f };

        auto corrColour = stops[4];
        for (int i = 0; i < 4; ++i)
        {
            if (c >= thresh[i] && c < thresh[i + 1])
            {
                corrColour = stops[i].interpolatedWith(stops[i + 1],
                    (c - thresh[i]) / (thresh[i + 1] - thresh[i]));
                break;
            }
        }
        g.setColour(corrColour);
        g.fillRoundedRectangle(fillRect, 2.0f);
    }

    g.setFont(juce::FontOptions(10.0f));
    g.setColour(juce::Colour(0xff6f7b87));
    g.drawFittedText(hasSignal ? juce::String(displayCorrelation, 1) : juce::String("---"),
        static_cast<int>(mb.getX()), static_cast<int>(mb.getY()),
        static_cast<int>(mb.getWidth()), static_cast<int>(mb.getHeight()),
        juce::Justification::centred, 1);

    // Tick labels
    auto& tb = lay.tickBox;
    g.setFont(juce::FontOptions(8.0f));
    for (float t = -1.0f; t <= 1.0f; t += 0.5f)
    {
        auto x = meterLeft + (t + 1.0f) * 0.5f * mb.getWidth();
        g.drawFittedText(juce::String(t, 1),
            static_cast<int>(x) - 14, static_cast<int>(tb.getY()),
            28, static_cast<int>(tb.getHeight()),
            juce::Justification::centred, 1);
    }
}

// ─── resized ──────────────────────────────────────────────────────────────────

void Vectorscope::resized()
{
    auto lay = computeLayout(getLocalBounds());
    auto toolbar = lay.toolbar;

    constexpr int btnW = 28, btnH = 20, btnGap = 4;
    auto btnRow = toolbar.withSizeKeepingCentre(toolbar.getWidth(), btnH);

    zoomOutBtn.setBounds(btnRow.removeFromLeft(btnW));
    btnRow.removeFromLeft(btnGap);
    zoomInBtn.setBounds(btnRow.removeFromLeft(btnW));
    colorBtn.setBounds(btnRow.removeFromRight(btnW));
}

// ─── mouse wheel ──────────────────────────────────────────────────────────────

void Vectorscope::mouseWheelMove(const juce::MouseEvent& event,
    const juce::MouseWheelDetails& wheel)
{
    juce::ignoreUnused(event);
    zoomFactor = juce::jlimit(0.5f, 8.0f, zoomFactor + wheel.deltaY * 0.5f);
    repaint();
}