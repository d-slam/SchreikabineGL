#pragma once

#include <JuceHeader.h>
#include <memory>
#include "UIComponent.h"
#include "AudioState.h" 
// forward-declare the audio callback type defined in the cpp file
struct LocalAudioCallback;

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    // Sampler + format manager
    juce::AudioFormatManager formatManager;
    juce::Synthesiser synth1; // contains sample 1
    juce::Synthesiser synth2; // contains sample 2 (bypassed by filter)
    juce::Synthesiser synth3; // contains sample 3 (bypassed by filter)

    // UI is delegated to UIComponent (defined in UIComponent.h)
    std::unique_ptr<UIComponent> uiComp;

    std::unique_ptr<AudioState> audioState;
    

    // custom audio callback for zero-latency input monitoring + synth mixing
    std::unique_ptr<LocalAudioCallback> customAudioCallback;
    bool useCustomAudioCallback = false;
    // Low-pass for monitor + sample1 (moved to AudioState)

    // helper
    void addSampleFromFile (const juce::File& f, int rootNote);
    void autoLoadSamplesFromFolder (const juce::File& folder);

    int sampleNote3 = 62;

    int sampleNote1 = 60;
    int sampleNote2 = 61;




    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
