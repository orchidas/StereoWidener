/*
  ==============================================================================

    TransientHandler.cpp
    Created: 15 May 2024 11:17:37am
    Author:  Orchisama Das

  ==============================================================================
*/

#include "TransientHandler.h"

TransientHandler::TransientHandler(){}
TransientHandler::~TransientHandler(){
    delete [] xfade_in_win;
    delete [] xfade_out_win;
    delete [] xfade_buffer;
    delete [] output_buffer;
}


void TransientHandler::prepare_xfade_windows(){
    xfade_in_win = new float[buffer_size];
    xfade_out_win = new float[buffer_size];
    xfade_buffer = new float[buffer_size];
    output_buffer = new float[buffer_size];
    
    for(int i = 0; i < buffer_size; i++){
        //half hann windows
        xfade_in_win[i] = 0.5*(1 - std::cos(PI * (i+1)));
        xfade_out_win[i] = 0.5*(1 - std::cos(PI * i));
        xfade_buffer[i] = 0.0f;
        output_buffer[i] = 0.0f;
    }
}

void TransientHandler::prepare(int bufferSize, float sampleRate){
    buffer_size = bufferSize;
    sample_rate = sampleRate;
    hold_counter = 0;
    inhibit_counter = 0;
    min_frames_hold = ms_to_frames(min_ms_hold);
    min_frames_inhibit = ms_to_frames(min_ms_inhibit);
    std::cout << min_frames_hold << ", " << min_frames_inhibit << std::endl;
    onset.prepare(buffer_size, sample_rate);
    this->prepare_xfade_windows();
}


float* TransientHandler::copy_buffer(float* input, float *output){
    for(int i = 0;i < buffer_size;i++)
        output[i] = input[i];
    return output;
}

void TransientHandler::apply_xfade(float* input1, float* input2){
    //cross-fades between two inputs by applying a fade-in to input1
    //and fade-out to input2.
    for(int i = 0; i < buffer_size; i++)
        xfade_buffer[i] = xfade_in_win[i] * input1[i] + xfade_out_win[i]*input2[i];
}


float* TransientHandler::process(float* input_buffer, float* widener_output_buffer){
    //cross-fade between the input buffer and stereo widener's output buffer
    //when a transient is detected.
    //Also keep tabs on when the onset and offset flags can change with the
    //hold and inhibit counter
    onset.process(input_buffer);
    
    if (0 < hold_counter && hold_counter < min_frames_hold){
        output_buffer = this->copy_buffer(input_buffer, output_buffer);
        hold_counter++;
        //std::cout << "Hold counter: " << hold_counter << std::endl;
    }
    
    else if (0 < inhibit_counter && inhibit_counter < min_frames_inhibit){
        output_buffer = this->copy_buffer(widener_output_buffer, output_buffer);
        inhibit_counter++;
        //std::cout << "Inhibit counter: " << inhibit_counter << std::endl;
    }
    
    else{
        hold_counter = 0;
        inhibit_counter = 0;
        
        if (onset.onset_flag){
            this->apply_xfade(input_buffer, widener_output_buffer);
            output_buffer = this->copy_buffer(xfade_buffer, output_buffer);
            hold_counter++;
            std::cout << "Onset detected" << std::endl;
        }
        else if (onset.offset_flag){
            this->apply_xfade(widener_output_buffer, input_buffer);
            output_buffer = this->copy_buffer(xfade_buffer, output_buffer);
            inhibit_counter++;
            std::cout << "Offset detected" << std::endl;
        }
    }
    return output_buffer;

}
