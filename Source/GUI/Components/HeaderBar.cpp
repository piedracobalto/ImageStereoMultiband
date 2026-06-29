#include "HeaderBar.h"

HeaderBar::HeaderBar()
{
    titleLabel.setText("Image Stereo Multiband", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);
}

void HeaderBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xff15171a));
    g.fillRect(bounds);

    g.setColour(juce::Colour(0xff343941));
    g.drawLine(bounds.getX(), bounds.getBottom() - 1.0f, bounds.getRight(), bounds.getBottom() - 1.0f);
}

void HeaderBar::resized()
{
    auto area = getLocalBounds().reduced(16, 8);
    titleLabel.setBounds(area);
}
