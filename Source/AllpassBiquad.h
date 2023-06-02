/*
  ==============================================================================

    AllpassBiquad.h
    Created: 2 Jun 2023 8:59:31pm
    Author:  Orchisama Das

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include <complex>


class AllpassBiquad{
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
    float* a;
    float* b;
    float* prevInput;
    float* prevOutput;

};
