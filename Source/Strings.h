#pragma once
#include <JuceHeader.h>

enum class Sprache
{
    DEUTSCH,
    ENGLISCH
};
struct Strings
{
    juce::String sprache;
    juce::String solo;
    juce::String duo;
};
class Localisation
{
public:
    static const Strings &get(Sprache sprache)
    {
        static const Strings deutsch{
            "Deutsch",
            "Eine Person",
            "Zwei Personen"

        };

        static const Strings englisch{
            "Englisch",
            "One Person",
            "Two Persons"

        };

        return sprache == Sprache::DEUTSCH ? deutsch : englisch;
    }
};