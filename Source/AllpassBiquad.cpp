/*
  ==============================================================================

    AllpassBiquad.cpp
    Created: 2 Jun 2023 8:59:31pm
    Author:  Orchisama Das

  ==============================================================================
*/

#include "AllpassBiquad.h"
AllpassBiquad::AllpassBiquad(){}
AllpassBiquad::~AllpassBiquad(){
    delete [] a;
    delete [] b;
    delete [] prevInput;
    delete [] prevOutput;
}


void AllpassBiquad::initialize(float pole_radii, float pole_angle){
    I.real(0); I.imag(1);                   // complex number 0 + 1i
    a = new float[order];
    b = new float[order + 1];
    prevInput = new float[order];
    prevOutput = new float[order];
    for (int i = 0; i < order; i ++){
        prevInput[i] = 0.0f;
        prevOutput[i] = 0.0f;
    }
    std::complex<float> pole = pole_radii * std::exp(2 * PI * I * pole_angle);
    a[0] = -2 * std::real(pole);
    a[1] = std::pow(std::abs(pole),2);
    b[0] = a[1];
    b[1] = a[0];
    b[2] = 1;
}

float AllpassBiquad::process(const float input){
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
    //std::cout << "Allpass INPUT :" << input << ", Allpass OUTPUT :" << output << std::endl;
    return output;
}
