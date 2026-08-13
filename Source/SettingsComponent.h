#pragma once
#include <JuceHeader.h>

class SettingsComponent : public juce::Component
{
public:
    SettingsComponent()
    {
        setOpaque(true);
        setVisible(true);

        volSlider1.reset(new juce::Slider("volSlider1"));
        addAndMakeVisible(volSlider1.get());
        volSlider1->setRange(0.0, 1.0, 0.001);
        volSlider1->setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);

        volSlider2.reset(new juce::Slider("volSlider2"));
        addAndMakeVisible(volSlider2.get());
        volSlider2->setRange(0.0, 1.0, 0.001);
        volSlider2->setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);

        volSlider3.reset(new juce::Slider("volSlider3"));
        addAndMakeVisible(volSlider3.get());
        volSlider3->setRange(0.0, 1.0, 0.001);
        volSlider3->setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);

        addAndMakeVisible(loadButton1);
        addAndMakeVisible(playButton1);
        addAndMakeVisible(fileLabel1);
        fileLabel1.setText("No file", juce::dontSendNotification);
        fileLabel1.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(loadButton2);
        addAndMakeVisible(playButton2);
        addAndMakeVisible(fileLabel2);
        fileLabel2.setText("No file", juce::dontSendNotification);
        fileLabel2.setColour(juce::Label::textColourId, juce::Colours::white);

        // sample3 controls
        addAndMakeVisible(loadButton3);
        addAndMakeVisible(playButton3);
        addAndMakeVisible(fileLabel3);
        fileLabel3.setText("No file", juce::dontSendNotification);
        fileLabel3.setColour(juce::Label::textColourId, juce::Colours::white);

        addAndMakeVisible(playBothButton);
        addAndMakeVisible(stopBothButton);
        addAndMakeVisible(monitorButton);

        // cutoffLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        // addAndMakeVisible(cutoffLabel);
    }
    ~SettingsComponent() override
    {
        volSlider1 = nullptr;
        volSlider2 = nullptr;
        volSlider3 = nullptr;
    }

    void paint(juce::Graphics &g) override
    {
        // give UI area a subtle different background so controls are visible against main window
        g.fillAll(juce::Colour::fromRGB(48, 58, 64));
    }

    void resized() override
    {
        volSlider1->setBounds(300, 0, 400, 30);
        volSlider2->setBounds(300, 50, 400, 30);
        volSlider3->setBounds(300, 100, 400, 30);

        loadButton1.setBounds(0, 0, 100, 30);
        playButton1.setBounds(100, 0, 100, 30);
        fileLabel1.setBounds(200, 0, 100, 30);
        playBothButton.setBounds(700, 100, 100, 30);

        loadButton2.setBounds(0, 50, 100, 30);
        playButton2.setBounds(100, 50, 100, 30);
        fileLabel2.setBounds(200, 50, 100, 30);
        stopBothButton.setBounds(700, 150, 100, 30);

        loadButton3.setBounds(0, 100, 100, 30);
        playButton3.setBounds(100, 100, 100, 30);
        fileLabel3.setBounds(200, 100, 100, 30);

        monitorButton.setBounds(0, 150, 100, 30);
    }

    // Expose controls publicly for MainComponent to hook handlers
    // juce::Slider volSlider1;
    std::unique_ptr<juce::Slider> volSlider1;
    std::unique_ptr<juce::Slider> volSlider2;
    std::unique_ptr<juce::Slider> volSlider3;

    // juce::Slider volSlider2;
    // juce::Slider volSlider3;

    juce::TextButton loadButton1{"Load Sample 1"};
    juce::TextButton playButton1{"Play 1"};
    juce::Label fileLabel1;

    juce::TextButton loadButton2{"Load Sample 2"};
    juce::TextButton playButton2{"Play 2"};
    juce::Label fileLabel2;

    juce::TextButton loadButton3{"Load Sample 3"};
    juce::TextButton playButton3{"Play 3"};
    juce::Label fileLabel3;

    juce::TextButton playBothButton{"Play Both"};
    juce::TextButton stopBothButton{"Stop Both"};
    juce::TextButton monitorButton{"Monitor Input"};

    // juce::Label cutoffLabel;
};
