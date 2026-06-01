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
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
	 ), 
	//apvts tiene los siguiente parametros: 
    // *this una clase de AudioProcessor, 
    // nullptr (un puntero a un objeto de tipo AudioProcessorValueTreeState::Listener, que se utiliza para recibir notificaciones de cambios en los parámetros del plugin), 
    // "Parameters" (el nombre del grupo de parámetros) 
    // y createParameters() (una función que devuelve un objeto de tipo AudioProcessorValueTreeState::ParameterLayout que contiene la lista de parámetros del plugin)
    apvts(*this, &undoManager, "Parameters", createParameters())
#endif
{
}

ImageStereoMultibandAudioProcessor::~ImageStereoMultibandAudioProcessor()
{
}

// aca van a usarse los parametros que se van a usar en el plugin, por ejemplo gain, cutoff, etc
juce::AudioProcessorValueTreeState::ParameterLayout ImageStereoMultibandAudioProcessor::createParameters() {
	//listado de parametros que se van a usar en el plugin
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    for (int i = 1; i <= numBands; ++i)
    {
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(
                "band" + juce::String(i) + "Width",
                "Band " + juce::String(i) + " Width",
                0.0f,
                2.0f,
                1.0f));

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

    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            "crossoverFreq",
            "Crossover Frequency",
            20.0f,
            20000.0f,
            1000.0f));


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
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ImageStereoMultibandAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ImageStereoMultibandAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ImageStereoMultibandAudioProcessor::getProgramName (int index)
{
    return {};
}

void ImageStereoMultibandAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ImageStereoMultibandAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
	//spec es un objeto de tipo juce::dsp::ProcessSpec se utiliza para configurar las especificaciones de procesamiento de audio, como la frecuencia de muestreo, el tamaño del bloque y el número de canales.
        juce::dsp::ProcessSpec spec;
		spec.maximumBlockSize = samplesPerBlock;
		spec.sampleRate = sampleRate;
		spec.numChannels = getTotalNumOutputChannels();

		// band.prepare(spec) es un método que se llama para preparar cada banda de procesamiento de audio con las especificaciones definidas en el objeto spec.
		// contiene midside.prepare(spec) que prepara el procesamiento mid-side con las especificaciones definidas en spec.
        for (auto& band : bands)
        {
            band.prepare(spec);
        }

        crossover.prepare(spec);

        lowBandBuffer.setSize(
            getTotalNumOutputChannels(),
            samplesPerBlock);

        highBandBuffer.setSize(
            getTotalNumOutputChannels(),
            samplesPerBlock);
        
}

void ImageStereoMultibandAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ImageStereoMultibandAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif


// processBlock es el método principal donde se realiza el procesamiento de audio. Se llama cada vez que hay un nuevo bloque de audio para procesar.
void ImageStereoMultibandAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
	auto totalNumInputChannels = getTotalNumInputChannels();
	auto totalnumOutputChannels = getTotalNumOutputChannels();

    updateParameters();

    crossover.process(
        buffer,
        lowBandBuffer,
        highBandBuffer);


    bands[0].process(lowBandBuffer);
    bands[1].process(highBandBuffer);

    applySoloLogic();

    buffer.makeCopyOf(lowBandBuffer);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        buffer.addFrom(
            ch,
            0,
            highBandBuffer,
            ch,
            0,
            buffer.getNumSamples());
    }
}


void ImageStereoMultibandAudioProcessor::updateParameters()
{
    for (int i = 0; i < numBands; ++i)
    {
        auto bandId = juce::String(i + 1);

        bands[i].setWidth(
            apvts.getRawParameterValue(
                "band" + bandId + "Width")->load());

        bands[i].setGain(
            apvts.getRawParameterValue(
                "band" + bandId + "Gain")->load());


		// el valor 0.5f se utiliza como umbral para determinar si el parámetro de silencio (Mute) está activado o desactivado.
        bands[i].setMute(
            apvts.getRawParameterValue(
                "band" + bandId + "Mute")->load() > 0.5f);

        bands[i].setSolo(
            apvts.getRawParameterValue(
                "band" + bandId + "Solo")->load() > 0.5f);
    }

    crossover.setFrequency(
        apvts.getRawParameterValue(
            "crossoverFreq")->load());
}

bool ImageStereoMultibandAudioProcessor::hasAnySolo() const
{
    for (const auto& band : bands)
    {
        if (band.isSolo())
            return true;
    }

    return false;
}

void ImageStereoMultibandAudioProcessor::applySoloLogic()
{
    if (!hasAnySolo())
        return;

    if (!bands[0].isSolo())
        lowBandBuffer.clear();

    if (!bands[1].isSolo())
        highBandBuffer.clear();
}

//==============================================================================
bool ImageStereoMultibandAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ImageStereoMultibandAudioProcessor::createEditor()
{
    //return new ImageStereoMultibandAudioProcessorEditor (*this);

	//cuando no se tiene un editor personalizado, se puede usar el editor generico de JUCE
	return new juce::GenericAudioProcessorEditor(*this); // Use the generic editor
}

//==============================================================================
void ImageStereoMultibandAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void ImageStereoMultibandAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ImageStereoMultibandAudioProcessor();
}
