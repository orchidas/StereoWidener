/*
  ==============================================================================

    DelayLine.h
    Created: 3 Jul 2021 9:27:30pm
    Author:  Orchisama Das

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"

class DelayLine{
public:
    DelayLine();
    ~DelayLine();
    
    //function to set delay line length
    void prepare(const int L, const float sampleRate);

    //read from pointer
    inline float read() const noexcept {
        return delayBuffer[readPtr];
    }
    
    /*velvet noise convolver with tapped delay line.
    position of samples specified by array called taps.
    gains is the array of the multipliers
    len is the length of the array taps */
    float velvetConvolver(int* taps, float* gains, int len);
    
    //write a pointer
    inline void write(const float input) {

        delayBuffer[writePtr] = input;
    }

    //update pointers
    inline void update() {
        --writePtr;

        if (writePtr < 0) // wrap write pointer
            writePtr = maxDelay - 1;

        readPtr = writePtr + length;
        if (readPtr >= maxDelay) // wrap read pointer
            readPtr -= maxDelay;
    }
    
private:
    enum
    {
        maxDelay = 32*8192,
    };
    float delayBuffer[maxDelay];
    int readPtr = 0, writePtr = 0, length;
};
