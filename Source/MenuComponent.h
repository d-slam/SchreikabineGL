#pragma once
#include <JuceHeader.h>

// solo
class SoloMenu : public juce::Component
{
public:
    SoloMenu()
    {
        btnA.reset(new juce::TextButton("btnA"));
        addAndMakeVisible(btnA.get());
        btnA->setButtonText("btnA_solo");
        btnA->setBounds(0, 200, 100, 30);
    }
    ~SoloMenu() override
    {
        btnA = nullptr;
    }
    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::aqua);
        g.setColour(juce::Colours::black);
        g.drawText(juce::String("solo menu"), getLocalBounds(), juce::Justification::centred);
    }
    void resized() override
    {
    }

private:
    std::unique_ptr<juce::TextButton> btnA;
};

// duo
class DuoMenu : public juce::Component
{
public:
    DuoMenu()
    {
        btnA.reset(new juce::TextButton("btnA"));
        addAndMakeVisible(btnA.get());
        btnA->setButtonText("btnA_duo");
        btnA->setBounds(500, 200, 100, 30);
    }
    ~DuoMenu() override
    {
        btnA = nullptr;
    }
    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::blueviolet);
        g.setColour(juce::Colours::black);
        g.drawText(juce::String("duo menu"), getLocalBounds(), juce::Justification::centred);
    }
    void resized() override
    {
    }

private:
    std::unique_ptr<juce::TextButton> btnA;
};

// Main Menu
class MenuComponent : public juce::Component
{
public:
    MenuComponent()
    {        
        soloMenu.reset(new SoloMenu);
        addAndMakeVisible(soloMenu.get());
        duoMenu.reset(new DuoMenu);
        addAndMakeVisible(duoMenu.get());

        soloMenu->setVisible(true);
        duoMenu->setVisible(false);

        btnSolo.reset(new juce::TextButton("btnSolo"));
        addAndMakeVisible(btnSolo.get());
        btnSolo->setButtonText("solo");
        btnSolo->setBounds(0, 0, 100, 30);
        btnSolo->onClick = [this]()
        {
            soloMenu->setVisible(true);
            duoMenu->setVisible(false);
        };

        btnDuo.reset(new juce::TextButton("btnDue"));
        addAndMakeVisible(btnDuo.get());
        btnDuo->setButtonText("duo");
        btnDuo->setBounds(100, 0, 100, 30);
        btnDuo->onClick = [this]()
        {
            soloMenu->setVisible(false);
            duoMenu->setVisible(true);
        };
    }
    ~MenuComponent() override
    {
        btnSolo = nullptr;
        btnDuo = nullptr;
        soloMenu = nullptr;
        duoMenu = nullptr;
        
    }
    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::skyblue);
        g.setColour(juce::Colours::black);
        g.drawText(juce::String("main menu"), getLocalBounds(), juce::Justification::centred);
    }
    void resized() override
    {
        soloMenu->setBounds(getLocalBounds());
        duoMenu->setBounds(getLocalBounds());
    }

private:
    // SoloMenu soloMenu;
    // DuoMenu duoMenu;
    std::unique_ptr<SoloMenu> soloMenu;
    std::unique_ptr<DuoMenu> duoMenu;



    std::unique_ptr<juce::TextButton> btnSolo;
    std::unique_ptr<juce::TextButton> btnDuo;
};