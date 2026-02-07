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

    template<int Index, typename ChainType, typename CoefficientType>
    void update(ChainType& chain, const CoefficientType& coefficients)
    { 
        updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
        chain.template setBypassed<Index> (false);
    }

    template<typename ChainType, typename CoefficientType>
    void updateCutFilter(ChainType& chain, 
                        const CoefficientType& coefficients,
//                      const ChainSettings& chainSettings, 
                        const Slope& slope)
    {
      
        chain.template setBypassed<0>(true);
        chain.template setBypassed<1>(true);
        chain.template setBypassed<2>(true);
        chain.template setBypassed<3>(true);

       
        switch (slope)
        {
            case Slope_48:
            {
                update<3>(chain, coefficients);
                //Does the same thing as this
                //*leftlowcut.template get<3>().coefficients = *cutcoefficients[3]; //get first filter and assign to first element cut coeefficients
                //leftlowcut.template setbypassed<3>(false);
            }
            case Slope_36:
            {
                update<2>(chain, coefficients);
            }
            case Slope_24:
            {
                update<1>(chain, coefficients);
            }
            case Slope_12:
            {
                update<0>(chain, coefficients);
            }
        }   //switch
    }

    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCUtFilters(const ChainSettings& chainSettings);

    void updateFilters();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
