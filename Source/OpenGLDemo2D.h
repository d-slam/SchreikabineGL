/*
  ==============================================================================

    OpenGLDemo2D.h
    Created: 18 Aug 2026 6:57:08pm
    Author:  Dami

  ==============================================================================
*/


#pragma once
#include <JuceHeader.h>

//==============================================================================
class OpenGLDemo2D final : public juce::Component
{
public:
    OpenGLDemo2D()
    {
        setOpaque(true);
        openGLContext.attachTo(*this);

        statusLabel.setJustificationType(juce::Justification::topLeft);
        statusLabel.setFont(juce::FontOptions(14.0f));
        addAndMakeVisible(statusLabel);

        fragmentCode =
            "void main()\n"
            "{\n"
            "    gl_FragColor = vec4 (0.12, 0.12, 0.14, pixelAlpha);\n"
            "}\n";

        setSize(500, 500);
    }

    ~OpenGLDemo2D() override
    {
        openGLContext.detach();
        shader.reset();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);

        if (shader.get() == nullptr || shader->getFragmentShaderCode() != fragmentCode)
        {
            shader.reset();

            if (fragmentCode.isNotEmpty())
            {
                shader.reset(new juce::OpenGLGraphicsContextCustomShader(fragmentCode));

                auto result = shader->checkCompilation(g.getInternalContext());

                if (result.failed())
                {
                    statusLabel.setText(result.getErrorMessage(), juce::dontSendNotification);
                    shader.reset();
                }
            }
        }

        if (shader.get() != nullptr)
        {
            statusLabel.setText({}, juce::dontSendNotification);

            shader->fillRect(g.getInternalContext(), getLocalBounds());
        }
    }

    void resized() override
    {
        statusLabel.setBounds(getLocalBounds().reduced(8).removeFromTop(70));
    }

    void setFragmentShaderCode(juce::String newFragmentCode)
    {
        if (fragmentCode == newFragmentCode)
            return;

        fragmentCode = std::move(newFragmentCode);
        shader.reset();
        repaint();
    }

    std::unique_ptr<juce::OpenGLGraphicsContextCustomShader> shader;

    juce::Label statusLabel;
    juce::String fragmentCode;

private:
    juce::OpenGLContext openGLContext;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGLDemo2D)
};
