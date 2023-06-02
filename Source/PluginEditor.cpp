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
    setSize (300,400);
    
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
    cutoffFrequencySlider.setRange (200.0, 8000.0);
    cutoffFrequencySlider.setValue(500.0);
    cutoffFrequencySlider.setSkewFactor(0.5);  //this will ensure more focus on lower frequencies
    cutoffFrequencyAttach.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (valueTreeState, "cutoffFrequency", cutoffFrequencySlider));

    
    addAndMakeVisible(cutoffFrequencyLabel);
    cutoffFrequencyLabel.setText("Filter cutoff frequency", juce::dontSendNotification);
    cutoffFrequencyLabel.setFont(juce::Font("Times New Roman", 15.0f, juce::Font::plain));
    cutoffFrequencyLabel.attachToComponent (&cutoffFrequencySlider, false);
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
    g.drawText ("StereoWidener", 150, 350, 180, 50, true);
}

void StereoWidenerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto sliderLeft = 30;
    widthLowerSlider.setBounds (sliderLeft , 50, getWidth() - sliderLeft - 10, 80);
    widthHigherSlider.setBounds (sliderLeft , 150, getWidth() - sliderLeft - 10, 80);
    cutoffFrequencySlider.setBounds (sliderLeft , 250, getWidth() - sliderLeft - 10, 80);
    
}
