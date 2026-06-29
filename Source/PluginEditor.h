/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GUI/Components/BandStrip.h"
#include "GUI/Components/HeaderBar.h"
#include "GUI/Components/Vectorscope.h"
#include "GUI/Components/SpectrumCrossoverControls.h"
#include "GUI/LookAndFeel/PluginLookAndFeel.h"

//==============================================================================
class ImageStereoMultibandAudioProcessorEditor : public juce::AudioProcessorEditor,
    private juce::Timer
{
public:
    ImageStereoMultibandAudioProcessorEditor(ImageStereoMultibandAudioProcessor&);
    ~ImageStereoMultibandAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateBandVisibility();

    ImageStereoMultibandAudioProcessor& audioProcessor;

    PluginLookAndFeel lookAndFeel;
    HeaderBar headerBar;
    Vectorscope vectorscope;
    SpectrumCrossoverControls spectrumCrossoverControls;
    std::array<std::unique_ptr<BandStrip>, 5> bandStrips;

    juce::TextButton addBandBtn{ "+" };
    juce::TextButton removeBandBtn{ "-" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageStereoMultibandAudioProcessorEditor)
};