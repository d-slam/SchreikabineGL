#pragma once
#include <JuceHeader.h>
#include "AudioState.h"

// solo
class SoloMenu : public juce::Component
{
public:
    SoloMenu()
    {
        btnA.reset(new juce::TextButton("btnA"));
        addAndMakeVisible(btnA.get());
        // btnA->setButtonText("btnA_solo");
        btnA->setBounds(0, 200, 100, 30);
    }
    ~SoloMenu() override
    {
        btnA = nullptr;
    }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::aqua);
        g.setColour(juce::Colours::black);
        g.drawText(juce::String("solo menu"), getLocalBounds(), juce::Justification::centred);
    }
    void resized() override
    {
    }
    void updateLocalisation(Sprache s)
    {
        const auto& texte = Localisation::get(s);
        btnA->setButtonText(texte.solo);
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
        // btnA->setButtonText("btnA_duo");
        btnA->setBounds(100, 200, 100, 30);
    }
    ~DuoMenu() override
    {
        btnA = nullptr;
    }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::blueviolet);
        g.setColour(juce::Colours::black);
        g.drawText(juce::String("duo menu"), getLocalBounds(), juce::Justification::centred);
    }
    void resized() override
    {
    }
    void updateLocalisation(Sprache s)
    {
        const auto& texte = Localisation::get(s);
        btnA->setButtonText(texte.duo);
    }

private:
    std::unique_ptr<juce::TextButton> btnA;
};

// Main Menu
class MenuComponent : public juce::Component
{
public:
    MenuComponent(AudioState& state) : audioState(state)
    {
        soloMenu.reset(new SoloMenu); // menu
        addAndMakeVisible(soloMenu.get());
        duoMenu.reset(new DuoMenu);
        addAndMakeVisible(duoMenu.get());

        soloMenu->setVisible(true);
        duoMenu->setVisible(false);

        btnSolo.reset(new juce::TextButton("btnSolo"));
        addAndMakeVisible(btnSolo.get());
        // btnSolo->setButtonText("solo");
        btnSolo->setBounds(0, 0, 100, 30);
        btnSolo->onClick = [this]()
            {
                soloMenu->setVisible(true);
                duoMenu->setVisible(false);
            };

        btnDuo.reset(new juce::TextButton("btnDue"));
        addAndMakeVisible(btnDuo.get());
        // btnDuo->setButtonText("duo");
        btnDuo->setBounds(100, 0, 100, 30);
        btnDuo->onClick = [this]()
            {
                soloMenu->setVisible(false);
                duoMenu->setVisible(true);
            };

        btnDeutsch.reset(new juce::TextButton("btnDeutsch")); // sprache
        addAndMakeVisible(btnDeutsch.get());
        btnDeutsch->setButtonText("Deutsch");
        btnDeutsch->setBounds(300, 0, 100, 30);
        btnDeutsch->onClick = [this]() { updateLocalisation(Sprache::DEUTSCH); };

        btnEnglisch.reset(new juce::TextButton("btnEnglisch"));
        addAndMakeVisible(btnEnglisch.get());
        btnEnglisch->setButtonText("Englisch");
        btnEnglisch->setBounds(400, 0, 100, 30);
        btnEnglisch->onClick = [this]() { updateLocalisation(Sprache::ENGLISCH); };

        btnTinnitus.reset(new juce::TextButton("btnTinnitus"));
        addAndMakeVisible(btnTinnitus.get());
        btnTinnitus->setButtonText("Tinnitus");
        btnTinnitus->setBounds(0, 100, 100, 30);
        btnTinnitus->setClickingTogglesState(true);
        btnTinnitus->onClick = [this]
            {
                btnTinnitus->setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
            };

        Sprache aktuelleSprache = Sprache::DEUTSCH; // Globale Sprache
        const auto& texte = Localisation::get(aktuelleSprache);

        cbxSelect.reset(new juce::ComboBox("cbxSelect"));
        addAndMakeVisible(cbxSelect.get());
        cbxSelect->setBounds(100, 100, 200, 30);
        cbxSelect->addItem(texte.schwerhoerigkeit_1, 1);
        cbxSelect->addItem(texte.schwerhoerigkeit_2, 2);
        cbxSelect->setSelectedId(audioState.filterType.load() == 0 ? 1 : 2, juce::dontSendNotification); // bullshit ternärer operator...maybe fix
        cbxSelect->onChange = [this]() { audioState.filterType.store(cbxSelect->getSelectedId() == 1 ? 0 : 1); };  // oder greif de scheise net un

        sldFx.reset(new juce::Slider("sldFx"));
        addAndMakeVisible(sldFx.get());
        sldFx->setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
        sldFx->setBounds(300, 100, 400, 30);
        sldFx->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 100, 30);
        sldFx->setRange(30.0f, 20000.0f, 1.0);
        sldFx->setValue(audioState.cutoffHz.load());
        sldFx->setSkewFactorFromMidPoint(5000.0f); // zu niedriger skew -> fader ruckelt
        sldFx->onValueChange = [this]() { audioState.cutoffHz.store(sldFx->getValue()); }; // wir bleibn bei cutoffHz...suscht muasi in localAudioCallback umbaun

        lblFx.reset(new juce::Label("lblFx"));
        addAndMakeVisible(lblFx.get());
        // lblFx->attachToComponent(sldFx.get(),false);
        lblFx->setColour(juce::Label::textColourId, juce::Colours::black);
        lblFx->setText(texte.labelSlider, juce::NotificationType::dontSendNotification);
        lblFx->setBounds(300, 130, 400, 30);

        // schreibe am ende strings
        updateLocalisation(aktuelleSprache);
    }
    ~MenuComponent() override
    {
        btnSolo = nullptr;
        btnDuo = nullptr;
        btnTinnitus = nullptr;
        cbxSelect = nullptr;
        sldFx = nullptr;
        lblFx = nullptr;
        btnDeutsch = nullptr;
        btnEnglisch = nullptr;
        soloMenu = nullptr;
        duoMenu = nullptr;
    }
    void paint(juce::Graphics& g) override
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

    void updateLocalisation(Sprache s)
    {
        const auto& texte = Localisation::get(s);

        btnSolo->setButtonText(texte.solo);
        btnDuo->setButtonText(texte.duo);

        const int selectedId = cbxSelect->getSelectedId();
        cbxSelect->changeItemText(1, texte.schwerhoerigkeit_1);
        cbxSelect->changeItemText(2, texte.schwerhoerigkeit_2);
        cbxSelect->setSelectedId(selectedId, juce::dontSendNotification);

        lblFx->setText(texte.labelSlider, juce::NotificationType::dontSendNotification);

        // call kinder
        soloMenu->updateLocalisation(s);
        duoMenu->updateLocalisation(s);
    }

private:
    AudioState& audioState;

    std::unique_ptr<juce::TextButton> btnSolo;
    std::unique_ptr<juce::TextButton> btnDuo;

    std::unique_ptr<juce::TextButton> btnTinnitus;
    std::unique_ptr<juce::ComboBox> cbxSelect;
    std::unique_ptr<juce::Slider> sldFx;
    std::unique_ptr<juce::Label> lblFx;

    std::unique_ptr<juce::TextButton> btnDeutsch;
    std::unique_ptr<juce::TextButton> btnEnglisch;

    std::unique_ptr<SoloMenu> soloMenu;
    std::unique_ptr<DuoMenu> duoMenu;
};