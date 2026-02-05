/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessor::SimpleEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SimpleEQAudioProcessor::~SimpleEQAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

/* Edits */
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load(); //returns parameter based on ranged when we defined parameter
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope = static_cast<Slope> (apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());

    return settings;
}

//==============================================================================
void SimpleEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    //process spec obj prepares filter by passing spec ob to chain which passes it to each link in chain 
    juce::dsp::ProcessSpec spec;

    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1; //mono chains can only handle one channel

    spec.sampleRate = sampleRate;
    
    leftChain.prepare(spec);
    rightChain.prepare(spec);

    //we have our settings so we can produce coefficients using static helper functions
    auto chainSettings = getChainSettings(apvts);
    
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 
                                                                                chainSettings.peakFreq, 
                                                                                chainSettings.peakQuality, 
                                                                                chainSettings.peakGainInDecibels);
    //coefficients is allocated on heap so we have to dereference it
    //functions needs an index to particular element in chain
    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;

    //peak filter will make audible changes if gain parameter is not 0 
    //changes to slider wont do anything since we're not updating filter with new coefficients (modify processBlock)

    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
                                                                                                    sampleRate,
                                                                                                    2 * (chainSettings.lowCutSlope + 1));   
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();

    leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);

    switch (chainSettings.lowCutSlope)
    {
    case Slope_12: //order of 2 is 1 coeffcient obj 
    {
        *leftLowCut.get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<0>(false);
        break;
    }
    case Slope_24:
    {
        *leftLowCut.get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *cutCoefficients[1]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<1>(false);
        break;
    }
    case Slope_36:
    {
        *leftLowCut.get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *cutCoefficients[1]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *cutCoefficients[2]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<2>(false);
        break;
    }
    case Slope_48:
    {
        *leftLowCut.get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *cutCoefficients[1]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *cutCoefficients[2]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<2>(false);
        *leftLowCut.get<3>().coefficients = *cutCoefficients[3]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<3>(false);
        break;
    }

    } //switch

    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();

    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);

    switch (chainSettings.lowCutSlope)
    {
    case Slope_12: //order of 2 is 1 coeffcient obj 
    {
        *rightLowCut.get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<0>(false);
        break;
    }
    case Slope_24:
    {
        *rightLowCut.get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *cutCoefficients[1]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<1>(false);
        break;
    }
    case Slope_36:
    {
        *rightLowCut.get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *cutCoefficients[1]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *cutCoefficients[2]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<2>(false);
        break;
    }
    case Slope_48:
    {
        *rightLowCut.get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *cutCoefficients[1]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *cutCoefficients[2]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<2>(false);
        *rightLowCut.get<3>().coefficients = *cutCoefficients[3]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<3>(false);
        break;
    }
    } //switch

}

void SimpleEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

//processor chain requires processing context to be passed into it to run audio through links in chain
//processor context needs audio block instance
//process Block is called by host and is given a buffer which has any number of channels
//we have to extract left and right channels from buffer (channels 0 and 1)

void SimpleEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.

   
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    //we have our settings so we can produce coefficients using static helper functions
    //update filter with new coefficients 
    auto chainSettings = getChainSettings(apvts);

    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
        chainSettings.peakFreq,
        chainSettings.peakQuality,
        chainSettings.peakGainInDecibels);

    //coefficients is allocated on heap so we have to dereference it
    //functions needs an index to particular element in chain
    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;

    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
        getSampleRate(),
        2 * (chainSettings.lowCutSlope + 1));
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();

    leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);

    switch (chainSettings.lowCutSlope)
    {
    case Slope_12: //order of 2 is 1 coeffcient obj 
    {
        *leftLowCut.get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<0>(false);
        break;
    }
    case Slope_24:
    {
        *leftLowCut.get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *cutCoefficients[1]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<1>(false);
        break;
    }
    case Slope_36:
    {
        *leftLowCut.get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *cutCoefficients[1]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *cutCoefficients[2]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<2>(false);
        break;
    }
    case Slope_48:
    {
        *leftLowCut.get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<0>(false);
        *leftLowCut.get<1>().coefficients = *cutCoefficients[1]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<1>(false);
        *leftLowCut.get<2>().coefficients = *cutCoefficients[2]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<2>(false);
        *leftLowCut.get<3>().coefficients = *cutCoefficients[3]; //get first filter and assign to first element cut coeefficients
        leftLowCut.setBypassed<3>(false);
        break;
    }

    } //switch

    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();

    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);

    switch (chainSettings.lowCutSlope)
    {
    case Slope_12: //order of 2 is 1 coeffcient obj 
    {
        *rightLowCut.get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<0>(false);
        break;
    }
    case Slope_24:
    {
        *rightLowCut.get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *cutCoefficients[1]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<1>(false);
        break;
    }
    case Slope_36:
    {
        *rightLowCut.get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *cutCoefficients[1]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *cutCoefficients[2]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<2>(false);
        break;
    }
    case Slope_48:
    {
        *rightLowCut.get<0>().coefficients = *cutCoefficients[0]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<0>(false);
        *rightLowCut.get<1>().coefficients = *cutCoefficients[1]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<1>(false);
        *rightLowCut.get<2>().coefficients = *cutCoefficients[2]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<2>(false);
        *rightLowCut.get<3>().coefficients = *cutCoefficients[3]; //get first filter and assign to first element cut coeefficients
        rightLowCut.setBypassed<3>(false);
        break;
    }
    } //switch


    //create audio block which wraps buffer
    juce::dsp::AudioBlock<float> block(buffer);

    //extract individual channels from buffer which is wrapped in more audio block
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    //create processing context that wrap each individual audio block
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    //context that we pass to mono filter chains
    leftChain.process(leftContext);
    rightChain.process(rightContext);
}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
    //return new SimpleEQAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this); //allows us to view what plugin currently looks like 
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SimpleEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

/************************************EDITS***********************************/
//- 3 equalizer bands (low, high and paraetric/peak)
//-cut bands (low/high): controllable frequency/slope cutoffs
//-parametric band: center frequency, gain, quality (narrow/wide peak is)

//first step: create parameters
//juce has audio processor parameter class 
juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    //set specfic frequency ranges (20Hz-20000Hz), slider changes value in steps of 1Hz, skew is how slider responds to specific range 
    // (more low range covered in slider or higher range is covered), defaultvalue is 20Hz (bottom of hearing range so we wont hear anything until we move it 
    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq", 
        "LowCut Freq", 
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 
        20.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq",
        "HighCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        20000.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq",
        "Peak Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        750.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain",
        "Peak Gain",
        juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
        0.0f));

    //peak band quality control (how narrrow or wide peak band is)
    // narrow (high q value), wide (low q value)
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Quality",
        "Peak Quality",
        juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
        1.f));

    //cut filters (multiple of 6 or 12) db/octaves (we'll do 12,24, 36, 48)
    //audioparameterchoice takes a string array so we have to create one with the values we've chosen
    juce::StringArray stringArray;

    for (int i = 0; i < 4; ++i)
    {
        juce::String str;
        str << (12 + i * 12);
        str << " dB/Oct";
        stringArray.add(str);
    }
    
    //adds 2 copies of string array to lowcut and highcut parameter 
    layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCutSlope", stringArray, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "LowCutSlope", stringArray, 0));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEQAudioProcessor();
}
