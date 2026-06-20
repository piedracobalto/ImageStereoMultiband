/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GUI/Components/BandStrip.h"
#include "GUI/Components/CrossoverControls.h"
#include "GUI/Components/HeaderBar.h"
#include "GUI/Components/StereoFieldMeter.h"
#include "GUI/LookAndFeel/PluginLookAndFeel.h"

//==============================================================================
/**
*/
class ImageStereoMultibandAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ImageStereoMultibandAudioProcessorEditor (ImageStereoMultibandAudioProcessor&);
    ~ImageStereoMultibandAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ImageStereoMultibandAudioProcessor& audioProcessor;

    PluginLookAndFeel lookAndFeel;
    HeaderBar headerBar;
    CrossoverControls crossoverControls;
    StereoFieldMeter stereoFieldMeter;
    std::array<std::unique_ptr<BandStrip>, 5> bandStrips;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageStereoMultibandAudioProcessorEditor)
};
