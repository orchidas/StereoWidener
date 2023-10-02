/*
  ==============================================================================

    BiquadCascade.h
    Created: 2 Oct 2023 7:59:25pm
    Author:  Orchisama Das

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include "BiquadFilter.h"
#include <random>

class BiquadCascade{
    
public:
    BiquadCascade();
    ~BiquadCascade();
    
    void initialize(int numBq, float sR, float** b, float** a);
    void update(float** b_new, float** a_new);
    float process(const float input);
    
    
private:
    int numBiquads;
    float sampleRate;
    BiquadFilter* biquads;
};
