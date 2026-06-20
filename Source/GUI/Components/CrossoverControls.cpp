#include "CrossoverControls.h"

CrossoverControls::CrossoverControls(juce::AudioProcessorValueTreeState& apvts)
    : valueTreeState(apvts),
      parameterIDs { "crossover1", "crossover2", "crossover3", "crossover4" }
{
    titleLabel.setText("Crossovers", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    const std::array<float, 4> defaultFrequencies { 120.0f, 500.0f, 2000.0f, 8000.0f };

    for (int i = 0; i < static_cast<int>(parameterIDs.size()); ++i)
    {
        frequencies[static_cast<size_t>(i)].store(defaultFrequencies[static_cast<size_t>(i)]);
        parameters[i] = valueTreeState.getParameter(parameterIDs[i]);
        valueTreeState.addParameterListener(parameterIDs[i], this);

        if (auto* value = valueTreeState.getRawParameterValue(parameterIDs[i]))
            frequencies[i].store(value->load());
    }
}

CrossoverControls::~CrossoverControls()
{
    for (const auto& parameterID : parameterIDs)
        valueTreeState.removeParameterListener(parameterID, this);
}

void CrossoverControls::paint(juce::Graphics& g)
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

    std::array<float, 6> bandEdges
    {
        graph.getX(),
        valueToX(frequencies[0].load()),
        valueToX(frequencies[1].load()),
        valueToX(frequencies[2].load()),
        valueToX(frequencies[3].load()),
        graph.getRight()
    };

    for (int i = 0; i < 5; ++i)
    {
        auto bandArea = juce::Rectangle<float>(bandEdges[i],
                                               graph.getY(),
                                               bandEdges[i + 1] - bandEdges[i],
                                               graph.getHeight());
        g.setColour(BandColours::getBandFillColour(i));
        g.fillRect(bandArea);
    }

    juce::Path response;
    response.startNewSubPath(graph.getX(), graph.getCentreY() - 6.0f);

    for (int i = 1; i <= 80; ++i)
    {
        const auto proportion = static_cast<float>(i) / 80.0f;
        const auto x = graph.getX() + graph.getWidth() * proportion;
        const auto ripple = std::sin(proportion * 34.0f) * 5.0f
                          + std::sin(proportion * 89.0f) * 2.0f;
        const auto tilt = proportion * 18.0f;
        response.lineTo(x, graph.getCentreY() - 10.0f + ripple + tilt);
    }

    g.setColour(juce::Colour(0xff65727f));
    g.strokePath(response, juce::PathStrokeType(1.4f));

    std::array<float, 6> markerFrequencies { 60.0f, 100.0f, 300.0f, 1000.0f, 3000.0f, 10000.0f };
    std::array<const char*, 6> markerLabels { "60", "100", "300", "1k", "3k", "10k" };

    for (int i = 0; i < static_cast<int>(markerFrequencies.size()); ++i)
    {
        const auto x = valueToX(markerFrequencies[i]);
        g.setColour(juce::Colour(0xff303944));
        g.drawVerticalLine(static_cast<int>(std::round(x)), graph.getY(), graph.getBottom());

        g.setColour(juce::Colour(0xff6f7b87));
        g.setFont(juce::FontOptions(11.0f));
        g.drawFittedText(markerLabels[i],
                         static_cast<int>(x - 22.0f),
                         graphBounds.getBottom() - 20,
                         44,
                         16,
                         juce::Justification::centred,
                         1);
    }

    for (int i = 0; i < static_cast<int>(frequencies.size()); ++i)
    {
        const auto x = valueToX(frequencies[i].load());
        const auto isActive = i == activeHandle;

        g.setColour(isActive ? juce::Colour(0xffffffff) : juce::Colour(0xffbac3cc));
        g.drawLine(x, graph.getY() - 6.0f, x, graph.getBottom() + 6.0f, isActive ? 2.0f : 1.2f);

        g.setColour(juce::Colour(0xff15171a));
        g.fillRoundedRectangle(x - 5.0f, graph.getY() - 10.0f, 10.0f, 22.0f, 4.0f);

        g.setColour(isActive ? juce::Colour(0xff58c7d9) : juce::Colour(0xffbac3cc));
        g.drawRoundedRectangle(x - 5.0f, graph.getY() - 10.0f, 10.0f, 22.0f, 4.0f, 1.2f);

        g.setFont(juce::FontOptions(12.0f));
        g.drawFittedText(formatFrequency(frequencies[i].load()),
                         static_cast<int>(x - 34.0f),
                         graphBounds.getBottom() + 4,
                         68,
                         18,
                         juce::Justification::centred,
                         1);
    }

    g.setColour(juce::Colour(0xff6f7b87));
    g.setFont(juce::FontOptions(12.0f));
    g.drawFittedText("Hz",
                     graphBounds.getRight() - 28,
                     graphBounds.getBottom() - 20,
                     28,
                     16,
                     juce::Justification::centredRight,
                     1);
}

