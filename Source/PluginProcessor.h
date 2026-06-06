/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DSP/Band/Band.h"
#include "DSP/MultibandSplitter/MultibandSplitter.h"



//==============================================================================
/**
*/
class ImageStereoMultibandAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    ImageStereoMultibandAudioProcessor();
    ~ImageStereoMultibandAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    //getStateInformation y setStateInformation son métodos que se utilizan para guardar y restaurar el estado del plugin. 
    // Estos métodos permiten que el host de audio guarde y cargue los parámetros del plugin, 
    // lo que es útil para la automatización y la recuperación de configuraciones.
    //tambien sirve cuando cierro la app y al reabrirlo vuelvo a tener los mismos parametros que tenia antes de cerrar la app
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;


    //ParameterLayout es una clase de Juce que contiene un conjunto de RangedAudioParameters y AudioProcessorParameterGroups que contienen RangedAudioParameters. 
    // Esta clase se utiliza en el constructor de AudioProcessorValueTreeState para permitir que se pasen arbitrariamente agrupados RangedAudioParameters a un AudioProcessor.
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

private:
    static constexpr int numBands = 5;

    void updateParameters();

    bool hasAnySolo() const;
    void applySoloLogic();

    //==========================================================================
    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;

    //==========================================================================
    MultibandSplitter splitter;
    juce::SmoothedValue<float> bypassMix;

    juce::AudioBuffer<float> dryBuffer;

    bool bypassed = false;

    void setBypassed(bool shouldBypass);

    std::array<Band, numBands> bands;

    std::array<juce::AudioBuffer<float>, numBands> bandBuffers;

    std::array<float, 4> currentCrossovers
    {
        120.0f,
        500.0f,
        2000.0f,
        8000.0f
    };

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageStereoMultibandAudioProcessor)
};