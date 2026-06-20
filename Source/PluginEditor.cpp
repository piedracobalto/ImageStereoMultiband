/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ImageStereoMultibandAudioProcessorEditor::ImageStereoMultibandAudioProcessorEditor (ImageStereoMultibandAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      headerBar(audioProcessor.getAPVTS()),
      crossoverControls(audioProcessor.getAPVTS()),
      stereoFieldMeter(audioProcessor.getAPVTS())
{
    setLookAndFeel(&lookAndFeel);

    addAndMakeVisible(headerBar);
    addAndMakeVisible(crossoverControls);
    addAndMakeVisible(stereoFieldMeter);

    for (int i = 0; i < static_cast<int>(bandStrips.size()); ++i)
    {
        bandStrips[i] = std::make_unique<BandStrip>(audioProcessor.getAPVTS(), i);
        addAndMakeVisible(*bandStrips[i]);
    }

    setSize (980, 560);
}

ImageStereoMultibandAudioProcessorEditor::~ImageStereoMultibandAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void ImageStereoMultibandAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void ImageStereoMultibandAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    headerBar.setBounds(area.removeFromTop(56));

    auto content = area.reduced(16);

    auto topArea = content.removeFromTop(160);
    stereoFieldMeter.setBounds(topArea.removeFromRight(250).reduced(4, 0));
    topArea.removeFromRight(8);
    crossoverControls.setBounds(topArea);

    content.removeFromTop(12);

    const auto stripWidth = content.getWidth() / static_cast<int>(bandStrips.size());

    for (auto& bandStrip : bandStrips)
        bandStrip->setBounds(content.removeFromLeft(stripWidth).reduced(4, 0));
}
