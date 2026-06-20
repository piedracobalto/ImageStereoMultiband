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
      spectrumCrossoverControls(audioProcessor.getAPVTS())
{
    setLookAndFeel(&lookAndFeel);

    addAndMakeVisible(headerBar);
    addAndMakeVisible(spectrumCrossoverControls);
    addAndMakeVisible(vectorscope);

    for (int i = 0; i < static_cast<int>(bandStrips.size()); ++i)
    {
        bandStrips[i] = std::make_unique<BandStrip>(audioProcessor.getAPVTS(), i);
        addAndMakeVisible(*bandStrips[i]);
    }

    setSize (980, 720);
    startTimerHz(30);
}

ImageStereoMultibandAudioProcessorEditor::~ImageStereoMultibandAudioProcessorEditor()
{
    stopTimer();
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

    headerBar.setBounds(area.removeFromTop(50));

    auto content = area.reduced(12, 8);

    // Top: fused Spectrum + Crossover controls
    spectrumCrossoverControls.setBounds(content.removeFromTop(200).reduced(2, 2));

    content.removeFromTop(6);

    // Bottom row: band strips (left) + Vectorscope square (right)
    auto bottomRow = content;

    const auto vecSize = juce::jmin(bottomRow.getHeight() - 4, bottomRow.getWidth() / 3 - 4);
    auto vecArea = bottomRow.removeFromRight(vecSize).reduced(2, 2);
    vectorscope.setBounds(vecArea);

    const auto stripWidth = bottomRow.getWidth() / static_cast<int>(bandStrips.size());

    for (auto& bandStrip : bandStrips)
        bandStrip->setBounds(bottomRow.removeFromLeft(stripWidth).reduced(3, 0));
}

void ImageStereoMultibandAudioProcessorEditor::timerCallback()
{
    AudioAnalyzer::Snapshot snap;
    if (audioProcessor.getAnalyzer().consumeSnapshot(snap))
    {
        vectorscope.pushScopeData(snap.scopeLeft, snap.scopeRight, snap.scopeCount);
        vectorscope.repaint();

        auto maxFreq = static_cast<float>(audioProcessor.getCurrentSampleRate()) * 0.5f;
        spectrumCrossoverControls.pushSpectrum(snap.spectrum, AudioAnalyzer::numBins, maxFreq);

        bool muted[5], soloed[5];
        for (int i = 0; i < 5; ++i)
        {
            muted[i] = audioProcessor.isBandMuted(i);
            soloed[i] = audioProcessor.isBandSoloed(i);
        }
        spectrumCrossoverControls.setBandStates(muted, soloed, 5);

        spectrumCrossoverControls.repaint();
    }
}
