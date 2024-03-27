/*
  ==============================================================================

    VelvetNoise.cpp
    Created: 6 May 2023 3:52:01pm
    Author:  Orchisama Das

  ==============================================================================
*/

#include "VelvetNoise.h"

VelvetNoise::VelvetNoise(){};
VelvetNoise::~VelvetNoise(){
    delete [] impulsePositions;
    delete [] impulseValues;
}

void VelvetNoise::initialize_from_string(juce::String opt_vn_filter){
    //since we don't know the size of these, we make them vectors
    std::vector<int> tempImpulsePositions;
    std::vector<float> tempImpulseValues;
    std::string nextNumber;
    
    //separate all characters in string by space
    juce::StringArray tokens;
    tokens.addTokens (opt_vn_filter, " ");

    for (int i=0; i<tokens.size(); i++)
    {
        if (tokens[i].getFloatValue() != 0.0f){
            tempImpulsePositions.push_back(i);
            tempImpulseValues.push_back(tokens[i].getFloatValue());
        }
    }
    
    seqLength = tempImpulsePositions.size();
    //convert vectors to arrays
    impulsePositions = new int [seqLength];
    impulseValues = new float [seqLength];
    
    for (int k= 0; k < seqLength; k++){
        impulsePositions[k] = tempImpulsePositions.at(k);
        impulseValues[k] = tempImpulseValues.at(k);
    }
    
}

void VelvetNoise::initialize(float SR, float L, int gS, float targetDecaydB, bool logDistribution){
    sampleRate = SR;
    length = (int) (sampleRate * L * 1e-3);
    delayLine.prepare(length, sampleRate);
    decaydB = targetDecaydB;
    gridSize = gS;
    setImpulseLocationValues();
    this->logDistribution = logDistribution;
}

void VelvetNoise::setImpulseLocationValues(){
    float impulseSpacing = sampleRate / gridSize;
    float impulseEnergy = 0.0;

    seqLength = (int)std::floor(length / impulseSpacing);
    impulsePositions = new int[seqLength];
    impulseValues = new float[seqLength];
    
    //create random distributions between 0, 1
    std::default_random_engine generator1, generator2;
    std::uniform_real_distribution<float> distribution(0.0,1.0);
    float runningSum = 0.f;
    float newImpulseSpacing = 0.f;
    
    for (int i = 0; i < seqLength; i++){
        float r1 =  distribution(generator1);
        float r2 = distribution(generator2);
        if (logDistribution){
            newImpulseSpacing = (length / 100) * std::pow(10, 2*gridSize*i/seqLength);
            runningSum += newImpulseSpacing;
            impulsePositions[i] = std::round(r2 * (newImpulseSpacing - 1) + runningSum);
        }
        else{
            impulsePositions[i] = std::round(i * impulseSpacing + r2 * (impulseSpacing - 1));
        }
        int sign = 2 * std::round(r1) - 1;
        impulseValues[i] = sign * std::exp(-convertdBtoDecayRate() * i);
        impulseEnergy += std::pow(impulseValues[i], 2);
    }
    
    //normalise by sequence energy
    for (int i = 0; i < seqLength; i++){
        impulseValues[i] /= std::sqrt(impulseEnergy);
        //std :: cout << "Impulse location " << impulsePositions[i] << std::endl;
        //std :: cout << "Impulse gain " << impulseValues[i] << std::endl;
    }
    
}

void VelvetNoise::update(int newGridSize){
    gridSize = newGridSize;
    setImpulseLocationValues();
}

float VelvetNoise::process(const float input){
    delayLine.update();
    delayLine.write(input);
    float output = delayLine.velvetConvolver(impulsePositions, impulseValues, seqLength);
    //std::cout << "VN output is " << output << std::endl;
    return output;
}

float VelvetNoise::convertdBtoDecayRate(){
    return -std::log(std::pow(10, -decaydB/20))/ seqLength;
}

