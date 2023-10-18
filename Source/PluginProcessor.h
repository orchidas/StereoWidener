/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "VelvetNoise.h"
#include "Panner.h"
#include "LinkwitzCrossover.h"
#include "ButterworthFilter.h"
#include "AllpassBiquadCascade.h"
//==============================================================================
/**
*/
class StereoWidenerAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    StereoWidenerAudioProcessor();
    ~StereoWidenerAudioProcessor() override;

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
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    inline float onePoleFilter(float input, float previous_output);

    //Input parameters
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* widthLower;         //stereo width (0 - original, 100 - max widening)
    std::atomic<float>* widthHigher;        
    std::atomic<float>* cutoffFrequency;    //filterbank cutoff frequency
    std::atomic<float>* isAmpPreserve;      //calculations are amplitude or energy preserving
    std::atomic<float>* hasAllpassDecorrelation; //what decorrelator to use - VN or AP
    const int numFreqBands = 2;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoWidenerAudioProcessor)
    const int numChannels = getMainBusNumInputChannels();
    const float PI = std::acos(-1);
    
    VelvetNoise* velvetSequence;
    AllpassBiquadCascade* allpassCascade;
    Panner* pan;
    LinkwitzCrossover** amp_preserve_filters;
    ButterworthFilter** energy_preserve_filters;
    int density = 1000;
    float targetDecaydB = 10.;
    bool logDistribution = true;                    //whether to concentrate VN impulses at the beginning
    bool prevAllpassDecorr = false;                 //whether to use allpass or velvet noise for decorrelation
    bool prevAmpPreserveFlag = false;               //whether to use amp. based or energetic calculations
    float* pannerInputs;
    float* temp_output;
    float* gain_multiplier;
    float prevWidthLower, curWidthLower;
    float prevWidthHigher, curWidthHigher;
    float prevCutoffFreq, curCutoffFreq;
    float smooth_factor;   //one pole filter for parameter update
    enum{
        vnLenMs = 15,
        smoothingTimeMs = 5,
        maxGroupDelayMs = 30,
        numBiquads = 200,
        prewarpFreqHz = 1000,
    };
    std::vector<std::vector<float>> inputData;

};
