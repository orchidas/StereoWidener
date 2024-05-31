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
    (juce::ParameterID{"widthLower",1}, // parameterID and parameter version
     "Lower frequency width", // parameter name
     0.0f,   // minimum value
     100.0f,   // maximum value
     0.0f),    // initial value
    std::make_unique<juce::AudioParameterFloat>
    (juce::ParameterID{"widthHigher",1}, // parameterID
     "Higher frequency width", // parameter name
     0.0f,   // minimum value
     100.0f,   // maximum value
     0.0f),
    std::make_unique<juce::AudioParameterFloat>
    (juce::ParameterID{"cutoffFrequency",1}, // parameterID
     "Filter cutoff frequency", // parameter name
     100.0f,   // minimum value
     4000.0f,   // maximum value
     0.0f),
    std::make_unique<juce::AudioParameterInt>
      (juce::ParameterID{"isAmpPreserve",1},
       "Amplitude preserve",
       0, 1, 0),
    std::make_unique<juce::AudioParameterInt>
        (juce::ParameterID{"hasAllpassDecorrelation",1},
         "Allpass decorrelation",
         0, 1, 0),
    std::make_unique<juce::AudioParameterInt>
      (juce::ParameterID{"handleTransients",1},
       "Transient detection",
       0, 1, 0),
    })
#endif
{
    //set user defined parameters
    widthLower = parameters.getRawParameterValue("widthLower");
    widthHigher = parameters.getRawParameterValue("widthHigher");
    cutoffFrequency = parameters.getRawParameterValue("cutoffFrequency");
    isAmpPreserve = parameters.getRawParameterValue("isAmpPreserve");
    hasAllpassDecorrelation = parameters.getRawParameterValue("hasAllpassDecorrelation");
    handleTransients = parameters.getRawParameterValue("handleTransients");

}

StereoWidenerAudioProcessor::~StereoWidenerAudioProcessor(){}

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

//read optimised VN file
juce::StringArray StereoWidenerAudioProcessor::initialise_velvet_from_binary_file(){
    auto input = juce::MemoryInputStream(BinaryData::opt_vn_filters_txt, BinaryData::opt_vn_filters_txtSize, false);
    //read the entire text file as a string
    const juce::String fileAsString = input.readEntireStreamAsString();
    //break each line into a new string
    auto opt_velvet_arrays = juce::StringArray::fromLines(juce::StringRef(fileAsString));
    return opt_velvet_arrays;
}


//==============================================================================
void StereoWidenerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    allpassCascade = new AllpassBiquadCascade[numChannels];
    velvetSequence = new VelvetNoise[numChannels];
    juce::StringArray opt_velvet_arrays = initialise_velvet_from_binary_file();
    
    pan = new Panner[numFreqBands * numChannels];
    amp_preserve_filters = new LinkwitzCrossover* [numFreqBands * numChannels];
    energy_preserve_filters = new ButterworthFilter* [numFreqBands * numChannels];
    gain_multiplier = new float[numFreqBands];
    temp_output = new float[numFreqBands];
    pannerInputs = new float[numChannels];
    transient_handler = new TransientHandler[numChannels];
    final_output = new float* [numChannels];
    
    int count = 0;

    for(int k = 0; k < numChannels; k++){
        //initialise transient handler
        if (handleTransients)
            transient_handler[k].prepare(samplesPerBlock, sampleRate);
        
        //initialise decorrelators
        allpassCascade[k].initialize(numBiquads, sampleRate, maxGroupDelayMs);
        
        if (useOptVelvetFilters){
            velvetSequence[k].initialize_from_string(opt_velvet_arrays[k]);
        }
        else{
            velvetSequence[k].initialize(sampleRate, vnLenMs, density, targetDecaydB, logDistribution);
        }
        
        //initialise panner inputs
        pannerInputs[k] = 0.f;
        //final output buffer
        final_output[k] = new float[samplesPerBlock];

        for (int i = 0; i < numFreqBands; i++){
            temp_output[i] = 0.0;
            gain_multiplier[i] =  (i == 0) ? 0.f : 1.f;
            
            //initialise panner
            pan[count].initialize();
            amp_preserve_filters[count] = new LinkwitzCrossover[numChannels];
            energy_preserve_filters[count] = new ButterworthFilter[numChannels];
            
           //0, 2 contains lowpass filter and 1, 3 contains highpass filter
            for (int j = 0; j < numChannels; j++){
                //initialise filters
                if (count % numChannels == 0){
                    amp_preserve_filters[count][j].initialize(sampleRate, "lowpass");
                    energy_preserve_filters[count][j].initialize(sampleRate, prewarpFreqHz, "lowpass");
                }
                else{
                    amp_preserve_filters[count][j].initialize(sampleRate, "highpass");
                    energy_preserve_filters[count][j].initialize(sampleRate, prewarpFreqHz, "highpass");
                }
            }
            count++;
        }
    }
    
    inputData = std::vector<std::vector<float>>(numChannels, std::vector<float>(samplesPerBlock, 0.0f));
    outputData = std::vector<std::vector<float>>(numChannels, std::vector<float>(samplesPerBlock, 0.0f));
    prevWidthLower = 0.f;
    curWidthLower = 0.f;
    prevWidthHigher = 0.0f;
    curWidthHigher = 0.f;
    prevCutoffFreq = 500.0f;
    smooth_factor = std::exp(-1.0f / (smoothingTimeMs * 0.001f * sampleRate));

}

void StereoWidenerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    delete [] pannerInputs;
    delete [] temp_output;
    delete [] pan;
    delete [] gain_multiplier;
    delete [] allpassCascade;
    delete [] velvetSequence;
    delete [] transient_handler;
    
    for (int i = 0; i < numChannels * numFreqBands; i++){
        delete [] amp_preserve_filters[i];
        delete [] energy_preserve_filters[i];
    }
    
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

inline float StereoWidenerAudioProcessor::onePoleFilter(float input, float previous_output){
    return (input * (1.0f-smooth_factor)) + (previous_output * smooth_factor);
}


void StereoWidenerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    int count = 0;
    
    //update parameter
    //panners 0 and 2 have lowpassed signals
    //panners 1 and 3 have highpass signals
    
    //update lowpass width
    if (prevWidthLower != *widthLower) {
        curWidthLower = onePoleFilter(*widthLower, prevWidthLower);
        pan[0].updateWidth(curWidthLower/100.0);
        pan[2].updateWidth(curWidthLower/100.0);
        prevWidthLower = curWidthLower;
    }
    
    //update highpass width
    if (prevWidthHigher != *widthHigher){
        curWidthHigher = onePoleFilter(*widthHigher, prevWidthHigher);
        pan[1].updateWidth(curWidthHigher/100.0);
        pan[3].updateWidth(curWidthHigher/100.0);
        prevWidthHigher = curWidthHigher;
    }
    
    
    //update filter cutoff frequency
    if (prevCutoffFreq != *cutoffFrequency){
        curCutoffFreq = onePoleFilter(*cutoffFrequency, prevCutoffFreq);
        count = 0;
        for(int k = 0; k < numChannels; k++){
            for (int i = 0; i < numFreqBands; i++){
                for (int j = 0; j < numChannels; j++){
                    amp_preserve_filters[count][j].update(curCutoffFreq);
                    energy_preserve_filters[count][j].update(curCutoffFreq);
                }
                count++;
            }
        }
        prevCutoffFreq = curCutoffFreq;
    }
    
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();
    jassert(totalNumOutputChannels == totalNumInputChannels);
        
    // read input data into multidimensional array
    for(int chan = 0; chan < totalNumInputChannels; chan++){
        const float* channelInData = buffer.getReadPointer(chan, 0);

        for (int i = 0; i < numSamples; i++){
            inputData[chan][i] = channelInData[i];
        }
    }
    
    //process input to get output
    for (int i = 0; i < numSamples; i++){
        count = 0;
        for(int chan = 0; chan < totalNumOutputChannels; chan++){
            float output = 0.0f;
            float decorr_output = 0.0f;
            
            //decorrelate input channel by convolving with VN sequence
            if (*hasAllpassDecorrelation)
                decorr_output = allpassCascade[chan].process(inputData[chan][i]);
            //or by passing through allpass cascade
            else
                decorr_output = velvetSequence[chan].process(inputData[chan][i]);
            
            //process in frequency bands
            for(int k = 0; k < numFreqBands; k++){
                float filtered_input = 0.0f;
                float filtered_decorr_output = 0.0f;
                //pass input and decorrelation output through filterbank
                if (*isAmpPreserve){
                    filtered_input = amp_preserve_filters[k][chan].process(inputData[chan][i]);
                    filtered_decorr_output = amp_preserve_filters[numFreqBands + k][chan].process(decorr_output);
                }
                else{
                    filtered_input = energy_preserve_filters[k][chan].process(inputData[chan][i]);
                    filtered_decorr_output = energy_preserve_filters[numFreqBands + k][chan].process(decorr_output);
                }

                pannerInputs[0] = filtered_decorr_output;
                pannerInputs[1] = filtered_input;
                float panner_output = pan[count++].process(pannerInputs);
                output += panner_output;
            }
            outputData[chan][i] = output;
            if (! *handleTransients)
                buffer.setSample(chan, i, output);
        }
    }
    
    // transient handling logic
    if (*handleTransients){
        for(int chan = 0; chan < totalNumOutputChannels; chan++){
            final_output[chan] = transient_handler[chan].process(&inputData[chan][0], &outputData[chan][0]);
            for (int i = 0; i < numSamples; i++){
                buffer.setSample(chan, i, final_output[chan][i]);
            }
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
    
    auto state = parameters.copyState();
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        copyXmlToBinary (*xml, destData);
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
