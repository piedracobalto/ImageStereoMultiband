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

    auto styleBtn = [](juce::TextButton& btn)
    {
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2f36));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8a9ba8));
    };
    styleBtn(addBandBtn);
    styleBtn(removeBandBtn);
    addBandBtn.onClick = [this]
    {
        if (audioProcessor.canAddBand())
        {
            audioProcessor.setNumBands(audioProcessor.getNumBands() + 1);
            updateBandVisibility();
            resized();
        }
    };
    removeBandBtn.onClick = [this]
    {
        if (audioProcessor.canRemoveBand())
        {
            audioProcessor.setNumBands(audioProcessor.getNumBands() - 1);
            updateBandVisibility();
            resized();
        }
    };
    addAndMakeVisible(addBandBtn);
    addAndMakeVisible(removeBandBtn);

    // Overlay de bypass al final para que quede encima de todo
    addAndMakeVisible(bypassOverlay);
    bypassOverlay.setVisible(false);

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

    // Band controls toolbar: + / - buttons centred
    auto toolbarArea = content.removeFromTop(28).reduced(2, 2);
    auto btnWidth = 50;
    auto totalBtnWidth = btnWidth * 2 + 6;
    auto btnArea = toolbarArea.withSizeKeepingCentre(totalBtnWidth, toolbarArea.getHeight());
    removeBandBtn.setBounds(btnArea.removeFromLeft(btnWidth).reduced(2));
    btnArea.removeFromLeft(6);
    addBandBtn.setBounds(btnArea.reduced(2));

    content.removeFromTop(4);

    // Bottom row: band strips (left) + Vectorscope square (right)
    auto bottomRow = content;

    const auto vecSize = juce::jmin(bottomRow.getHeight() - 4, bottomRow.getWidth() / 3 - 4);
    auto vecArea = bottomRow.removeFromRight(vecSize).reduced(2, 2);
    vectorscope.setBounds(vecArea);

    const auto numActive = audioProcessor.getNumBands();
    const auto stripWidth = bottomRow.getWidth() / numActive;

    for (int i = 0; i < numActive; ++i)
        bandStrips[i]->setBounds(bottomRow.removeFromLeft(stripWidth).reduced(3, 0));

    // Show/hide extra bands
    for (int i = numActive; i < 5; ++i)
        bandStrips[i]->setVisible(false);

    updateBandVisibility();

    auto overlayArea = getLocalBounds();
    overlayArea.removeFromTop(50); // Dejar el header bar clickeable
    bypassOverlay.setBounds(overlayArea);
}

void ImageStereoMultibandAudioProcessorEditor::timerCallback()
{
    bypassOverlay.setVisible(audioProcessor.isBypassed());

    if (audioProcessor.isBypassed())
    {
        vectorscope.setHasSignal(false);
        vectorscope.tickSmoothing();
        return;
    }

    AudioAnalyzer::Snapshot snap;
    if (audioProcessor.getAnalyzer().consumeSnapshot(snap))
    {
        const int numBands = audioProcessor.getNumBands();
        for (int i = 0; i < numBands; ++i)
        {
            auto& bs = audioProcessor.getBandScope(i);
            vectorscope.pushBandScope(i, bs.left.data(), bs.right.data(), bs.count);
        }

        {
            double sumL = 0.0, sumR = 0.0, sumLR = 0.0;
            for (int i = 0; i < snap.scopeCount; ++i)
            {
                auto l = static_cast<double>(snap.scopeLeft[i]);
                auto r = static_cast<double>(snap.scopeRight[i]);
                sumL += l * l; sumR += r * r; sumLR += l * r;
            }
            vectorscope.setHasSignal((sumL + sumR) > 1e-2);
            auto denom = std::sqrt(sumL * sumR);
            vectorscope.setCorrelation(denom > 1e-12 ? static_cast<float>(sumLR / denom) : 0.0f);
        }

        auto maxFreq = static_cast<float>(audioProcessor.getCurrentSampleRate()) * 0.5f;
        spectrumCrossoverControls.pushSpectrum(snap.spectrum, AudioAnalyzer::numBins, maxFreq);

        bool muted[5]{}, soloed[5]{};
        bool anySolo = false;
        for (int i = 0; i < numBands; ++i)
        {
            muted[i] = audioProcessor.isBandMuted(i);
            soloed[i] = audioProcessor.isBandSoloed(i);
            if (soloed[i]) anySolo = true;
        }
        for (int i = 0; i < numBands; ++i)
            bandStrips[i]->setHasOtherSolo(anySolo && !soloed[i]);

        spectrumCrossoverControls.setBandStates(muted, soloed, numBands);

    }
    else
    {
        vectorscope.setHasSignal(false);
    }
    vectorscope.tickSmoothing();
}

void ImageStereoMultibandAudioProcessorEditor::updateBandVisibility()
{
    const int numActive = audioProcessor.getNumBands();
    const bool canAdd = audioProcessor.canAddBand();
    const bool canRemove = audioProcessor.canRemoveBand();

    for (int i = 0; i < 5; ++i)
        bandStrips[i]->setVisible(i < numActive);

    addBandBtn.setVisible(true);
    addBandBtn.setEnabled(canAdd);
    removeBandBtn.setVisible(true);
    removeBandBtn.setEnabled(canRemove);

    vectorscope.setNumBands(numActive);

    bool muted[5]{}, soloed[5]{};
    bool anySolo = false;
    for (int i = 0; i < numActive; ++i)
    {
        muted[i] = audioProcessor.isBandMuted(i);
        soloed[i] = audioProcessor.isBandSoloed(i);
        if (soloed[i]) anySolo = true;
    }
    for (int i = 0; i < numActive; ++i)
        bandStrips[i]->setHasOtherSolo(anySolo && !soloed[i]);

    spectrumCrossoverControls.setBandStates(muted, soloed, numActive);
}
