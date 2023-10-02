/*
  ==============================================================================

    BiquadFilter.cpp
    Created: 2 Oct 2023 7:40:33pm
    Author:  Orchisama Das

  ==============================================================================
*/

#include "BiquadFilter.h"

BiquadFilter::BiquadFilter(){}
BiquadFilter::~BiquadFilter(){
    delete [] a;
    delete [] b;
    delete [] prevInput;
    delete [] prevOutput;
}


void BiquadFilter::initialize(float b0, float b1, float b2, float a0, float a1){
    a = new float[order];
    b = new float[order + 1];
    prevInput = new float[order];
    prevOutput = new float[order];
    for (int i = 0; i < order; i ++){
        prevInput[i] = 0.0f;
        prevOutput[i] = 0.0f;
    }
    a[0] = a0; a[1] = a1;
    b[0] = b0; b[1] = b1; b[2] = b2;
}

void BiquadFilter::update(float b0, float b1, float b2, float a0, float a1){
    a[0] = a0; a[1] = a1;
    b[0] = b0; b[1] = b1; b[2] = b2;
}

float BiquadFilter::process(const float input){
    float output = b[0] * input;
    for (int i = 0; i < order; i++){
        output += (b[i+1] * prevInput[i]) - (a[i] * prevOutput[i]);
    }
    
    //update previous input and output buffer - right shift by 1
    for(int i = order; i > 0; i--){
        prevOutput[i] = prevOutput[i-1];
        prevInput[i] = prevInput[i-1];
    }
    prevInput[0] = input;
    prevOutput[0] = output;
    //std::cout << "Filter INPUT :" << input << ", Filter OUTPUT :" << output << std::endl;
    return output;
}




