/*
  ==============================================================================

    Panner.cpp
    Created: 1 Jun 2023 12:00:14pm
    Author:  Orchisama Das

  ==============================================================================
*/

#include "Panner.h"

Panner::Panner(){}
Panner::~Panner(){
    delete [] output;
}


void Panner::initialize(){
    angle = 0.f;
    width = 0.f;
    output = new float[numChans];
    for (int i =0; i < numChans; i++){
        output[i] = 0.0f;
    }
}

float Panner::process(const float* input){
    output[0] = std::sin(angle) * input[0];
    output[1] = std::cos(angle) * input[1];
    return output[0] + output[1];
}

void Panner::updateWidth(float newWidth){
    width = newWidth;
    angle = (float) juce::jmap (width, 0.f, 1.0f, 0.f, PI/2.0f);
}


