/*
  ==============================================================================

    LinkwitzCrossover.cpp
    Created: 1 Jun 2023 6:18:50pm
    Author:  Orchisama Das

  ==============================================================================
*/

#include "LinkwitzCrossover.h"

LinkwitzCrossover::LinkwitzCrossover(){}
LinkwitzCrossover::~LinkwitzCrossover(){
    delete [] numCoeffs;
    delete [] denCoeffs;
    delete [] prevInput;
    delete [] prevOutput;
}


void LinkwitzCrossover::setCoefficients(){
    /*
    //4th order filter is unstable
    float wc = 2*PI*cutoff;
    float wc2=std::pow(wc, 2);
    float wc3=std::pow(wc, 3);
    float wc4=std::pow(wc, 4);

    float k = wc / std::tan(PI * cutoff/sampleRate);
    float k2=std::pow(k, 2);
    float k3=std::pow(k, 3);
    float k4=std::pow(k, 4);
    float sq_tmp1=std::sqrt(2)*wc3*k;
    float sq_tmp2=std::sqrt(2)*wc*k3;
    float a_tmp = 4 * wc2 * k2 + 2 * sq_tmp1 + k4 + 2 * sq_tmp2 + wc4;

    denCoeffs[0]=(4*(wc4+sq_tmp1-k4-sq_tmp2))/a_tmp;
    denCoeffs[1]=(6*wc4-8*wc2*k2+6*k4)/a_tmp;
    denCoeffs[2]=(4*(wc4-sq_tmp1+sq_tmp2-k4))/a_tmp;
    denCoeffs[3]=(k4-2*sq_tmp1+wc4-2*sq_tmp2+4*wc2*k2)/a_tmp;

    //================================================
    // low-pass
    //================================================
    if (lowpass){
        numCoeffs[0] =wc4/a_tmp;
        numCoeffs[1] =4*wc4/a_tmp;
        numCoeffs[2] =6*wc4/a_tmp;
        numCoeffs[3] =numCoeffs[1];
        numCoeffs[4] =numCoeffs[0];
    }
    //=====================================================
    // high-pass
    //=====================================================
    else{
        numCoeffs[0]=k4/a_tmp;
        numCoeffs[1]=-4*k4/a_tmp;
        numCoeffs[2]=6*k4/a_tmp;
        numCoeffs[3]=numCoeffs[1];
        numCoeffs[4]=numCoeffs[0];
    }
    */
    
    //2nd order is stable
    float fpi = PI*cutoff;
    float wc = 2*fpi;
    float wc2 = wc*wc;
    float wc22 = 2*wc2;
    float k = wc/std::tan(fpi/sampleRate);
    float k2 = k*k;
    float k22 = 2*k2;
    float wck2 = 2*wc*k;
    float tmpk = (k2+wc2+wck2);
    
    
    denCoeffs[0] = (-k22+wc22)/tmpk;
    denCoeffs[1] = (-wck2+k2+wc2)/tmpk;
    //---------------
    // low-pass
    //---------------
    if (lowpass){
    numCoeffs[0] = (wc2)/tmpk;
    numCoeffs[1] = (wc22)/tmpk;
    numCoeffs[2] = (wc2)/tmpk;
    }
    else{
    //----------------
    // high-pass
    //----------------
    numCoeffs[0] = (k2)/tmpk;
    numCoeffs[1] = (-k22)/tmpk;
    numCoeffs[2] = (k2)/tmpk;
    }

}

void LinkwitzCrossover::initialize(float sR, std::string type){
    sampleRate = sR;
    numCoeffs = new float[order + 1];
    denCoeffs = new float[order];
    prevInput = new float[order];
    prevOutput = new float[order];
    for (int i = 0; i < order; i++){
        numCoeffs[i] = 0.0;
        denCoeffs[i] = 0.0;
        prevInput[i] = 0.0;
        prevOutput[i] = 0.0;
    }
    numCoeffs[order] = 0.0;
    lowpass = (type == "lowpass")? true : false;
    setCoefficients();
}


void LinkwitzCrossover::update(float newCutoffFrequency){
    cutoff = newCutoffFrequency;
    setCoefficients();
}

float LinkwitzCrossover::process(const float input){
    float output = numCoeffs[0] * input;
    for (int i = 0; i < order; i++){
        output += (numCoeffs[i+1] * prevInput[i]) - (denCoeffs[i] * prevOutput[i]);
    }
    
    //update previous input and output buffer - right shift by 1
    for(int i = order; i > 0; i--){
        prevOutput[i] = prevOutput[i-1];
        prevInput[i] = prevInput[i-1];
    }
    prevInput[0] = input;
    prevOutput[0] = output;
    
    //there is a 180 degree phase shift between lowpass and highpass
    if (lowpass)
        return output;
    else
        return -output;
}