void CrossoverControls::resized()
{
    auto area = getLocalBounds().reduced(10);
    titleLabel.setBounds(area.removeFromTop(24));
}

void CrossoverControls::mouseDown(const juce::MouseEvent& event)
{
    activeHandle = findNearestHandle(event.getPosition());

    if (activeHandle >= 0 && parameters[activeHandle] != nullptr)
        parameters[activeHandle]->beginChangeGesture();

    mouseDrag(event);
}

void CrossoverControls::mouseDrag(const juce::MouseEvent& event)
{
    if (activeHandle < 0)
        return;

    setCrossoverFrequency(activeHandle, xToValue(static_cast<float>(event.x)));
}

void CrossoverControls::mouseUp(const juce::MouseEvent&)
{
    if (activeHandle >= 0 && parameters[activeHandle] != nullptr)
        parameters[activeHandle]->endChangeGesture();

    activeHandle = -1;
    repaint();
}

void CrossoverControls::parameterChanged(const juce::String& parameterID, float newValue)
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

void CrossoverControls::handleAsyncUpdate()
{
    repaint();
}

juce::Rectangle<int> CrossoverControls::getGraphBounds() const
{
    auto area = getLocalBounds().reduced(14);
    area.removeFromTop(34);
    area.removeFromBottom(24);
    return area;
}

float CrossoverControls::valueToX(float frequency) const
{
    const auto graph = getGraphBounds().toFloat();
    const auto clampedFrequency = juce::jlimit(minFrequency, maxFrequency, frequency);
    const auto minLog = std::log10(minFrequency);
    const auto maxLog = std::log10(maxFrequency);
    const auto proportion = (std::log10(clampedFrequency) - minLog) / (maxLog - minLog);

    return graph.getX() + graph.getWidth() * proportion;
}

float CrossoverControls::xToValue(float x) const
{
    const auto graph = getGraphBounds().toFloat();
    const auto proportion = juce::jlimit(0.0f, 1.0f, (x - graph.getX()) / graph.getWidth());
    const auto minLog = std::log10(minFrequency);
    const auto maxLog = std::log10(maxFrequency);

    return std::pow(10.0f, minLog + proportion * (maxLog - minLog));
}

float CrossoverControls::getConstrainedFrequency(int index, float frequency) const
{
    auto lowerLimit = minFrequency;
    auto upperLimit = maxFrequency;

    if (index > 0)
        lowerLimit = frequencies[static_cast<size_t>(index - 1)].load() + minGapHz;

    if (index < static_cast<int>(frequencies.size()) - 1)
        upperLimit = frequencies[static_cast<size_t>(index + 1)].load() - minGapHz;

    return juce::jlimit(lowerLimit, upperLimit, frequency);
}

void CrossoverControls::setCrossoverFrequency(int index, float frequency)
{
    if (index < 0 || index >= static_cast<int>(parameters.size()) || parameters[static_cast<size_t>(index)] == nullptr)
        return;

    const auto constrainedFrequency = getConstrainedFrequency(index, frequency);
    frequencies[static_cast<size_t>(index)].store(constrainedFrequency);

    auto* parameter = parameters[static_cast<size_t>(index)];
    parameter->setValueNotifyingHost(parameter->convertTo0to1(constrainedFrequency));
    repaint();
}

int CrossoverControls::findNearestHandle(juce::Point<int> position) const
{
    if (! getGraphBounds().expanded(12, 18).contains(position))
        return -1;

    auto nearestIndex = -1;
    auto nearestDistance = std::numeric_limits<float>::max();

    for (int i = 0; i < static_cast<int>(frequencies.size()); ++i)
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

juce::String CrossoverControls::formatFrequency(float frequency) const
{
    if (frequency >= 1000.0f)
        return juce::String(frequency / 1000.0f, frequency >= 10000.0f ? 1 : 2) + "k";

    return juce::String(static_cast<int>(std::round(frequency)));
}
