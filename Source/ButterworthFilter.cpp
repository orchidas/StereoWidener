/*
  ==============================================================================

    ButterworthFilter.cpp
    Created: 2 Oct 2023 8:29:46pm
    Author:  Orchisama Das

  ==============================================================================
*/

#include "ButterworthFilter.h"
ButterworthFilter::ButterworthFilter(){}
ButterworthFilter::~ButterworthFilter(){
    for(int i = 0; i < numBiquads; i++){
        delete [] a[i];
        delete [] b[i];
    }
    delete [] a;
    delete [] b;
}

void ButterworthFilter::initialize(float sR, float prewarp_frequency, std::string filter_type){
    numBiquads = order / 2;
    sample_rate = sR;
    lowpass = (filter_type == "lowpass")? true : false;
    bilinear_warp_factor = 2 * PI * prewarp_frequency / std::tan(PI * prewarp_frequency/sample_rate);
    a = new float* [numBiquads];
    b = new float* [numBiquads];
    for(int i = 0; i < numBiquads; i++){
        a[i] = new float [2];
        b[i] = new float [3];
        a[i][0] = lowpass == true? 2.0 : -2.0;
        a[i][1] = 1.0;
    }
    setCoefficients();
    biquadCascade.initialize(numBiquads, sample_rate, b, a);
    
}

void ButterworthFilter::update(float newCutoffFreq){
    cutoff_frequency = newCutoffFreq;
    setCoefficients();
    biquadCascade.update(b, a);
}


void ButterworthFilter::setCoefficients(){
    float w_c = 2 * PI * cutoff_frequency;
    float frac = bilinear_warp_factor / w_c;
    for (int k = 0; k < numBiquads; k++){
        if (!lowpass)
            frac = 1.0 / frac;
        
        b[k][0] = std::pow(frac - 1, 2);
        b[k][1] = 2 * (1 - std::pow(frac, 2)) * std::cos((PI*2*(k+1) + order - 1) / (2 * order));
        b[k][2] = b[k][0];

    }
}

float ButterworthFilter::process(const float input){
    return biquadCascade.process(input);
}

