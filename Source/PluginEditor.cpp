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
    setSize (300, 300);
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
    g.drawText ("StereoWidener", 150, 250, 180, 50, true);
}

void StereoWidenerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto sliderLeft = 30;
    widthLowerSlider.setBounds (sliderLeft , 50, getWidth() - sliderLeft - 10, 80);
    widthHigherSlider.setBounds (sliderLeft , 150, getWidth() - sliderLeft - 10, 80);

    
}
