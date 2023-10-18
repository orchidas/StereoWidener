/*
  ==============================================================================

    BiquadCascade.cpp
    Created: 2 Oct 2023 7:59:25pm
    Author:  Orchisama Das

  ==============================================================================
*/

#include "BiquadCascade.h"
BiquadCascade::BiquadCascade(){}
BiquadCascade::~BiquadCascade(){
    delete [] biquads;
}

void BiquadCascade::initialize(int numBq, float sR, float** b, float** a){
    sampleRate = sR;
    numBiquads = numBq;
    biquads = new BiquadFilter[numBiquads];
    
    for(int i = 0; i < numBiquads; i++){
        float a0 = a[i][0];
        float a1 = a[i][1];
        float b0 = b[i][0];
        float b1 = b[i][1];
        float b2 = b[i][2];
        biquads[i].initialize(b0, b1, b2, a0, a1);
    }
}

void BiquadCascade::update(float** b_new, float** a_new){
    for(int i = 0; i < numBiquads; i++){
        float a0 = a_new[i][0];
        float a1 = a_new[i][1];
        float b0 = b_new[i][0];
        float b1 = b_new[i][1];
        float b2 = b_new[i][2];
        biquads[i].update(b0, b1, b2, a0, a1);
    }
}


float BiquadCascade::process(const float input){
    float curOutput = 0.0f;
    float curInput = input;
    for(int i = 0; i < numBiquads; i++){
        curOutput = biquads[i].process(curInput);
        curInput = curOutput;
    }
    return curOutput;
}
