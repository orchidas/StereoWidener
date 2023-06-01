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

    //Input parameters
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* widthLower; // stereo width (0 - original, 100 - max widening)
    std::atomic<float>* widthHigher;
    const int numFreqBands = 2;
    Panner* pan;
    LinkwitzCrossover** filters;
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoWidenerAudioProcessor)
    VelvetNoise* vnSeq;
    int density = 1000;
    float targetDecaydB = 10.;
    float* pannerInputs;
    float* temp_output;
    float prevWidthLower = 0.0f;
    float prevWidthHigher = 0.0f;
    const float staticGain = (-1 + std::sqrt(5)) / 2.;
    enum{
        vnLenMs = 30,
    };
    std::vector<std::vector<float>> inputData;

};
