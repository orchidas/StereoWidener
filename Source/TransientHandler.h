/*
  ==============================================================================

    TransientHandler.h
    Created: 15 May 2024 11:17:37am
    Author:  Orchisama Das

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include "OnsetDetector.h"

class TransientHandler{
public:
    TransientHandler();
    ~TransientHandler();
    
    inline int ms_to_frames(float time_ms){
        return int(std::ceil(time_ms * 1e-3 * sample_rate / buffer_size));
    }
    void prepare_xfade_windows();
    void prepare(int bufferSize, float sampleRate);
    void apply_xfade(float* input1, float* input2);
    float* copy_buffer(float* input, float* output);
    float* process(float* input_buffer, float* widener_output_buffer);

private:
    const float PI = std::acos(-1);
    int buffer_size;
    float sample_rate;
    //cross-fading parameters when onset is detected
    float* xfade_in_win;
    float* xfade_out_win;
    float* xfade_buffer;
    float* output_buffer;
    //onset detector object
    OnsetDetector onset;
    //
    int hold_counter;       //if an onset is detected, the flag will be true for a
                            //minimum number of frames to prevent false offset detection
    int min_frames_hold;
    int inhibit_counter;    // if an offset is detected, we will wait a
                            // minimum number of frames to prevent false onset detection
    int min_frames_inhibit;
    //calculated with a buffer size of 256 samples
    enum{
        min_ms_hold = 80,
        min_ms_inhibit = 20,
    };
    };
