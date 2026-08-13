#pragma once
#include <JuceHeader.h>

enum class Sprache
{
    DEUTSCH,
    ENGLISCH
};
struct Strings
{
    juce::String solo;
    juce::String duo;
    juce::String schwerhoerigkeit_1;
    juce::String schwerhoerigkeit_2;
    juce::String labelSlider;

};
class Localisation
{
public:
    static const Strings &get(Sprache sprache)
    {
        static const Strings deutsch{

            "Eine Person",
            "Zwei Personen",
            juce::String::fromUTF8("Altersschwerhörigkeit"),
            juce::String::fromUTF8("Hochtonschwerhörigkeit"),
            juce::String::fromUTF8("Schiebe den Regler nach rechts um schlechter zu hören")
            


        };

        static const Strings englisch{

            "One Person",
            "Two Persons",
            "Age-related high-frequency hearing loss",
            "Low-frequency hearing loss",
            "Move the Fader to the right to hear bad"


        };

        return sprache == Sprache::DEUTSCH ? deutsch : englisch;
    }
};