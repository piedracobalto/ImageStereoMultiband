#include "SpectrumCrossoverControls.h"

SpectrumCrossoverControls::SpectrumCrossoverControls(juce::AudioProcessorValueTreeState& apvts)
    : valueTreeState(apvts),
      parameterIDs{ "crossover1", "crossover2", "crossover3", "crossover4" }
{
    titleLabel.setText("Frequency", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    const std::array<float, 4> defaultFrequencies{ 120.0f, 500.0f, 2000.0f, 8000.0f };

    for (int i = 0; i < static_cast<int>(parameterIDs.size()); ++i)
    {
        frequencies[static_cast<size_t>(i)].store(defaultFrequencies[static_cast<size_t>(i)]);
        valueTreeState.addParameterListener(parameterIDs[i], this);

        if (auto* value = valueTreeState.getRawParameterValue(parameterIDs[i]))
            frequencies[i].store(value->load());
    }

    crossoverEditLabel.setJustificationType(juce::Justification::centred);
    crossoverEditLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    crossoverEditLabel.setColour(juce::Label::textColourId, juce::Colour(0xff58c7d9));
    crossoverEditLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff14181d));
    crossoverEditLabel.setColour(juce::Label::backgroundWhenEditingColourId, juce::Colour(0xff14181d));
    crossoverEditLabel.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    crossoverEditLabel.setEditable(true, true);
    crossoverEditLabel.setVisible(false);
    crossoverEditLabel.onEditorHide = [this] { finishEditingCrossover(); };
    addAndMakeVisible(crossoverEditLabel);
}

SpectrumCrossoverControls::~SpectrumCrossoverControls()
{
    for (const auto& parameterID : parameterIDs)
        valueTreeState.removeParameterListener(parameterID, this);
}

void SpectrumCrossoverControls::pushSpectrum(const float* data, int numBinsIn, float maxFrequencyHz)
{
    numBins = juce::jlimit(0, maxBins, numBinsIn);
    maxSpectrumFreq = maxFrequencyHz;
    for (int i = 0; i < numBins; ++i)
        spectrum[i] = data[i];
}

void SpectrumCrossoverControls::setBandStates(const bool* muted, const bool* soloed, int numBandsIn)
{
    numBands = juce::jlimit(0, 5, numBandsIn);
    for (int i = 0; i < numBands; ++i)
    {
        bandMuted[i] = muted[i];
        bandSoloed[i] = soloed[i];
    }
    repaint();
}

