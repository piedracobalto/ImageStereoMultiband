/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

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
    juce::AudioProcessorValueTreeState::ParameterLayout parameters;
	// Agregar los parámetros que desees aquí, por ejemplo:
	// aca se agrega un parametro de tipo float  con el parametro ID "gain" que hace unico la manera de identificarlo, 
    // nombre "Gain", 
    // rango de 0.0 a 1.0 
    // y valor por defecto de 0.5

	parameters.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("gain", 1), "Gain", 0.0f, 1.0f, 0.5f));

    return parameters;
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
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
}

//==============================================================================
bool ImageStereoMultibandAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ImageStereoMultibandAudioProcessor::createEditor()
{
    return new ImageStereoMultibandAudioProcessorEditor (*this);

	//cuando no se tiene un editor personalizado, se puede usar el editor generico de JUCE
	//return new juce::GenericAudioProcessorEditor(*this); // Use the generic editor
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
