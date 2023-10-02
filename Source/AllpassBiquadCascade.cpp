/*
  ==============================================================================

    BiquadCascade.cpp
    Created: 2 Jun 2023 9:18:50pm
    Author:  Orchisama Das

  ==============================================================================
*/

#include "AllpassBiquadCascade.h"

AllpassBiquad::AllpassBiquad(){}
AllpassBiquad::~AllpassBiquad(){}


void AllpassBiquad::initialize(float pole_radii, float pole_angle){
    I.real(0); I.imag(1);                   // complex number 0 + 1i
    std::complex<float> pole = pole_radii * std::exp(2 * PI * I * pole_angle);
    float a0 = -2 * std::real(pole);
    float a1 = std::pow(std::abs(pole),2);
    float b0 = a1;
    float b1 = a0;
    float b2 = 1.0;
    BiquadFilter::initialize(b0, b1, b2, a0, a1);
}

float AllpassBiquad::process(const float input){
    return BiquadFilter::process(input);
}

//------------------------------------------------------------------------------

AllpassBiquadCascade::AllpassBiquadCascade(){}
AllpassBiquadCascade::~AllpassBiquadCascade(){
    delete [] biquads;
}

float AllpassBiquadCascade::warpPoleAngle(float pole_angle){
    std::complex <float> pole_warped = std::exp(I * pole_angle);
    std::complex <float>lambdam = std::log((warpFactor + pole_warped) / (1.0f + warpFactor * pole_warped));
    return std::imag(lambdam);
}


void AllpassBiquadCascade::initialize(int numBq, float sR, float maxGroupDelayMs){
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


float AllpassBiquadCascade::process(const float input){
    float curOutput = 0.0f;
    float curInput = input;
    for(int i = 0; i < numBiquads; i++){
        curOutput = biquads[i].process(curInput);
        curInput = curOutput;
    }
    //std::cout << "Cascade INPUT :" << input << ", Cascade OUTPUT :" << curOutput << std::endl;
    return curOutput;
}