void SpectrumCrossoverControls::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(juce::Colour(0xff20242a));
    g.fillRoundedRectangle(bounds, 8.0f);

    g.setColour(juce::Colour(0xff343941));
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

    auto graphBounds = getGraphBounds();
    auto graph = graphBounds.toFloat();

    g.setColour(juce::Colour(0xff14181d));
    g.fillRoundedRectangle(graph, 6.0f);

    g.saveState();
    g.reduceClipRegion(graphBounds);

    // ---- Band fills between crossovers ----
    const int activeCrossovers = juce::jmax(0, numBands - 1);
    std::array<float, 6> bandEdges{};
    bandEdges[0] = graph.getX();
    for (int i = 0; i < activeCrossovers; ++i)
        bandEdges[i + 1] = valueToX(frequencies[i].load());
    bandEdges[numBands] = graph.getRight();

    bool hasSolo = false;
    for (int i = 0; i < numBands; ++i)
    {
        if (bandSoloed[i])
        {
            hasSolo = true;
            break;
        }
    }

    for (int i = 0; i < numBands; ++i)
    {
        auto left = bandEdges[i];
        auto right = bandEdges[i + 1];
        if (right <= left)
            continue;

        float alpha = 0.20f;
        if (hasSolo)
            alpha = bandSoloed[i] ? 0.40f : 0.05f;
        else if (bandMuted[i])
            alpha = 0.05f;

        g.setColour(BandColours::getBandFillColour(i).withAlpha(alpha));
        g.fillRect(left, graph.getY(), right - left, graph.getHeight());
    }

    // ---- Grid lines (horizontal dB) ----
    for (float db : { -80.0f, -60.0f, -40.0f, -20.0f })
    {
        auto x = graph.getX();
        auto y = graph.getY() + graph.getHeight() * (1.0f - (db - minDb) / (maxDb - minDb));
        g.setColour(juce::Colour(0xff252a31));
        g.drawLine(x, y, graph.getRight(), y, 0.5f);
    }

    // ---- Grid lines (vertical frequency markers) ----
    std::array<float, 7> markerFreqs = { 30.0f, 60.0f, 100.0f, 300.0f, 1000.0f, 5000.0f, 15000.0f };
    for (auto f : markerFreqs)
    {
        auto x = valueToX(f);
        g.setColour(juce::Colour(0xff252a31));
        g.drawLine(x, graph.getY(), x, graph.getBottom(), 0.5f);
    }

    // ---- Spectrum curve ----
    if (numBins > 0)
    {
        juce::Path spectrumPath;
        bool started = false;

        for (int i = 0; i < numBins; ++i)
        {
            const auto binFreq =
                static_cast<float>(i) * maxSpectrumFreq /
                static_cast<float>(numBins);

            if (binFreq < minFrequency)
                continue;

            if (binFreq > maxFrequency)
                break;

            const auto x = valueToX(binFreq);

            const auto db = juce::jlimit(
                minDb,
                maxDb,
                spectrum[i]);

            const auto y =
                graph.getY() +
                graph.getHeight() *
                (1.0f - (db - minDb) / (maxDb - minDb));

            if (!started)
            {
                spectrumPath.startNewSubPath(x, y);
                started = true;
            }
            else
            {
                spectrumPath.lineTo(x, y);
            }
        }

        g.setColour(
            juce::Colour(0xff58c7d9)
            .withAlpha(0.85f));

        g.strokePath(
            spectrumPath,
            juce::PathStrokeType(
                1.5f,
                juce::PathStrokeType::curved));
    }

    // ---- Crossover lines with handle labels ----
    for (int i = 0; i < activeCrossovers; ++i)
    {
        const auto x = valueToX(frequencies[i].load());
        const auto isActive = i == activeHandle;

        g.setColour(isActive ? juce::Colour(0xffffffff) : juce::Colour(0xffbac3cc));
        g.drawLine(x, graph.getY(), x, graph.getBottom(), isActive ? 2.0f : 1.2f);

        g.setColour(juce::Colour(0xff15171a));
        g.fillRoundedRectangle(x - 5.0f, graph.getY() - 10.0f, 10.0f, 22.0f, 4.0f);

        g.setColour(isActive ? juce::Colour(0xff58c7d9) : juce::Colour(0xffbac3cc));
        g.drawRoundedRectangle(x - 5.0f, graph.getY() - 10.0f, 10.0f, 22.0f, 4.0f, 1.2f);
    }

    g.restoreState();

    // ---- Labels below graph area ----
    g.setColour(juce::Colour(0xff6f7b87));
    g.setFont(juce::FontOptions(10.0f));

    for (auto f : markerFreqs)
    {
        auto x = valueToX(f);
        juce::String label;
        if (f >= 1000.0f)
            label = juce::String(f / 1000.0f, 1) + "k";
        else
            label = juce::String(static_cast<int>(f));

        auto labelW = 40.0f;
        g.drawFittedText(label,
                         static_cast<int>(x - labelW / 2.0f),
                         graphBounds.getBottom() + 2,
                         static_cast<int>(labelW),
                         14,
                         juce::Justification::centred,
                         1);
    }

    // Crossover frequency labels below markers - staggered to prevent overlap
    for (int i = 0; i < activeCrossovers; ++i)
    {
        const auto x = valueToX(frequencies[i].load());

        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff58c7d9));
        g.drawFittedText(formatFrequency(frequencies[i].load()),
                         static_cast<int>(x - 34.0f),
                         graphBounds.getBottom() + 12 + i * 8,
                         60,
                         12,
                         juce::Justification::centred,
                         1);
    }

    g.setFont(juce::FontOptions(10.0f));
    g.drawFittedText("Hz",
                     graphBounds.getRight() - 22,
                     graphBounds.getBottom() + 2,
                     22,
                     14,
                     juce::Justification::centredRight,
                     1);

    // dB labels
    g.setFont(juce::FontOptions(10.0f));
    for (float db : { -60.0f, -40.0f, -20.0f })
    {
        auto y = graph.getY() + graph.getHeight() * (1.0f - (db - minDb) / (maxDb - minDb));
        g.drawFittedText(juce::String(static_cast<int>(db)),
                         graphBounds.getX() - 26,
                         static_cast<int>(y - 8.0f),
                         22,
                         16,
                         juce::Justification::centredRight,
                         1);
    }
}

void SpectrumCrossoverControls::resized()
{
    auto area = getLocalBounds().reduced(10);
    titleLabel.setBounds(area.removeFromTop(24));
}

void SpectrumCrossoverControls::mouseDown(const juce::MouseEvent& event)
{
    const int activeCrossovers = juce::jmax(0, numBands - 1);
    activeHandle = findNearestHandle(event.getPosition());

    mouseDrag(event);
}

void SpectrumCrossoverControls::mouseDrag(const juce::MouseEvent& event)
{
    if (activeHandle < 0)
        return;

    setCrossoverFrequency(activeHandle, xToValue(static_cast<float>(event.x)));
}

void SpectrumCrossoverControls::mouseUp(const juce::MouseEvent&)
{
    activeHandle = -1;
    repaint();
}

void SpectrumCrossoverControls::mouseDoubleClick(const juce::MouseEvent& event)
{
    const int activeCrossovers = juce::jmax(0, numBands - 1);
    auto graphBounds = getGraphBounds();

    for (int i = 0; i < activeCrossovers; ++i)
    {
        const auto x = static_cast<int>(valueToX(frequencies[i].load()));
        auto labelBounds = juce::Rectangle<int>(x - 34,
                                                graphBounds.getBottom() + 12 + i * 8,
                                                60, 12);
        if (labelBounds.contains(event.getPosition()))
        {
            startEditingCrossover(i);
            return;
        }
    }
}

