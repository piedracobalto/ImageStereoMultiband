/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DSP/Midside/MidSide.h"
#include "DSP/Band/Band.h"
#include "DSP/Crossover/Crossover.h"

//==============================================================================
/**
*/
class ImageStereoMultibandAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    ImageStereoMultibandAudioProcessor();
    ~ImageStereoMultibandAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
	//getStateInformation y setStateInformation son métodos que se utilizan para guardar y restaurar el estado del plugin. 
    // Estos métodos permiten que el host de audio guarde y cargue los parámetros del plugin, 
    // lo que es útil para la automatización y la recuperación de configuraciones.
	//tambien sirve cuando cierro la app y al reabrirlo vuelvo a tener los mismos parametros que tenia antes de cerrar la app
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

	
	//ParameterLayout es una clase de Juce que contiene un conjunto de RangedAudioParameters y AudioProcessorParameterGroups que contienen RangedAudioParameters. 
    // Esta clase se utiliza en el constructor de AudioProcessorValueTreeState para permitir que se pasen arbitrariamente agrupados RangedAudioParameters a un AudioProcessor.
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

private:
    static constexpr int numBands = 2;


    //undoManager es una clase de Juce que permite deshacer y rehacer cambios en los parámetros del plugin.
    juce::UndoManager undoManager;

    //AudioProcessorValueTreeState es una clase de Juce que maneja datos y estado del audioProcessor y contiene los parametros utilizados por el plugin
    juce::AudioProcessorValueTreeState apvts;
    
	void updateParameters();
    bool hasAnySolo() const;
    void applySoloLogic();

    Crossover crossover;

    juce::AudioBuffer<float> lowBandBuffer;
    juce::AudioBuffer<float> highBandBuffer;

    std::array<Band, numBands> bands;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageStereoMultibandAudioProcessor)
};
