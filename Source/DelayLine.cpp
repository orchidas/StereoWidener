/*
  ==============================================================================

    DelayLine.cpp
    Created: 3 Jul 2021 9:27:30pm
    Author:  Orchisama Das

  ==============================================================================
*/

#include "DelayLine.h"


DelayLine::DelayLine(){}
DelayLine::~DelayLine(){}

float DelayLine::velvetConvolver(int* taps, float* gains, int len){
    float output = 0.0f;
    for (int i = 0; i < len; i++){
        int indexAt = readPtr + taps[i];
        if (indexAt >= maxDelay) 
            indexAt -= maxDelay;
        output += gains[i] * delayBuffer[indexAt];
    }
    return output;
}

void DelayLine::prepare(const int L, const float sampleRate){
    
    length = L;  //length of delay line in samples
    //initialize delay lines to prevent garbage memory values
    for (int i = 0; i < maxDelay; i++)
        delayBuffer[i] = 0.0f;
}


