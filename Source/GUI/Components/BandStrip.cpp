#include "BandStrip.h"

BandStrip::BandStrip(juce::AudioProcessorValueTreeState& apvts, int bandIndex)
    : bandNumber(bandIndex + 1),
      accentColour(BandColours::getBandColour(bandIndex))
{
    titleLabel.setText("Band " + juce::String(bandNumber), juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    widthLabel.setText("Width", juce::dontSendNotification);
    widthLabel.setJustificationType(juce::Justification::centred);
    widthLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaeb6c2));
    addAndMakeVisible(widthLabel);

    gainLabel.setText("Gain", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centred);
    gainLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaeb6c2));
    addAndMakeVisible(gainLabel);

    configureSlider(widthSlider);
    configureSlider(gainSlider);
    widthSlider.setSliderStyle(juce::Slider::LinearHorizontal);

    for (auto* slider : { &widthSlider, &gainSlider })
    {
        slider->setColour(juce::Slider::thumbColourId, accentColour);
        slider->setColour(juce::Slider::trackColourId, accentColour);
        slider->setColour(juce::Slider::backgroundColourId, juce::Colour(0xff505a64));
    }

    addAndMakeVisible(widthSlider);
    addAndMakeVisible(gainSlider);

    muteButton.setClickingTogglesState(true);
    soloButton.setClickingTogglesState(true);
    muteButton.setColour(juce::TextButton::buttonOnColourId, accentColour.withAlpha(0.85f));
    soloButton.setColour(juce::TextButton::buttonOnColourId, accentColour.withAlpha(0.85f));
    addAndMakeVisible(muteButton);
    addAndMakeVisible(soloButton);

    const auto bandPrefix = "band" + juce::String(bandNumber);
    widthAttachment = std::make_unique<SliderAttachment>(apvts, bandPrefix + "Width", widthSlider);
    gainAttachment = std::make_unique<SliderAttachment>(apvts, bandPrefix + "Gain", gainSlider);
    muteAttachment = std::make_unique<ButtonAttachment>(apvts, bandPrefix + "Mute", muteButton);
    soloAttachment = std::make_unique<ButtonAttachment>(apvts, bandPrefix + "Solo", soloButton);
}

void BandStrip::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(juce::Colour(0xff20242a));
    g.fillRoundedRectangle(bounds, 8.0f);

    g.setColour(juce::Colour(0xff343941));
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

    auto accent = bounds.reduced(7.0f);
    accent.setHeight(3.0f);
    g.setColour(accentColour);
    g.fillRoundedRectangle(accent, 1.5f);
}

void BandStrip::resized()
{
    auto area = getLocalBounds().reduced(8);

    titleLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(4);

    auto buttonArea = area.removeFromBottom(28);
    muteButton.setBounds(buttonArea.removeFromLeft(buttonArea.getWidth() / 2).reduced(2));
    soloButton.setBounds(buttonArea.reduced(2));

    auto widthArea = area.removeFromBottom(58);
    widthLabel.setBounds(widthArea.removeFromTop(18));
    widthSlider.setBounds(widthArea.reduced(2, 0));

    gainLabel.setBounds(area.removeFromBottom(18));
    gainSlider.setBounds(area.reduced(10, 2));
}

void BandStrip::configureSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::LinearVertical);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
}
