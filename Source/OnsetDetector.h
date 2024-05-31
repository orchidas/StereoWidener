/*
  ==============================================================================

    OnsetDetector.h
    Created: 15 May 2024 11:16:46am
    Author:  Orchisama Das

  ==============================================================================
*/

#pragma once
#include "LeakyIntegrator.h"

class OnsetDetector{
public:
    //public variables
    bool onset_flag = false;
    bool offset_flag = false;
    
    //public methods
    OnsetDetector();
    ~OnsetDetector();
    void prepare(int bufferSize, float sampleRate);
    void process(float* input_buffer);
    inline bool check_local_peak();
    inline bool check_direction(bool is_rising);
    inline bool check_onset(float cur_threshold, bool check_offset);
    float* get_signal_envelope(float* input_buffer);

private:
    int buffer_size;
    float sample_rate;
    LeakyIntegrator leaky;
    enum{
        attack_time_ms = 5,
        release_time_ms = 50,
    };
    float  threshold;                 //dynamic threshold for onset calculation
    float running_mean_env;           //running mean of the signal envelope
    unsigned long num_samps = 0;      //keeps track of number of samples in input signal
    float second_last_samp = 0.0;     //last 3 samples of the signal envelope
    float last_samp = 0.0;
    float cur_samp = 0.0;
    float forget_factor = 0.0;        //forget factor for threshold calculation

    };
