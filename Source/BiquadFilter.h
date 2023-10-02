/*
  ==============================================================================

    BiquadFilter.h
    Created: 2 Oct 2023 7:40:33pm
    Author:  Orchisama Das

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include <complex>


class BiquadFilter{
    //allpass biquad filter parameterised by pole radius and angle
public:
    BiquadFilter();
    ~BiquadFilter();
    
    void initialize(float b0, float b1, float b2, float a0, float a1);
    void update(float b0, float b1, float b2, float a0, float a1);
    float process(const float input);
    
    
private:
    const int order = 2;
    float* a;
    float* b;
    float* prevInput;
    float* prevOutput;

};
