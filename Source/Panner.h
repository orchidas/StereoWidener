/*
  ==============================================================================

    Panner.h
    Created: 1 Jun 2023 12:00:14pm
    Author:  Orchisama Das

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
class Panner{
public:
    Panner();
    ~Panner();
    
    void initialize();
    float* process(const float* input);
    void update(float newWidth);
    
    
private:
    const float PI = std::acos(-1);     //PI
    const int numChans = 2;             //number of channels panned
    float angle;                        //a value between 0 and pi/2 rad that determines                                   left and right gain weightings
    float width;                        //determines stereo width (0 - original width, 1                                    - max width)
    float* output;                      //2 channel output
};

