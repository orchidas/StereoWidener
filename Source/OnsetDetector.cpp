/*
  ==============================================================================

    OnsetDetector.cpp
    Created: 15 May 2024 11:16:46am
    Author:  Orchisama Das

  ==============================================================================
*/

#include "OnsetDetector.h"

OnsetDetector::OnsetDetector(){}
OnsetDetector::~OnsetDetector(){
    delete [] threshold;
}


void OnsetDetector::prepare(int bufferSize, float sampleRate){
    buffer_size = bufferSize;
    sample_rate = sampleRate;
    //initialise leaky integrator
    leaky.prepare(bufferSize, sampleRate, attack_time_ms, release_time_ms);
    threshold = new float[buffer_size];
    for (int i = 0; i < bufferSize; i++){
        threshold[i] = 1e9;
    }
    //keeps track of the mean of the signal envelope
    running_mean_env = 0.0;
}

bool OnsetDetector::check_local_peak(){
    if ((last_samp > second_last_samp) && (last_samp > cur_samp))
        return true;
    else
        return false;
}

bool OnsetDetector::check_direction(bool is_rising){
    //checks direction of signal. If is_rising is true, returns
    //true if direction is rising. If is_rising is false, returns
    //true if direction is falling.
    
    if (is_rising)
        return ((last_samp > second_last_samp) && (last_samp < cur_samp));
    else
        return ((last_samp < second_last_samp) && (last_samp > cur_samp));
}

float* OnsetDetector::get_signal_envelope(float* input_buffer){
    return leaky.process(input_buffer);
}


bool OnsetDetector::check_onset(float cur_threshold, bool check_offset){
    //checks if there is an onset or offset based on the current value of threshold
    //checks for offset if check_offset is true, else checks for onset
    if (!check_offset){
        return (check_direction(true) && (last_samp > cur_threshold));
    }
    else
        return (check_direction(false) && (last_samp < cur_threshold));
}

void OnsetDetector::process(float* input_buffer){
    float* signal_env = this->get_signal_envelope(input_buffer);
    for (int i = 0; i < buffer_size; i++){
        //update the values of the last 3 samples
        if (i == 0){
            second_last_samp = last_samp;
            last_samp = cur_samp;
            cur_samp = signal_env[i];
        }
        else if (i == 1){
            second_last_samp = last_samp;
            last_samp = signal_env[i-1];
            cur_samp = signal_env[i];
        }
        else{
            second_last_samp = signal_env[i-2];
            last_samp = signal_env[i-1];
            cur_samp = signal_env[i];
        }
        
        //calculate running mean of the signal envelope
        float scaling = 1.0/(++num_samps);
        running_mean_env = signal_env[i] * scaling + (1-scaling) * running_mean_env;
        
        // if a local peak is detected, update threshold to 2xrunning_mean, else
        // keep the last value
        if (this->check_local_peak() || i == 0)
            threshold[i] = 2 * running_mean_env;
        else
            threshold[i] = threshold[i-1];
        
        //if onset or offset has already been detected, continue
        if (onset_flag || offset_flag)
            continue;
        //these flags are set only once per buffer, but the
        //rest of the calculations are carried out sample by sample
        else{
            onset_flag = this->check_onset(threshold[i], false);
            offset_flag = this->check_onset(threshold[i], true);
        }
    }
}
