#pragma once
#include <JuceHeader.h>
#include "Strings.h"
#include "MenuComponent.h"
#include "SettingsComponent.h"

class UIComponent : public juce::Component
{
public:
    UIComponent()
    {
        // setSize(getParentWidth(), getParentHeight());

        // const auto& texte = Localisation::get(Sprache::DEUTSCH);

        // maSlider.reset(new juce::Slider("maSlider"));
        // addAndMakeVisible(maSlider.get());
        // maSlider->setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
        // maSlider->setBounds(0, 0, 100, 500);
        // maSlider->setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, 100, 30);

        // maButton.reset(new juce::TextButton("maButton"));
        // addAndMakeVisible(maButton.get());
        // maButton->setButtonText("maButton");
        // maButton->setBounds(100, 0, 100, 30);

        // maBox.reset(new juce::ComboBox("maBox"));
        // addAndMakeVisible(maBox.get());
        // maBox->setBounds(200, 0, 100, 30);

        menuComponent.reset(new MenuComponent);
        addAndMakeVisible(menuComponent.get());

        settingsComponent.reset(new SettingsComponent);
        addAndMakeVisible(settingsComponent.get());
    }
    ~UIComponent() override
    {
        maSlider = nullptr;
        maButton = nullptr;
        maBox = nullptr;
        menuComponent = nullptr;
    }

    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::darkgreen);
        g.setColour(juce::Colours::black);
        g.drawText(juce::String("test string"), getLocalBounds(), juce::Justification::topRight);
    }

    void resized() override
    {
        menuComponent->setBounds(getLocalBounds());
        settingsComponent->setBounds(getLocalBounds().removeFromBottom(getHeight() / 3));
    }

private:
    std::unique_ptr<juce::Slider> maSlider;

    std::unique_ptr<juce::TextButton> maButton;

    std::unique_ptr<juce::ComboBox> maBox;

    std::unique_ptr<MenuComponent> menuComponent;

    std::unique_ptr<SettingsComponent> settingsComponent;
};
