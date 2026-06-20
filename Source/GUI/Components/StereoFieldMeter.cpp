#include "StereoFieldMeter.h"

StereoFieldMeter::StereoFieldMeter(juce::AudioProcessorValueTreeState& apvts)
    : valueTreeState(apvts),
      widthParameterIDs { "band1Width", "band2Width", "band3Width", "band4Width", "band5Width" }
{
    for (int i = 0; i < BandColours::numBands; ++i)
    {
        widths[static_cast<size_t>(i)].store(1.0f);
        valueTreeState.addParameterListener(widthParameterIDs[static_cast<size_t>(i)], this);

        if (auto* value = valueTreeState.getRawParameterValue(widthParameterIDs[static_cast<size_t>(i)]))
            widths[static_cast<size_t>(i)].store(value->load());
    }
}

StereoFieldMeter::~StereoFieldMeter()
{
    for (const auto& parameterID : widthParameterIDs)
        valueTreeState.removeParameterListener(parameterID, this);
}

void StereoFieldMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(juce::Colour(0xff20242a));
    g.fillRoundedRectangle(bounds, 8.0f);

    g.setColour(juce::Colour(0xff343941));
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

    auto area = getLocalBounds().reduced(12);

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(14.0f));
    g.drawFittedText("Stereo Field", area.removeFromTop(22), juce::Justification::centredLeft, 1);

    auto scope = area.reduced(2, 6).toFloat();
    const auto size = juce::jmin(scope.getWidth(), scope.getHeight() * 1.75f);
    const auto centre = juce::Point<float>(scope.getCentreX(), scope.getBottom() - 8.0f);
    const auto radius = size * 0.43f;

    g.setColour(juce::Colour(0xff15191f));
    g.fillRoundedRectangle(scope, 6.0f);

    juce::Path outerArc;
    outerArc.addCentredArc(centre.x,
                           centre.y,
                           radius,
                           radius,
                           0.0f,
                           -juce::MathConstants<float>::pi,
                           0.0f,
                           true);

    g.setColour(juce::Colour(0xff46515e));
    g.strokePath(outerArc, juce::PathStrokeType(1.0f));
    g.drawLine(centre.x - radius, centre.y, centre.x + radius, centre.y, 1.0f);

    for (float angle : { -0.75f, -0.5f, -0.25f, 0.0f, 0.25f, 0.5f, 0.75f })
    {
        const auto radians = -juce::MathConstants<float>::halfPi + angle * juce::MathConstants<float>::halfPi;
        const auto end = centre + juce::Point<float>(std::cos(radians) * radius,
                                                    std::sin(radians) * radius);
        g.setColour(angle == 0.0f ? juce::Colour(0xff586575) : juce::Colour(0xff303944));
        g.drawLine(centre.x, centre.y, end.x, end.y, angle == 0.0f ? 1.2f : 0.8f);
    }

    for (int i = 0; i < BandColours::numBands; ++i)
    {
        const auto width = juce::jlimit(0.0f, 2.0f, widths[static_cast<size_t>(i)].load());
        const auto normalisedWidth = width / 2.0f;
        const auto laneRadius = radius * (0.28f + 0.13f * static_cast<float>(i));
        const auto spread = juce::jmap(normalisedWidth, 0.08f, 0.95f);
        const auto leftAngle = -juce::MathConstants<float>::halfPi - spread * juce::MathConstants<float>::halfPi * 0.78f;
        const auto rightAngle = -juce::MathConstants<float>::halfPi + spread * juce::MathConstants<float>::halfPi * 0.78f;

        juce::Path bandArc;
        bandArc.addCentredArc(centre.x,
                              centre.y,
                              laneRadius,
                              laneRadius,
                              0.0f,
                              leftAngle,
                              rightAngle,
                              true);

        g.setColour(BandColours::getBandColour(i).withAlpha(0.82f));
        g.strokePath(bandArc, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));

        const auto dot = centre + juce::Point<float>(std::cos(rightAngle) * laneRadius,
                                                    std::sin(rightAngle) * laneRadius);
        g.fillEllipse(dot.x - 2.5f, dot.y - 2.5f, 5.0f, 5.0f);
    }

    g.setColour(juce::Colour(0xff6f7b87));
    g.setFont(juce::FontOptions(11.0f));
    g.drawFittedText("Mono", static_cast<int>(centre.x - 24.0f), static_cast<int>(centre.y - radius - 4.0f), 48, 16, juce::Justification::centred, 1);
    g.drawFittedText("Wide", static_cast<int>(centre.x + radius - 44.0f), static_cast<int>(centre.y - 18.0f), 42, 16, juce::Justification::centredRight, 1);
}

void StereoFieldMeter::parameterChanged(const juce::String& parameterID, float newValue)
{
    for (int i = 0; i < BandColours::numBands; ++i)
    {
        if (widthParameterIDs[static_cast<size_t>(i)] == parameterID)
        {
            widths[static_cast<size_t>(i)].store(newValue);
            triggerAsyncUpdate();
            return;
        }
    }
}

void StereoFieldMeter::handleAsyncUpdate()
{
    repaint();
}
