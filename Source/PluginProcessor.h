/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class SimpleEQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /**********************************     Changes to Code     ******************************************/
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(); //function to declare all parameters of apvts layout

    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout()};

private:

    /* DSP namespace uses a lot of templates and nested namespaces*/
    // we can create type aliases to elimnate namespace and template definitions
    
    //we can put 4 filters in prcoessor chain to pass a single context and process all audio automatically
    //each filter in IIR filter has response of 12 dB/oct for low/high pass filter
    using Filter = juce::dsp::IIR::Filter<float>; //peak filter

    //define a chain pass a single context and process audio automatically
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

    //use filter for mono signal path
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

    //stereo processing
    MonoChain leftChain, rightChain;

    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
