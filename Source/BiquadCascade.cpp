/*
  ==============================================================================

    BiquadCascade.cpp
    Created: 2 Jun 2023 9:18:50pm
    Author:  Orchisama Das

  ==============================================================================
*/

#include "BiquadCascade.h"

BiquadCascade::BiquadCascade(){}
BiquadCascade::~BiquadCascade(){
    delete [] biquads;
}

float BiquadCascade::warpPoleAngle(float pole_angle){
    std::complex <float> pole_warped = std::exp(I * pole_angle);
    std::complex <float>lambdam = std::log((warpFactor + pole_warped) / (1.0f + warpFactor * pole_warped));
    return std::imag(lambdam);
}



void BiquadCascade::initialize(int numBq, float sR, float maxGroupDelayMs){
    I.real(0); I.imag(1);                   // complex number 0 + 1i
    sampleRate = sR;
    numBiquads = numBq;
    biquads = new AllpassBiquad[numBiquads];
    float maxGrpDel = (1.0 - maxGroupDelayMs * 1e-3) / (1.0 + maxGroupDelayMs * 1e-3);

    warpFactor =  0.7464 * std::sqrt(2.0 / PI * std::atan(0.1418 * sampleRate)) + 0.03237;
    
    //generate random pole radii and pole angle
    std::default_random_engine generator;
    //randomly diistributed between 0.5 and beta
    std::uniform_real_distribution<float> distribution_radii(0.5, maxGrpDel * 0.001f / sampleRate);
    //randomly distriibuted between 0 and 2PI in ERB scale
    std::uniform_real_distribution<float> distribution_angle(0, 2*PI);
    
    for(int i = 0; i < numBiquads; i++){
        float radius =  distribution_radii(generator);
        float angle = warpPoleAngle(distribution_angle(generator));
        biquads[i].initialize(radius, angle);
    }
}


float BiquadCascade::process(const float input){
    float curOutput = 0.0f;
    float curInput = input;
    for(int i = 0; i < numBiquads; i++){
        curOutput = biquads[i].process(curInput);
        curInput = curOutput;
    }
    //std::cout << "Cascade INPUT :" << input << ", Cascade OUTPUT :" << curOutput << std::endl;

    return curOutput;
}
