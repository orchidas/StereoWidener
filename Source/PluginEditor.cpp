/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
StereoWidenerAudioProcessorEditor::StereoWidenerAudioProcessorEditor (StereoWidenerAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), audioProcessor (p), valueTreeState (vts)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (300,450);
    
    //add sliders and labels
    addAndMakeVisible(widthLowerSlider);
    widthLowerSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    widthLowerSlider.setRange (0, 100.0);
    widthLowerSlider.setValue(0.0);
    widthLowerAttach.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (valueTreeState, "widthLower", widthLowerSlider));

    
    addAndMakeVisible(widthLowerLabel);
    widthLowerLabel.setText("Lower frequency width", juce::dontSendNotification);
    widthLowerLabel.setFont(juce::Font("Times New Roman", 15.0f, juce::Font::plain));
    widthLowerLabel.attachToComponent (&widthLowerSlider, false);
    
    
    //add sliders and labels
    addAndMakeVisible(widthHigherSlider);
    widthHigherSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    widthHigherSlider.setRange (0, 100.0);
    widthHigherSlider.setValue(0.0);
    widthHigherAttach.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (valueTreeState, "widthHigher", widthHigherSlider));

    
    addAndMakeVisible(widthHigherLabel);
    widthHigherLabel.setText("Higher frequency width", juce::dontSendNotification);
    widthHigherLabel.setFont(juce::Font("Times New Roman", 15.0f, juce::Font::plain));
    widthHigherLabel.attachToComponent (&widthHigherSlider, false);
    
    //add sliders and labels
    addAndMakeVisible(cutoffFrequencySlider);
    cutoffFrequencySlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    cutoffFrequencySlider.setRange (100.0, 4000.0);
    cutoffFrequencySlider.setValue(500.0);
    cutoffFrequencySlider.setSkewFactor(0.5);  //this will ensure more focus on lower frequencies
    cutoffFrequencyAttach.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (valueTreeState, "cutoffFrequency", cutoffFrequencySlider));
    
    addAndMakeVisible(cutoffFrequencyLabel);
    cutoffFrequencyLabel.setText("Filter cutoff frequency", juce::dontSendNotification);
    cutoffFrequencyLabel.setFont(juce::Font("Times New Roman", 15.0f, juce::Font::plain));
    cutoffFrequencyLabel.attachToComponent (&cutoffFrequencySlider, false);
    
    //add toggle button to switch between amplitude and energetic calculations
    addAndMakeVisible(isAmpPreserve);
    // [=] indicates a lambda function, it sets the parameterChangedCallback below
    isAmpPreserveAttach = std::make_unique<juce::ParameterAttachment>(*vts.getParameter("isAmpPreserve"), [=] (float value) {
            bool isSelected = value == 1.0f;
            isAmpPreserve.setToggleState(isSelected, juce::sendNotificationSync);
        });
    
    isAmpPreserve.onClick = [=] {
        //if toggle state is true, then
        if (isAmpPreserve.getToggleState())
            isAmpPreserveAttach->setValueAsCompleteGesture(1.0f);
        else
            isAmpPreserveAttach->setValueAsCompleteGesture(0.0f);
    };
    isAmpPreserveAttach->sendInitialUpdate();
    
    //add labels
    addAndMakeVisible(isAmpPreserveLabel);
    isAmpPreserveLabel.setText ("Amplitude preserve", juce::dontSendNotification);
    isAmpPreserveLabel.setFont(juce::Font ("Times New Roman", 15.0f, juce::Font::plain));
    
    // add toggle button for choosing decorrelator
    addAndMakeVisible(hasAllpassDecorrelation);
    // [=] indicates a lambda function, it sets the parameterChangedCallback below
    hasAllpassDecorrelationAttach = std::make_unique<juce::ParameterAttachment>(*vts.getParameter("hasAllpassDecorrelation"), [=] (float value) {
            bool isSelected = value == 1.0f;
            hasAllpassDecorrelation.setToggleState(isSelected, juce::sendNotificationSync);
        });
    
    hasAllpassDecorrelation.onClick = [=] {
        //if toggle state is true, then
        if (hasAllpassDecorrelation.getToggleState())
            hasAllpassDecorrelationAttach->setValueAsCompleteGesture(1.0f);
        else
            hasAllpassDecorrelationAttach->setValueAsCompleteGesture(0.0f);
    };
    hasAllpassDecorrelationAttach->sendInitialUpdate();
    
    //add labels
    addAndMakeVisible(hasAllpassDecorrelationLabel);
    hasAllpassDecorrelationLabel.setText ("Allpass decorrelation", juce::dontSendNotification);
    hasAllpassDecorrelationLabel.setFont(juce::Font ("Times New Roman", 15.0f, juce::Font::plain));
}
    
StereoWidenerAudioProcessorEditor::~StereoWidenerAudioProcessorEditor()
{
}

//==============================================================================
void StereoWidenerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setFont (juce::Font ("Times New Roman", 20.0f, juce::Font::bold));
    g.setColour (juce::Colours::lightgrey);
    g.drawText ("StereoWidener", 150, 400, 180, 50, true);
}

void StereoWidenerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto sliderLeft = 30;
    widthLowerSlider.setBounds (sliderLeft , 50, getWidth() - sliderLeft - 10, 80);
    widthHigherSlider.setBounds (sliderLeft , 150, getWidth() - sliderLeft - 10, 80);
    cutoffFrequencySlider.setBounds (sliderLeft , 250, getWidth() - sliderLeft - 10, 80);
    isAmpPreserve.setBounds (sliderLeft, 320, getWidth() - sliderLeft - 10, 50);
    isAmpPreserveLabel.setBounds(sliderLeft + 50, 340, getWidth() - sliderLeft - 10, 20);
    hasAllpassDecorrelation.setBounds (sliderLeft, 360, getWidth() - sliderLeft - 10, 50);
    hasAllpassDecorrelationLabel.setBounds(sliderLeft + 50, 380, getWidth() - sliderLeft - 10, 20);
}
