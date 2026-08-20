#pragma once

#include <JuceHeader.h>

class OpenGLScopeView final : public juce::Component,
                              private juce::OpenGLRenderer
{
public:
    OpenGLScopeView()
    {
        setOpaque(true);
        openGLContext.setRenderer(this);
        openGLContext.setComponentPaintingEnabled(false);
        openGLContext.setContinuousRepainting(true);
        openGLContext.setOpenGLVersionRequired(juce::OpenGLContext::defaultGLVersion);
        openGLContext.attachTo(*this);
    }

    ~OpenGLScopeView() override
    {
        openGLContext.detach();
    }

    void setRenderData(const std::vector<float>& values,
                       const std::vector<juce::Point<float>>& particlePoints,
                       float glowValue,
                       float glowAmountValue,
                       float particleRadiusValue)
    {
        const juce::ScopedLock lock(dataLock);
        pendingSpectrum = values;
        pendingParticles = particlePoints;
        pendingGlow = glowValue;
        pendingGlowAmount = glowAmountValue;
        pendingParticleRadius = particleRadiusValue;
        hasPendingData = true;
    }

    void paint(juce::Graphics&) override {}
    void resized() override {}

private:
    struct Vertex { float x; float y; };

    static constexpr const char* vertexShader = R"glsl(
        attribute vec2 position;
        uniform float pointSize;
        void main() {
            gl_Position = vec4(position, 0.0, 1.0);
            gl_PointSize = pointSize;
        }
    )glsl";

    static constexpr const char* fragmentShader = R"glsl(
        uniform vec4 colour;
        uniform float pointMode;
        void main() {
            if (pointMode > 0.5) {
                vec2 p = gl_PointCoord - vec2(0.5);
                float alpha = 1.0 - smoothstep(0.15, 1.0, length(p) * 2.0);
                if (alpha <= 0.01) discard;
                gl_FragColor = vec4(colour.rgb, colour.a * alpha);
            } else {
                gl_FragColor = colour;
            }
        }
    )glsl";

    void newOpenGLContextCreated() override
    {
        shader = std::make_unique<juce::OpenGLShaderProgram>(openGLContext);
        if (!shader->addVertexShader(vertexShader)
            || !shader->addFragmentShader(fragmentShader)
            || !shader->link())
        {
            DBG("OpenGLScopeView shader error: " << shader->getLastError());
            shader.reset();
            return;
        }

        positionAttribute = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader, "position");
        juce::gl::glGenBuffers(1, &lineBuffer);
        juce::gl::glGenBuffers(1, &particleBuffer);
    }

    void openGLContextClosing() override
    {
        if (lineBuffer != 0)
            juce::gl::glDeleteBuffers(1, &lineBuffer);
        if (particleBuffer != 0)
            juce::gl::glDeleteBuffers(1, &particleBuffer);
        lineBuffer = 0;
        particleBuffer = 0;
        positionAttribute.reset();
        shader.reset();
    }

    void renderOpenGL() override
    {
        const auto scale = (float) openGLContext.getRenderingScale();
        const int width = juce::jmax(1, juce::roundToInt((float) getWidth() * scale));
        const int height = juce::jmax(1, juce::roundToInt((float) getHeight() * scale));
        juce::gl::glViewport(0, 0, width, height);
        juce::gl::glClearColor(0.018f, 0.030f, 0.045f, 1.0f);
        juce::gl::glClear(juce::gl::GL_COLOR_BUFFER_BIT);

        if (shader == nullptr || positionAttribute == nullptr)
            return;

        copyPendingData();
        if (spectrumVertices.empty())
            return;

        uploadBuffer(lineBuffer, spectrumVertices);
        uploadBuffer(particleBuffer, particleVertices);
        juce::gl::glEnable(juce::gl::GL_BLEND);
        juce::gl::glBlendFunc(juce::gl::GL_SRC_ALPHA, juce::gl::GL_ONE);
        shader->use();

        drawLine(0.028f, 0.045f, 0.060f, 0.95f, 16.0f);
        drawLine(0.060f, 0.18f, 0.16f, 0.95f, 10.0f);
        drawLine(0.105f, 0.30f, 0.24f, 0.95f, 6.0f);
        drawLine(0.86f, 0.82f, 0.92f, 1.0f, 3.0f);

        if (!particleVertices.empty())
        {
            shader->setUniform("pointMode", 1.0f);
            shader->setUniform("pointSize", juce::jmax(2.0f, particleRadius * 3.0f) * scale);
            shader->setUniform("colour", 0.45f, 1.0f, 0.68f, 0.18f + alphaGlow * 0.22f);
            drawBuffer(particleBuffer, juce::gl::GL_POINTS, (int) particleVertices.size());
            shader->setUniform("pointSize", juce::jmax(1.0f, particleRadius * 1.35f) * scale);
            shader->setUniform("colour", 0.88f, 1.0f, 0.93f, 0.75f);
            drawBuffer(particleBuffer, juce::gl::GL_POINTS, (int) particleVertices.size());
        }

        juce::gl::glDisable(juce::gl::GL_BLEND);
    }

    void copyPendingData()
    {
        const juce::ScopedLock lock(dataLock);
        if (!hasPendingData) return;

        spectrumVertices.clear();
        spectrumVertices.reserve(pendingSpectrum.size());
        for (size_t i = 0; i < pendingSpectrum.size(); ++i)
        {
            const float x = pendingSpectrum.size() > 1
                ? (float) i / (float) (pendingSpectrum.size() - 1) : 0.0f;
            spectrumVertices.push_back({ x * 2.0f - 1.0f, pendingSpectrum[i] * 2.0f - 1.0f });
        }

        particleVertices.clear();
        particleVertices.reserve(pendingParticles.size());
        for (const auto& point : pendingParticles)
        {
            const float x = getWidth() > 0 ? point.x / (float) getWidth() : 0.0f;
            const float y = getHeight() > 0 ? point.y / (float) getHeight() : 0.0f;
            particleVertices.push_back({ x * 2.0f - 1.0f, 1.0f - y * 2.0f });
        }

        alphaGlow = juce::jlimit(0.0f, 1.0f, pendingGlow * juce::jlimit(0.0f, 1.0f, pendingGlowAmount));
        particleRadius = pendingParticleRadius;
        hasPendingData = false;
    }

    void uploadBuffer(GLuint buffer, const std::vector<Vertex>& vertices)
    {
        juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, buffer);
        juce::gl::glBufferData(juce::gl::GL_ARRAY_BUFFER,
                               (GLsizeiptr) (vertices.size() * sizeof(Vertex)),
                               vertices.data(),
                               juce::gl::GL_DYNAMIC_DRAW);
    }

    void drawLine(float red, float green, float blue, float alpha, float lineWidth)
    {
        shader->setUniform("pointMode", 0.0f);
        shader->setUniform("pointSize", 1.0f);
        shader->setUniform("colour", red, green, blue, alpha * (0.2f + alphaGlow * 0.8f));
        juce::gl::glLineWidth(lineWidth);
        drawBuffer(lineBuffer, juce::gl::GL_LINE_STRIP, (int) spectrumVertices.size());
    }

    void drawBuffer(GLuint buffer, GLenum primitive, int count)
    {
        juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, buffer);
        juce::gl::glEnableVertexAttribArray(positionAttribute->attributeID);
        juce::gl::glVertexAttribPointer(positionAttribute->attributeID, 2, juce::gl::GL_FLOAT,
                                        juce::gl::GL_FALSE, sizeof(Vertex), nullptr);
        juce::gl::glDrawArrays(primitive, 0, count);
        juce::gl::glDisableVertexAttribArray(positionAttribute->attributeID);
        juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, 0);
    }

    juce::OpenGLContext openGLContext;
    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    std::unique_ptr<juce::OpenGLShaderProgram::Attribute> positionAttribute;
    GLuint lineBuffer { 0 };
    GLuint particleBuffer { 0 };
    juce::CriticalSection dataLock;
    std::vector<float> pendingSpectrum;
    std::vector<juce::Point<float>> pendingParticles;
    std::vector<Vertex> spectrumVertices;
    std::vector<Vertex> particleVertices;
    float pendingGlow { 0.75f };
    float pendingGlowAmount { 0.91f };
    float pendingParticleRadius { 1.0f };
    float alphaGlow { 0.7f };
    float particleRadius { 1.0f };
    bool hasPendingData { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGLScopeView)
};
