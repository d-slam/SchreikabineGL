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
            juce::String::fromUTF8("Hochtonschwerhörigkeit")

        };

        static const Strings englisch{

            "One Person",
            "Two Persons",
            "Age-related high-frequency hearing loss",
            "Low-frequency hearing loss"

        };

        return sprache == Sprache::DEUTSCH ? deutsch : englisch;
    }
};