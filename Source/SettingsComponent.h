#pragma once
#include <JuceHeader.h>
#include "AudioState.h"

class SettingsComponent : public juce::Component
{
public:
    SettingsComponent(AudioState &state) : audioState(state)
    {
        setOpaque(true);
        setVisible(true);

        volSlider1.reset(new juce::Slider("volSlider1"));
        addAndMakeVisible(volSlider1.get());
        volSlider1->setRange(0.0, 1.0, 0.001);
        volSlider1->setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        volSlider1->setValue(audioState.vol1.load(), juce::dontSendNotification);
        volSlider1->onValueChange = [this]()
        { audioState.vol1.store((float)volSlider1->getValue()); };

        volSlider2.reset(new juce::Slider("volSlider2"));
        addAndMakeVisible(volSlider2.get());
        volSlider2->setRange(0.0, 1.0, 0.001);
        volSlider2->setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        volSlider2->setValue(audioState.vol2.load(), juce::dontSendNotification);
        volSlider2->onValueChange = [this]()
        { audioState.vol2.store((float)volSlider2->getValue()); };

        volSlider3.reset(new juce::Slider("volSlider3"));
        addAndMakeVisible(volSlider3.get());
        volSlider3->setRange(0.0, 1.0, 0.001);
        volSlider3->setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        volSlider3->setValue(audioState.vol3.load(), juce::dontSendNotification);
        volSlider3->onValueChange = [this]()
        { audioState.vol1.store((float)volSlider3->getValue()); };

        fileLabel1.reset(new juce::Label("fileLabel1"));
        addAndMakeVisible(fileLabel1.get());
        fileLabel1->setText("No file", juce::dontSendNotification);
        fileLabel1->setColour(juce::Label::textColourId, juce::Colours::white);

        fileLabel2.reset(new juce::Label("fileLabel2"));
        addAndMakeVisible(fileLabel2.get());
        fileLabel2->setText("No file", juce::dontSendNotification);
        fileLabel2->setColour(juce::Label::textColourId, juce::Colours::white);

        fileLabel3.reset(new juce::Label("volSlider3"));
        addAndMakeVisible(fileLabel3.get());
        fileLabel3->setText("No file", juce::dontSendNotification);
        fileLabel3->setColour(juce::Label::textColourId, juce::Colours::white);

        loadButton1.reset(new juce::TextButton("Load..."));
        addAndMakeVisible(loadButton1.get());

        loadButton2.reset(new juce::TextButton("Load..."));
        addAndMakeVisible(loadButton2.get());

        loadButton3.reset(new juce::TextButton("Load..."));
        addAndMakeVisible(loadButton3.get());

        playButton1.reset(new juce::TextButton("Play Voice"));
        addAndMakeVisible(playButton1.get());

        playButton2.reset(new juce::TextButton("Play Tinnitus"));
        addAndMakeVisible(playButton2.get());

        playButton3.reset(new juce::TextButton("Play Voice"));
        addAndMakeVisible(playButton3.get());

        playBothButton.reset(new juce::TextButton("Play All"));
        addAndMakeVisible(playBothButton.get());

        stopBothButton.reset(new juce::TextButton("Stop All"));
        addAndMakeVisible(stopBothButton.get());

        monitorButton.reset(new juce::TextButton("Monitor"));
        addAndMakeVisible(monitorButton.get());
        monitorButton->onClick = [this]()
        {
            audioState.monitorEnabled.store(!audioState.monitorEnabled.load());
            monitorButton->setButtonText(audioState.monitorEnabled.load() ? "Monitor: ON" : "Monitor: OFF");
        };
    }
    ~SettingsComponent() override
    {
        volSlider1 = nullptr;
        volSlider2 = nullptr;
        volSlider3 = nullptr;
        fileLabel1 = nullptr;
        fileLabel2 = nullptr;
        fileLabel3 = nullptr;

        loadButton1 = nullptr;
        loadButton2 = nullptr;
        loadButton3 = nullptr;

        playButton1 = nullptr;
        playButton2 = nullptr;
        playButton3 = nullptr;

        playBothButton = nullptr;
        stopBothButton = nullptr;
        monitorButton = nullptr;
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

        fileLabel1->setBounds(200, 0, 100, 30);
        fileLabel2->setBounds(200, 50, 100, 30);
        fileLabel3->setBounds(200, 100, 100, 30);

        loadButton1->setBounds(0, 0, 100, 30);
        loadButton2->setBounds(0, 50, 100, 30);
        loadButton3->setBounds(0, 100, 100, 30);

        playButton1->setBounds(100, 0, 100, 30);
        playButton2->setBounds(100, 50, 100, 30);
        playButton3->setBounds(100, 100, 100, 30);

        playBothButton->setBounds(700, 100, 100, 30);
        stopBothButton->setBounds(700, 150, 100, 30);
        monitorButton->setBounds(0, 150, 100, 30);
    }

    // Expose controls publicly for MainComponent to hook handlers

    std::unique_ptr<juce::Slider> volSlider1;
    std::unique_ptr<juce::Slider> volSlider2;
    std::unique_ptr<juce::Slider> volSlider3;

    std::unique_ptr<juce::Label> fileLabel1;
    std::unique_ptr<juce::Label> fileLabel2;
    std::unique_ptr<juce::Label> fileLabel3;

    std::unique_ptr<juce::TextButton> loadButton1;
    std::unique_ptr<juce::TextButton> loadButton2;
    std::unique_ptr<juce::TextButton> loadButton3;

    std::unique_ptr<juce::TextButton> playButton1;
    std::unique_ptr<juce::TextButton> playButton2;
    std::unique_ptr<juce::TextButton> playButton3;

    std::unique_ptr<juce::TextButton> playBothButton;
    std::unique_ptr<juce::TextButton> stopBothButton;
    std::unique_ptr<juce::TextButton> monitorButton;

private:
    AudioState &audioState;
};
