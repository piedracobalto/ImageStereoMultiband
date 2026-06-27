#include "PluginLookAndFeel.h"

PluginLookAndFeel::PluginLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff15171a));
    setColour(juce::Slider::thumbColourId, juce::Colour(0xffffb84d));
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff58c7d9));
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff343941));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff252a31));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff58c7d9));
    setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    setColour(juce::TextButton::textColourOnId, juce::Colours::black);
}

void PluginLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                         int x,
                                         int y,
                                         int width,
                                         int height,
                                         float sliderPos,
                                         float rotaryStartAngle,
                                         float rotaryEndAngle,
                                         juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x),
                                              static_cast<float>(y),
                                              static_cast<float>(width),
                                              static_cast<float>(height)).reduced(7.0f);

    const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto lineWidth = juce::jmax(2.0f, radius * 0.12f);
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centre.x,
                                centre.y,
                                radius,
                                radius,
                                0.0f,
                                rotaryStartAngle,
                                rotaryEndAngle,
                                true);

    g.setColour(slider.findColour(juce::Slider::rotarySliderOutlineColourId));
    g.strokePath(backgroundArc, juce::PathStrokeType(lineWidth, juce::PathStrokeType::curved));

    juce::Path valueArc;
    valueArc.addCentredArc(centre.x,
                           centre.y,
                           radius,
                           radius,
                           0.0f,
                           rotaryStartAngle,
                           angle,
                           true);

    g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
    g.strokePath(valueArc, juce::PathStrokeType(lineWidth, juce::PathStrokeType::curved));

    g.setColour(slider.findColour(juce::Slider::thumbColourId));
    g.fillEllipse(centre.x + std::cos(angle - juce::MathConstants<float>::halfPi) * radius - 4.0f,
                  centre.y + std::sin(angle - juce::MathConstants<float>::halfPi) * radius - 4.0f,
                  8.0f,
                  8.0f);
}

void PluginLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                         int x, int y, int width, int height,
                                         float sliderPos,
                                         float minSliderPos,
                                         float maxSliderPos,
                                         juce::Slider::SliderStyle style,
                                         juce::Slider& slider)
{
    if (slider.getProperties().contains("widthSlider"))
    {
        auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                             static_cast<float>(width), static_cast<float>(height))
                          .reduced(2.0f, static_cast<float>(height) * 0.3f);

        // Background
        g.setColour(juce::Colour(0xff343941));
        g.fillRoundedRectangle(bounds, bounds.getHeight() * 0.5f);

        // Map slider value 0-100 → fill 0-1 (1 = full center→sides)
        const float norm = juce::jlimit(0.0f, 1.0f, static_cast<float>(slider.getValue()) / 100.0f);
        const float cx = bounds.getCentreX();
        const float halfW = bounds.getWidth() * 0.5f * norm;

        if (norm > 0.01f)
        {
            auto colour = slider.findColour(juce::Slider::thumbColourId);
            auto fillRect = juce::Rectangle<float>(cx - halfW, bounds.getY(),
                                                   halfW * 2.0f, bounds.getHeight());
            g.setColour(colour);
            g.fillRoundedRectangle(fillRect, fillRect.getHeight() * 0.5f);
        }

        // Center line
        g.setColour(juce::Colour(0xff6f7b87));
        g.drawVerticalLine(static_cast<int>(cx),
                           static_cast<int>(bounds.getY()),
                           static_cast<int>(bounds.getBottom()));
        return;
    }

    LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
}
