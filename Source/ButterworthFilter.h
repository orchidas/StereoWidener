/*
  ==============================================================================

    ButterworthFilter.h
    Created: 2 Oct 2023 8:29:46pm
    Author:  Orchisama Das

  ==============================================================================
*/

#pragma once
#include "BiquadCascade.h"

class ButterworthFilter{
    //Butterworth filter of 16th order
public:
    ButterworthFilter();
    ~ButterworthFilter();
    
    void initialize(float sR, float prewarp_frequency, std::string filter_type);
    void update(float newCutoffFreq);
    void setCoefficients();
    float process(const float input);
    
    
private:
    const float PI = std::acos(-1);
    const int order = 8;       //filter order
    bool lowpass;               //type of filter, 'lowpass' or 'highpass'
    float numBiquads;
    float sample_rate;
    float cutoff_frequency = 500;    //cutoff frequency in Hz
    float bilinear_warp_factor;
    BiquadCascade biquadCascade;    //biquad cascade object
    float** a; float** b;           //filter coefficient arrays
};
