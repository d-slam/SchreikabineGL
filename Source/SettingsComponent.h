#pragma once
#include <JuceHeader.h>

class SettingsComponent : public juce::Component
{
public:
    SettingsComponent()
    {
        setOpaque (true);
        setVisible (true);

        addAndMakeVisible (loadButton1);
        addAndMakeVisible (playButton1);
        addAndMakeVisible (fileLabel1);
        fileLabel1.setText ("No file", juce::dontSendNotification);
        fileLabel1.setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (volSlider1);
        volSlider1.setRange (0.0, 1.0, 0.001);
        volSlider1.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);

        addAndMakeVisible (loadButton2);
        addAndMakeVisible (playButton2);
        addAndMakeVisible (fileLabel2);
        fileLabel2.setText ("No file", juce::dontSendNotification);
        fileLabel2.setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (volSlider2);
        volSlider2.setRange (0.0, 1.0, 0.001);
        volSlider2.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);
        // sample3 controls
        addAndMakeVisible (loadButton3);
        addAndMakeVisible (playButton3);
        addAndMakeVisible (fileLabel3);
        fileLabel3.setText ("No file", juce::dontSendNotification);
        fileLabel3.setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (volSlider3);
        volSlider3.setRange (0.0, 1.0, 0.001);
        volSlider3.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);

        addAndMakeVisible (playBothButton);
        addAndMakeVisible (stopBothButton);
        addAndMakeVisible (monitorButton);


        cutoffLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (cutoffLabel);
    }
    ~SettingsComponent() override
    {

    }

    void paint (juce::Graphics& g) override
    {
        // give UI area a subtle different background so controls are visible against main window
        g.fillAll (juce::Colour::fromRGB (48, 58, 64));
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (8);
        auto top = r.removeFromTop (28);

        loadButton1.setBounds (top.removeFromLeft (150).reduced (4));
        playButton1.setBounds (top.removeFromLeft (100).reduced (4));
        fileLabel1.setBounds (top.removeFromLeft (220).reduced (4));
        volSlider1.setBounds (top.removeFromLeft (150).reduced (4));
        playBothButton.setBounds (top.removeFromLeft (120).reduced (4));
        stopBothButton.setBounds (top.removeFromLeft (120).reduced (4));

        r = getLocalBounds().reduced (8);
        r.removeFromTop (40);
        auto next = r.removeFromTop (28);
        loadButton2.setBounds (next.removeFromLeft (120).reduced (4));
        playButton2.setBounds (next.removeFromLeft (80).reduced (4));
        fileLabel2.setBounds (next.removeFromLeft (200).reduced (4));
        volSlider2.setBounds (next.removeFromLeft (120).reduced (4));

        // place sample3 controls on the next row (below sample2)
        r.removeFromTop (8);
        auto row3 = r.removeFromTop (28);
        loadButton3.setBounds (row3.removeFromLeft (120).reduced (4));
        playButton3.setBounds (row3.removeFromLeft (80).reduced (4));
        fileLabel3.setBounds (row3.removeFromLeft (200).reduced (4));
        volSlider3.setBounds (row3.removeFromLeft (120).reduced (4));

        auto leftX = getLocalBounds().reduced (8).getX();
        // place monitor and filter controls below the last row (row3)
        monitorButton.setBounds (leftX + 4, row3.getBottom() + 12, 140, 24);

        cutoffLabel.setBounds (leftX + 152 + 184 + 124, row3.getBottom() + 12, 80, 24);
    }

    // Expose controls publicly for MainComponent to hook handlers
    juce::TextButton loadButton1 { "Load Sample 1" };
    juce::TextButton playButton1 { "Play 1" };
    juce::Label fileLabel1;
    juce::Slider volSlider1;

    juce::TextButton loadButton2 { "Load Sample 2" };
    juce::TextButton playButton2 { "Play 2" };
    juce::Label fileLabel2;
    juce::Slider volSlider2;

    juce::TextButton loadButton3 { "Load Sample 3" };
    juce::TextButton playButton3 { "Play 3" };
    juce::Label fileLabel3;
    juce::Slider volSlider3;

    juce::TextButton playBothButton { "Play Both" };
    juce::TextButton stopBothButton { "Stop Both" };
    juce::TextButton monitorButton { "Monitor Input" };


    juce::Label cutoffLabel;

};
