/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

//create a data structure to extract parameters from audio processor value tree state
struct ChainSettings
{
    float peakFreq{ 0 }, peakGainInDecibels{ 0 }, peakQuality{ 1.f };
    float lowCutFreq{ 0 }, highCutFreq{ 0 };
    Slope lowCutSlope{Slope::Slope_12}, highCutSlope{ Slope::Slope_12};
};


ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

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

    //enums for get function in the chain
    enum ChainPositions
    {
        LowCut,
        Peak,
        HighCut
    };
    
    //refactoring the DSP
    void updatePeakFilter(const ChainSettings& chainSettings);
    using Coefficients = Filter::CoefficientsPtr;
    static void updateCoefficients(Coefficients& old, const Coefficients& replacements);

    template<typename ChainType, typename CoefficientType>
    void updateCutFilter(ChainType& leftLowCut, 
                        const CoefficientType& cutCoefficients,
//                      const ChainSettings& chainSettings, 
                        const Slope& lowCutSlope)
    {
      
        leftLowCut.template setBypassed<0>(true);
        leftLowCut.template setBypassed<1>(true);
        leftLowCut.template setBypassed<2>(true);
        leftLowCut.template setBypassed<3>(true);

        //switch (chainSettings.lowCutSlope)
        switch (lowCutSlope)
        {
        case Slope_12: //order of 2 is 1 coeffcient obj 
        {
            *leftLowCut.template get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
            leftLowCut.template setBypassed<0>(false);
            break;
        }
        case Slope_24:
        {
            *leftLowCut.template get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
            leftLowCut.template setBypassed<0>(false);
            *leftLowCut.template get<1>().coefficients = *cutCoefficients[1]; //get first filter and assign to first element cut coeefficients
            leftLowCut.template setBypassed<1>(false);
            break;
        }
        case Slope_36:
        {
            *leftLowCut.template get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
            leftLowCut.template setBypassed<0>(false);
            *leftLowCut.template get<1>().coefficients = *cutCoefficients[1]; //get first filter and assign to first element cut coeefficients
            leftLowCut.template setBypassed<1>(false);
            *leftLowCut.template get<2>().coefficients = *cutCoefficients[2]; //get first filter and assign to first element cut coeefficients
            leftLowCut.template setBypassed<2>(false);
            break;
        }
        case Slope_48:
        {
            *leftLowCut.template get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
            leftLowCut.template setBypassed<0>(false);
            *leftLowCut.template get<1>().coefficients = *cutCoefficients[1]; //get first filter and assign to first element cut coeefficients
            leftLowCut.template setBypassed<1>(false);
            *leftLowCut.template get<2>().coefficients = *cutCoefficients[2]; //get first filter and assign to first element cut coeefficients
            leftLowCut.template setBypassed<2>(false);
            *leftLowCut.template get<3>().coefficients = *cutCoefficients[3]; //get first filter and assign to first element cut coeefficients
            leftLowCut.template setBypassed<3>(false);
            break;
        }

        } //switch
    }
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
