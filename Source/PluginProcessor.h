/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DSP/Band/Band.h"
#include "DSP/MultibandSplitter/MultibandSplitter.h"
#include "DSP/Analyzer/AudioAnalyzer.h"



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
    juce::AudioProcessorValueTreeState& getAPVTS();

    AudioAnalyzer& getAnalyzer() { return analyzer; }
    const std::array<float, 4>& getCrossovers() const { return currentCrossovers; }
    double getCurrentSampleRate() const { return currentSampleRate; }
    bool isBandMuted(int index) const { return index >= 0 && index < numBands ? bands[index].isMuted() : false; }
    bool isBandSoloed(int index) const { return index >= 0 && index < numBands ? bands[index].isSolo() : false; }

    int getNumBands() const { return numBands; }
    bool canAddBand() const { return numBands < 5; }
    bool canRemoveBand() const { return numBands > 2; }
    void setNumBands(int n);

    struct BandScopeBuffer
    {
        static constexpr int size = 512;
        std::array<float, size> left{};
        std::array<float, size> right{};
        int count = 0;

    private:
        int pos = 0;
        friend class ImageStereoMultibandAudioProcessor;
    };

    const BandScopeBuffer& getBandScope(int index) const { return bandScopes[static_cast<size_t>(index)]; }

private:
    int numBands = 2;

    void updateParameters();

    bool hasAnySolo() const;
    void applySoloLogic();

    //==========================================================================
    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;

    //==========================================================================
    MultibandSplitter splitter;

    static constexpr int maxNumBands = 5;

    std::array<Band, maxNumBands> bands;

    std::array<juce::AudioBuffer<float>, maxNumBands> bandBuffers;

    std::array<float, 4> currentCrossovers
    {
        120.0f,
        500.0f,
        2000.0f,
        8000.0f
    };

    AudioAnalyzer analyzer;
    std::array<BandScopeBuffer, maxNumBands> bandScopes;
    double currentSampleRate = 44100.0;

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageStereoMultibandAudioProcessor)
};
