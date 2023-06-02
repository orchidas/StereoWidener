/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
StereoWidenerAudioProcessor::StereoWidenerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    parameters(*this, nullptr, juce::Identifier ("StereoWidener"),{
    std::make_unique<juce::AudioParameterFloat>
    ("widthLower", // parameterID
     "Lower frequency width", // parameter name
     0.0f,   // minimum value
     100.0f,   // maximum value
     0.0f),
    std::make_unique<juce::AudioParameterFloat>
    ("widthHigher", // parameterID
     "Higher frequency width", // parameter name
     0.0f,   // minimum value
     100.0f,   // maximum value
     0.0f),
    std::make_unique<juce::AudioParameterFloat>
    ("cutoffFrequency", // parameterID
     "Filter cutoff frequency", // parameter name
     200.0f,   // minimum value
     8000.0f,   // maximum value
     0.0f),
    }) // default value)
#endif
{
    //set user defined parameters
    widthLower = parameters.getRawParameterValue("widthLower");
    widthHigher = parameters.getRawParameterValue("widthHigher");
    cutoffFrequency = parameters.getRawParameterValue("cutoffFrequency");

}

StereoWidenerAudioProcessor::~StereoWidenerAudioProcessor()
{
    delete [] pannerInputs;
    delete [] temp_output;
    delete [] pan;
    delete [] vnSeq;
    for (int i = 0; i < 2 * numFreqBands; i++){
        delete [] filters[i];
    }
    
}

//==============================================================================
const juce::String StereoWidenerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool StereoWidenerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool StereoWidenerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool StereoWidenerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double StereoWidenerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int StereoWidenerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int StereoWidenerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void StereoWidenerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String StereoWidenerAudioProcessor::getProgramName (int index)
{
    return {};
}

void StereoWidenerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void StereoWidenerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    vnSeq = new VelvetNoise[numChannels];
    pan = new Panner[numFreqBands * numChannels];
    filters = new LinkwitzCrossover* [numFreqBands * numChannels];
    temp_output = new float[numFreqBands];
    pannerInputs = new float[numChannels];
    int count = 0;

    for(int k = 0; k < numChannels; k++){
        
        vnSeq[k].initialize(sampleRate, vnLenMs, density, targetDecaydB);
        pannerInputs[k] = 0.f;

        for (int i = 0; i < numFreqBands; i++){
            pan[count].initialize();
            temp_output[i] = 0.0;
            filters[count] = new LinkwitzCrossover[numChannels];
            
           //0, 2 contains lowpass filter and 1, 3 contains highpass filter
            for (int j = 0; j < numChannels; j++){
                if (count % numChannels == 0)
                    filters[count][j].initialize(sampleRate, "lowpass");
                else
                    filters[count][j].initialize(sampleRate, "highpass");
            }
            count++;
        }
    }
    
    inputData = std::vector<std::vector<float>>(samplesPerBlock, std::vector<float>(numChannels, 0.0f));
    prevWidthLower = 0.f;
    prevWidthHigher = 0.0f;
    prevCutoffFreq = 500.0;

}

void StereoWidenerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool StereoWidenerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void StereoWidenerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    //update parameter
    //panners 0 and 2 have lowpassed signals
    //panners 1 and 3 have highpass signals
    if (prevWidthLower != *widthLower) {
            pan[0].update(*widthLower/100.0);
            pan[2].update(*widthLower/100.0);
            prevWidthLower = *widthLower;
        }
    
    if (prevWidthHigher != *widthHigher){
        pan[1].update(*widthHigher/100.0);
        pan[3].update(*widthLower/100.0);
        prevWidthHigher = *widthHigher;
    }
    
    if (prevCutoffFreq != *cutoffFrequency){
        int count = 0;
        for(int k = 0; k < numChannels; k++){
            for (int i = 0; i < numFreqBands; i++){
                for (int j = 0; j < numChannels; j++)
                    filters[count][j].update(*cutoffFrequency);
                count++;
            }
        }
        prevCutoffFreq = *cutoffFrequency;
    }
    
    
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();
    jassert(totalNumOutputChannels == totalNumInputChannels);
        
    // read input data into multidimensional array
    for(int chan = 0; chan < totalNumInputChannels; chan++){
        const float* channelInData = buffer.getReadPointer(chan, 0);

        for (int i = 0; i < numSamples; i++){
            inputData[i][chan] = channelInData[i];
        }
    }
    
    //process input to get output
    for (int i = 0; i < numSamples; i++){
        int count = 0;
        for(int chan = 0; chan < totalNumOutputChannels; chan++){
            //decorrelate input channel by convolving with VN sequence
            float output = 0.0f;
            float vn_output = vnSeq[chan].process(inputData[i][chan]);
            
            //process in frequency bands
            for(int k = 0; k < numFreqBands; k++){
                //filter input and VN output
                float filtered_input = filters[k][chan].process(inputData[i][chan]);
                float filtered_vn_output = filters[numFreqBands + k][chan].process(vn_output);
                //send filtered signals to panner
                pannerInputs[0] = filtered_vn_output;
                pannerInputs[1] = filtered_input;
                float *panner_output = pan[count++].process(pannerInputs);
                temp_output[k] = panner_output[0] + panner_output[1];
                output += temp_output[k];
            }
            //output channels from panner are added
            buffer.setSample(chan, i, output);
        }
    }
}
    

//==============================================================================
bool StereoWidenerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* StereoWidenerAudioProcessor::createEditor()
{
    return new StereoWidenerAudioProcessorEditor (*this, parameters);
}

//==============================================================================
void StereoWidenerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void StereoWidenerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
        
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StereoWidenerAudioProcessor();
}