void SpectrumCrossoverControls::startEditingCrossover(int index)
{
    editingCrossover = index;
    const auto x = static_cast<int>(valueToX(frequencies[static_cast<size_t>(index)].load()));
    auto graphBounds = getGraphBounds();
    auto labelBounds = juce::Rectangle<int>(x - 26,
                                            graphBounds.getBottom() + 12 + index * 8,
                                            48, 12);
    crossoverEditLabel.setBounds(labelBounds);
    crossoverEditLabel.setText(formatFrequency(frequencies[static_cast<size_t>(index)].load()),
                               juce::dontSendNotification);
    crossoverEditLabel.setVisible(true);
    crossoverEditLabel.showEditor();
}

void SpectrumCrossoverControls::finishEditingCrossover()
{
    if (editingCrossover < 0)
        return;

    auto text = crossoverEditLabel.getText();
    float newFreq = text.getFloatValue();
    if (newFreq >= minFrequency && newFreq <= maxFrequency)
        setCrossoverFrequency(editingCrossover, newFreq);

    crossoverEditLabel.setVisible(false);
    editingCrossover = -1;
}

void SpectrumCrossoverControls::parameterChanged(const juce::String& parameterID, float newValue)
{
    for (int i = 0; i < static_cast<int>(parameterIDs.size()); ++i)
    {
        if (parameterIDs[i] == parameterID)
        {
            frequencies[i].store(newValue);
            triggerAsyncUpdate();
            return;
        }
    }
}

void SpectrumCrossoverControls::handleAsyncUpdate()
{
    repaint();
}

juce::Rectangle<int> SpectrumCrossoverControls::getGraphBounds() const
{
    auto area = getLocalBounds().reduced(14);
    area.removeFromTop(34);
    area.removeFromBottom(48);
    area.removeFromLeft(8);
    return area.reduced(4, 6);
}

float SpectrumCrossoverControls::valueToX(float frequency) const
{
    const auto graph = getGraphBounds().toFloat();
    const auto clampedFrequency = juce::jlimit(minFrequency, maxFrequency, frequency);
    const auto minLog = std::log10(minFrequency);
    const auto maxLog = std::log10(maxFrequency);
    const auto proportion = (std::log10(clampedFrequency) - minLog) / (maxLog - minLog);

    return graph.getX() + graph.getWidth() * proportion;
}

float SpectrumCrossoverControls::xToValue(float x) const
{
    const auto graph = getGraphBounds().toFloat();
    const auto proportion = juce::jlimit(0.0f, 1.0f, (x - graph.getX()) / graph.getWidth());
    const auto minLog = std::log10(minFrequency);
    const auto maxLog = std::log10(maxFrequency);

    return std::pow(10.0f, minLog + proportion * (maxLog - minLog));
}

float SpectrumCrossoverControls::getConstrainedFrequency(int index, float frequency) const
{
    const int activeCrossovers = juce::jmax(0, numBands - 1);
    auto lowerLimit = minFrequency;
    auto upperLimit = maxFrequency;

    if (index > 0)
        lowerLimit = frequencies[static_cast<size_t>(index - 1)].load() + minGapHz;

    if (index < activeCrossovers - 1)
        upperLimit = frequencies[static_cast<size_t>(index + 1)].load() - minGapHz;

    return juce::jlimit(lowerLimit, upperLimit, frequency);
}

void SpectrumCrossoverControls::setCrossoverFrequency(int index, float frequency)
{
    const int activeCrossovers = juce::jmax(0, numBands - 1);
    if (index < 0 || index >= activeCrossovers)
        return;

    const auto constrainedFrequency = getConstrainedFrequency(index, frequency);
    frequencies[static_cast<size_t>(index)].store(constrainedFrequency);

    // Update via RawParameterValue (thread-safe, no processor pointer needed)
    if (auto* value = valueTreeState.getRawParameterValue(parameterIDs[static_cast<size_t>(index)]))
    {
        auto* param = valueTreeState.getParameter(parameterIDs[static_cast<size_t>(index)]);
        if (param != nullptr)
            *value = param->convertTo0to1(constrainedFrequency);
    }
    repaint();
}

int SpectrumCrossoverControls::findNearestHandle(juce::Point<int> position) const
{
    if (! getGraphBounds().expanded(12, 18).contains(position))
        return -1;

    const int activeCrossovers = juce::jmax(0, numBands - 1);
    auto nearestIndex = -1;
    auto nearestDistance = std::numeric_limits<float>::max();

    for (int i = 0; i < activeCrossovers; ++i)
    {
        const auto distance = std::abs(static_cast<float>(position.x) - valueToX(frequencies[static_cast<size_t>(i)].load()));

        if (distance < nearestDistance)
        {
            nearestDistance = distance;
            nearestIndex = i;
        }
    }

    return nearestDistance <= 28.0f ? nearestIndex : -1;
}

juce::String SpectrumCrossoverControls::formatFrequency(float frequency) const
{
    if (frequency >= 1000.0f)
        return juce::String(frequency / 1000.0f, frequency >= 10000.0f ? 1 : 2) + "k";

    return juce::String(static_cast<int>(std::round(frequency)));
}
