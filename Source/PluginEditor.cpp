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
    setSize (300, 200);
    //add sliders and labels
    addAndMakeVisible(widthSlider);
    widthSlider.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    widthSlider.setRange (0, 100.0);
    widthSlider.setValue(0.0);
    widthAttach.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (valueTreeState, "width", widthSlider));

    
    addAndMakeVisible(widthLabel);
    widthLabel.setText("Stereo Width", juce::dontSendNotification);
    widthLabel.setFont(juce::Font("Times New Roman", 15.0f, juce::Font::plain));
    widthLabel.attachToComponent (&widthSlider, false);
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
    g.drawText ("StereoWidener", 150, 150, 180, 50, true);
}

void StereoWidenerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto sliderLeft = 30;
    widthSlider.setBounds (sliderLeft , 50, getWidth() - sliderLeft - 10, 80);
}
