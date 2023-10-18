/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class StereoWidenerAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    StereoWidenerAudioProcessorEditor (StereoWidenerAudioProcessor&,            juce::AudioProcessorValueTreeState&);
    ~StereoWidenerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    StereoWidenerAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& valueTreeState;
    juce::Label widthLowerLabel;
    juce::Slider widthLowerSlider;
    juce::Label widthHigherLabel;
    juce::Slider widthHigherSlider;
    juce::Slider cutoffFrequencySlider;
    juce::Label cutoffFrequencyLabel;
    juce::ToggleButton isAmpPreserve;
    juce::Label isAmpPreserveLabel;
    juce::ToggleButton hasAllpassDecorrelation;
    juce::Label hasAllpassDecorrelationLabel;
    
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> widthLowerAttach;
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> widthHigherAttach;
    std::unique_ptr <juce::AudioProcessorValueTreeState::SliderAttachment> cutoffFrequencyAttach;
    std::unique_ptr <juce::ParameterAttachment> isAmpPreserveAttach, hasAllpassDecorrelationAttach;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoWidenerAudioProcessorEditor)
};
