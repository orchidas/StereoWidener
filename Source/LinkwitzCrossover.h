/*
  ==============================================================================

    LinkwitzCrossover.h
    Created: 1 Jun 2023 6:18:50pm
    Author:  Orchisama Das

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include "DelayLine.h"

class LinkwitzCrossover{
    /* 4th order Linkwitz Riley crossover filters */
public:
    LinkwitzCrossover();
    ~LinkwitzCrossover();
    
    void initialize(float sR, std::string type);
    float process(const float input);
    void update(float newCutoffFreq);
    void setCoefficients();
    
    
private:
    const float PI = std::acos(-1);
    const int order = 2;         //4th order IIR filter
    float sampleRate;            //sample rate in Hz
    float cutoff = 500;          //cutoff frequency in Hz
    float* numCoeffs;
    float* denCoeffs;
    float* prevInput;
    float* prevOutput;
    bool lowpass;                //if filter is lowpass or highpass

};
