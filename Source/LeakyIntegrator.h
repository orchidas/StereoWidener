/*
  ==============================================================================

    LeakyIntegrator.h
    Created: 15 May 2024 11:17:24am
    Author:  Orchisama Das

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"

class LeakyIntegrator{
public:
    LeakyIntegrator(){};
    ~LeakyIntegrator(){
        delete [] signal_env;
    };
    
    inline float ms_to_samps(float time_ms){
        return (time_ms * 1e-3 * sample_rate);
    }
    
    void prepare(int bufferSize, float sampleRate, float attack_time_ms, float release_time_ms){
        buffer_size = bufferSize;
        sample_rate = sampleRate;
        tau_attack = ms_to_samps(attack_time_ms);
        tau_release = ms_to_samps(release_time_ms);
        signal_env = new float[buffer_size];
        for(int i=0; i < buffer_size; i++){
            signal_env[i] = 0.0f;
        }
    }
    
    //signal envelope calculation with a leaky integrator
    float* process(float* input_buffer){
        float prev_env_samp;
        for (int i = 0; i< buffer_size; i++){
            if (i == 0)
                prev_env_samp = signal_env[buffer_size-1];
            else
                prev_env_samp = signal_env[i-1];
            if (input_buffer[i] > prev_env_samp){
                signal_env[i] = prev_env_samp + (1-std::exp(-1.0/tau_attack)) * (std::abs(input_buffer[i]) - prev_env_samp);
            }
            else{
                signal_env[i] = prev_env_samp + (1-std::exp(-1.0/tau_release)) * (std::abs(input_buffer[i]) - prev_env_samp);
            }
        }
        return signal_env;
    }
    
private:
    float sample_rate;
    int buffer_size;
    float tau_attack;
    float tau_release;
    float* signal_env;
};
