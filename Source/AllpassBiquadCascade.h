/*
  ==============================================================================

    BiquadCascade.h
    Created: 2 Jun 2023 9:18:50pm
    Author:  Orchisama Das

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include "BiquadFilter.h"
#include <random>
#include <complex>

class AllpassBiquad : public BiquadFilter{
    //allpass biquad filter parameterised by pole radius and angle
public:
    AllpassBiquad();
    ~AllpassBiquad();
    
    void initialize(float pole_radii, float pole_angle);
    float process(const float input);
    
    
private:
    const float PI = std::acos(-1);     //PI
    const int order = 2;
    std::complex<float> I;              //Imaginary number i

};

//-----------------------------------------------------------------

class AllpassBiquadCascade{
    //allpass biquad filter cascade
public:
    AllpassBiquadCascade();
    ~AllpassBiquadCascade();
    
    void initialize(int numBq, float sR, float maxGroupDelayMs);
    float warpPoleAngle(float pole_angle);
    float process(const float input);
    
    
private:
    int numBiquads;
    float sampleRate;
    float warpFactor;                   //for ERB warping of pole angles
    const float PI = std::acos(-1);
    std::complex<float> I;              //Imaginary number i
    AllpassBiquad* biquads;
};
