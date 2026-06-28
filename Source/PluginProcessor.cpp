/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/Midside/MidSide.h"

//==============================================================================

ImageStereoMultibandAudioProcessor::ImageStereoMultibandAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
        BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    apvts(*this,
        &undoManager,
        "Parameters",
        createParameters())
#endif
{
}

ImageStereoMultibandAudioProcessor::~ImageStereoMultibandAudioProcessor()
{
}

//==============================================================================
// Parámetros

juce::AudioProcessorValueTreeState::ParameterLayout
ImageStereoMultibandAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    for (int i = 1; i <= maxNumBands; ++i)
    {
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(
                "band" + juce::String(i) + "Width",
                "Band " + juce::String(i) + " Width",
                juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
                50.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(
                "band" + juce::String(i) + "Gain",
                "Band " + juce::String(i) + " Gain",
                -24.0f,
                24.0f,
                0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterBool>(
                "band" + juce::String(i) + "Mute",
                "Band " + juce::String(i) + " Mute",
                false));

        params.push_back(
            std::make_unique<juce::AudioParameterBool>(
                "band" + juce::String(i) + "Solo",
                "Band " + juce::String(i) + " Solo",
                false));
    }

    std::array<float, 4> defaultCrossovers{
        120.0f,
        500.0f,
        2000.0f,
        8000.0f
    };

    for (int i = 0; i < 4; ++i)
    {
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(
                "crossover" + juce::String(i + 1),
                "Crossover " + juce::String(i + 1),
                20.0f,
                20000.0f,
                defaultCrossovers[i]));
    }

    params.push_back(
        std::make_unique<juce::AudioParameterBool>(
            "bypass",
            "Bypass",
            false));

    return { params.begin(), params.end() };
}

//==============================================================================

const juce::String ImageStereoMultibandAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ImageStereoMultibandAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool ImageStereoMultibandAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool ImageStereoMultibandAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double ImageStereoMultibandAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ImageStereoMultibandAudioProcessor::getNumPrograms()
{
    return 1;
}

int ImageStereoMultibandAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ImageStereoMultibandAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String ImageStereoMultibandAudioProcessor::getProgramName(int index)
{
    return {};
}

void ImageStereoMultibandAudioProcessor::changeProgramName(
    int index,
    const juce::String& newName)
{
}

//==============================================================================
// Audio

void ImageStereoMultibandAudioProcessor::prepareToPlay(
    double sampleRate,
    int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    splitter.prepare(spec);

    for (auto& band : bands)
        band.prepare(spec);

    for (auto& buffer : bandBuffers)
    {
        buffer.setSize(
            getTotalNumOutputChannels(),
            samplesPerBlock);
    }

    analyzer.prepare(sampleRate, samplesPerBlock);
    currentSampleRate = sampleRate;

    bypassMix.reset(sampleRate, 0.05); // 50 ms
    bypassMix.setCurrentAndTargetValue(1.0f);

    dryBuffer.setSize(
        getTotalNumOutputChannels(),
        samplesPerBlock);
}

void ImageStereoMultibandAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations

bool ImageStereoMultibandAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect

    juce::ignoreUnused(layouts);
    return true;

#else

    if (layouts.getMainOutputChannelSet()
        != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet()
        != juce::AudioChannelSet::stereo())
    {
        return false;
    }

#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet()
        != layouts.getMainInputChannelSet())
    {
        return false;
    }
#endif

    return true;

#endif
}

#endif

//==============================================================================
// Procesamiento

void ImageStereoMultibandAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    updateParameters();

    // Guardar señal original antes de cualquier procesamiento
    dryBuffer.makeCopyOf(buffer);

    if (!bypassed)
    {
        splitter.setNumBands(numBands);

        splitter.process(buffer, bandBuffers);

        for (int i = 0; i < numBands; ++i)
            bands[i].process(bandBuffers[i]);

        applySoloLogic();

        for (int b = 0; b < numBands; ++b)
        {
            auto& buf = bandScopes[static_cast<size_t>(b)];
            auto& bandBuf = bandBuffers[static_cast<size_t>(b)];
            auto numSamples = bandBuf.getNumSamples();
            auto* l = bandBuf.getReadPointer(0);
            auto* r = bandBuf.getReadPointer(1);
            for (int s = 0; s < numSamples; ++s)
            {
                buf.left[static_cast<size_t>(buf.pos)] = l[s];
                buf.right[static_cast<size_t>(buf.pos)] = r[s];
                buf.pos = (buf.pos + 1) % BandScopeBuffer::size;
                if (buf.count < BandScopeBuffer::size)
                    ++buf.count;
            }
        }

        buffer.clear();

        for (int band = 0; band < numBands; ++band)
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                buffer.addFrom(
                    ch,
                    0,
                    bandBuffers[band],
                    ch,
                    0,
                    buffer.getNumSamples());
            }
        }

        analyzer.process(buffer);
    }

    // Bypass: crossfade suave entre procesado (wet) y se�al original (dry)
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        auto mix = bypassMix.getNextValue();

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto wet = buffer.getSample(ch, sample);
            auto dry = dryBuffer.getSample(ch, sample);
            buffer.setSample(ch, sample, dry + (wet - dry) * mix);
        }
    }


}

