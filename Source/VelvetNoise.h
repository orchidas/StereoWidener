/*
  ==============================================================================

    VelvetNoise.h
    Created: 6 May 2023 3:52:01pm
    Author:  Orchisama Das

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include "DelayLine.h"
#include <random>


class VelvetNoise{
public:
    VelvetNoise();
    ~VelvetNoise();
    
    void initialize(float sR, float L, int gS, float targetDecaydB, bool logDistribution);
    void initialize_from_string(juce::String opt_vn_filter);
    float process(const float input);
    void update(int newGridSize);
    void setImpulseLocationValues();
    float convertdBtoDecayRate();
    
    
private:
    int length;             //total length of delay line (in samples)
    int seqLength;          //length of impulse sequence
    int gridSize;           //density of impulses
    int* impulsePositions;  //positions of impulses in sequence
    float* impulseValues;   //value at impulse positions
    float decaydB;          //decay in dB of the sequence
    float sampleRate;       //sampling rate in Hz
    bool logDistribution;   // are the impulses concentrated at the start?
    DelayLine delayLine;    //Delay line to do convolution with velvet sequence

};