//==============================================================================

void ImageStereoMultibandAudioProcessor::updateParameters()
{
    for (int i = 0; i < numBands; ++i)
    {
        auto bandId = juce::String(i + 1);

        {
            auto* wParam = apvts.getParameter("band" + bandId + "Width");
            auto wNorm = apvts.getRawParameterValue("band" + bandId + "Width")->load();
            bands[i].setWidth(wParam != nullptr ? wParam->convertFrom0to1(wNorm) : wNorm);
        }

        bands[i].setGain(
            apvts.getRawParameterValue(
                "band" + bandId + "Gain")->load());

        bands[i].setMute(
            apvts.getRawParameterValue(
                "band" + bandId + "Mute")->load() > 0.5f);

        bands[i].setSolo(
            apvts.getRawParameterValue(
                "band" + bandId + "Solo")->load() > 0.5f);
    }

    constexpr float minBandWidth = 100.0f;
    const int numCrossovers = numBands - 1;

    for (int i = 0; i < 4; ++i)
    {
        auto freq = apvts.getRawParameterValue("crossover" + juce::String(i + 1))->load();

        if (i < numCrossovers)
        {
            float lower = (i == 0) ? 20.0f : currentCrossovers[i - 1] + minBandWidth;
            float upper = 20000.0f - (numCrossovers - 1 - i) * minBandWidth;
            freq = juce::jlimit(lower, upper, freq);
            currentCrossovers[i] = freq;
            splitter.setFrequency(i, freq);
        }
    }


    setBypassed(
        apvts.getRawParameterValue("bypass")->load() > 0.5f);
}

//==============================================================================

bool ImageStereoMultibandAudioProcessor::hasAnySolo() const
{
    for (int i = 0; i < numBands; ++i)
    {
        if (bands[i].isSolo())
            return true;
    }

    return false;
}

void ImageStereoMultibandAudioProcessor::applySoloLogic()
{
    const bool anySolo = hasAnySolo();

    for (int i = 0; i < numBands; ++i)
    {
        float targetGain = 1.0f;

        if (anySolo)
        {
            targetGain = bands[i].isSolo() ? 1.0f : 0.0f;
        }
        else
        {
            targetGain = bands[i].isMuted() ? 0.0f : 1.0f;
        }

        bands[i].setLevelTarget(targetGain);
    }

    // Inactive bands get zero gain
    for (int i = numBands; i < 5; ++i)
        bands[i].setLevelTarget(0.0f);
}

//==============================================================================
// Editor

bool ImageStereoMultibandAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor*
ImageStereoMultibandAudioProcessor::createEditor()
{
    return new ImageStereoMultibandAudioProcessorEditor(*this);
}

//==============================================================================
// Estado

void ImageStereoMultibandAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    std::unique_ptr<juce::XmlElement> xml(
        state.createXml());

    copyXmlToBinary(*xml, destData);
}

void ImageStereoMultibandAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(
        getXmlFromBinary(data, sizeInBytes));

    if (xml != nullptr)
    {
        if (xml->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState(
                juce::ValueTree::fromXml(*xml));
        }
    }
}

//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ImageStereoMultibandAudioProcessor();
}

juce::AudioProcessorValueTreeState& ImageStereoMultibandAudioProcessor::getAPVTS()
{
    return apvts;
}

void ImageStereoMultibandAudioProcessor::setNumBands(int n)
{
    numBands = juce::jlimit(2, 5, n);
    splitter.setNumBands(numBands);
}

void ImageStereoMultibandAudioProcessor::setBypassed(bool shouldBypass)
{
    if (bypassed == shouldBypass)
        return;

    bypassed = shouldBypass;

    bypassMix.setTargetValue(
        shouldBypass ? 0.0f : 1.0f);
}
