#pragma once

#include <JuceHeader.h>
#include "AudioState.h"

class OpenGLScopeView final : public juce::Component,
                              private juce::OpenGLRenderer
{
public:
    explicit OpenGLScopeView(AudioState& state) : audioState(state)
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

    void setRenderData(const std::vector<float>& values)
    {
        const juce::ScopedLock lock(dataLock);
        pendingSpectrum = values;
        hasPendingData = true;
    }

    void paint(juce::Graphics&) override {}
    void resized() override {}

private:
    struct LineVertex { float x; float y; };
    struct ParticleVertex { float x; float y; float alpha; };
    struct Particle { float x; float y; float vy; float alpha; };

    static constexpr const char* vertexShader = R"glsl(
        attribute vec2 position;
        attribute float alphaIn;
        uniform float pointSize;
        uniform float pointMode;
        varying float vAlpha;
        void main() {
            gl_Position = vec4(position, 0.0, 1.0);
            gl_PointSize = pointSize;
            vAlpha = pointMode > 0.5 ? alphaIn : 1.0;
        }
    )glsl";

    static constexpr const char* fragmentShader = R"glsl(
        uniform vec4 colour;
        uniform float pointMode;
        varying float vAlpha;
        void main() {
            if (pointMode > 0.5) {
                vec2 p = gl_PointCoord - vec2(0.5);
                float alpha = 1.0 - smoothstep(0.15, 1.0, length(p) * 2.0);
                if (alpha <= 0.01) discard;
                gl_FragColor = vec4(colour.rgb, colour.a * alpha * vAlpha);
            } else {
                gl_FragColor = vec4(colour.rgb, colour.a * vAlpha);
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
        alphaAttribute = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader, "alphaIn");
        juce::gl::glEnable(juce::gl::GL_PROGRAM_POINT_SIZE);
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
        alphaAttribute.reset();
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
        syncVisualParams();
        updateSpectrumVerticesAndSpawn(width, height);
        updateParticles(width, height);

        if (spectrumVertices.empty())
            return;

        uploadBuffer(lineBuffer, spectrumVertices);
        buildParticleVertices();
        uploadBuffer(particleBuffer, particleVertices);
        juce::gl::glEnable(juce::gl::GL_BLEND);
        juce::gl::glBlendFunc(juce::gl::GL_SRC_ALPHA, juce::gl::GL_ONE);
        shader->use();

        drawGlowPoints(scale);
        drawLine(0.62f, 1.0f, 0.78f, 0.85f, 1.8f);
        drawLine(0.92f, 1.0f, 0.96f, 1.0f, 1.1f);

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

        currentSpectrum = pendingSpectrum;
        spectrumDirty = true;
        hasPendingData = false;
    }

    void syncVisualParams()
    {
        const float glow = audioState.glow.load();
        const float glowAmount = juce::jlimit(0.0f, 1.0f, audioState.glowAmount.load());
        const float frameMagnitude = getCurrentSpectrumPeak();
        const float glowScale = juce::jmap(glowAmount, 1.0f, juce::jlimit(0.0f, 1.0f, frameMagnitude));
        alphaGlow = juce::jlimit(0.0f, 1.0f, glow * glowScale);
        particleRadius = juce::jmax(0.1f, audioState.particleRadius.load());
    }

    float getCurrentSpectrumPeak() const
    {
        float peak = 0.0f;
        for (float v : currentSpectrum)
            peak = juce::jmax(peak, juce::jlimit(0.0f, 1.0f, v));
        return peak;
    }

    void updateSpectrumVerticesAndSpawn(int width, int height)
    {
        if (currentSpectrum.empty())
        {
            spectrumVertices.clear();
            return;
        }

        spectrumVertices.clear();
        spectrumVertices.reserve(currentSpectrum.size());
        for (size_t i = 0; i < currentSpectrum.size(); ++i)
        {
            const float x = currentSpectrum.size() > 1 ? (float)i / (float)(currentSpectrum.size() - 1) : 0.0f;
            const float y = juce::jlimit(0.0f, 1.0f, currentSpectrum[i]);
            spectrumVertices.push_back({ x * 2.0f - 1.0f, y * 2.0f - 1.0f });
        }

        if (!spectrumDirty || width <= 0 || height <= 0)
            return;

        spawnParticlesFromSpectrum(width, height);
        spectrumDirty = false;
    }

    void spawnParticlesFromSpectrum(int width, int height)
    {
        const int spawnStep = juce::jmax(1, audioState.particleSpawnStep.load());
        const size_t maxCount = (size_t)juce::jmax(1, audioState.particleMaxCount.load());
        const float initVyPx = juce::jmax(0.0f, audioState.particleInitVy.load());
        const float pxToNdcX = 2.0f / (float)juce::jmax(1, width);
        const float pxToNdcY = 2.0f / (float)juce::jmax(1, height);

        for (size_t i = 0; i < spectrumVertices.size(); i += (size_t)spawnStep)
        {
            if (particles.size() >= maxCount)
                break;

            const float jitter = (particleRandom.nextFloat() - 0.5f) * 2.0f;
            Particle p;
            p.x = spectrumVertices[i].x + jitter * (1.5f * pxToNdcX);
            p.y = spectrumVertices[i].y;
            p.vy = -(initVyPx * (0.8f + particleRandom.nextFloat() * 0.4f)) * pxToNdcY;
            p.alpha = 1.0f;
            particles.push_back(p);
        }
    }

    void updateParticles(int width, int height)
    {
        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        if (lastFrameTimeMs <= 0.0)
        {
            lastFrameTimeMs = nowMs;
            return;
        }

        const float dt = juce::jlimit(0.0f, 0.05f, (float)((nowMs - lastFrameTimeMs) * 0.001));
        lastFrameTimeMs = nowMs;
        if (dt <= 0.0f || particles.empty())
            return;

        const float pxToNdcY = 2.0f / (float)juce::jmax(1, height);
        const float gravity = -audioState.particleGravity.load() * pxToNdcY;
        const float fadeRate = juce::jmax(0.0f, audioState.particleFadeRate.load());

        for (size_t i = 0; i < particles.size();)
        {
            auto& p = particles[i];
            p.vy += gravity * dt;
            p.y += p.vy * dt;
            p.alpha -= fadeRate * dt;

            if (p.alpha <= 0.0f || p.y < -1.1f)
            {
                particles[i] = particles.back();
                particles.pop_back();
            }
            else
            {
                ++i;
            }
        }
    }

    void buildParticleVertices()
    {
        particleVertices.clear();
        particleVertices.reserve(particles.size());
        for (const auto& p : particles)
            particleVertices.push_back({ p.x, p.y, juce::jlimit(0.0f, 1.0f, p.alpha) });
    }

    void uploadBuffer(GLuint buffer, const std::vector<LineVertex>& vertices)
    {
        juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, buffer);
        juce::gl::glBufferData(juce::gl::GL_ARRAY_BUFFER,
                               (GLsizeiptr) (vertices.size() * sizeof(LineVertex)),
                               vertices.data(),
                               juce::gl::GL_DYNAMIC_DRAW);
    }

    void uploadBuffer(GLuint buffer, const std::vector<ParticleVertex>& vertices)
    {
        juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, buffer);
        juce::gl::glBufferData(juce::gl::GL_ARRAY_BUFFER,
                               (GLsizeiptr) (vertices.size() * sizeof(ParticleVertex)),
                               vertices.data(),
                               juce::gl::GL_DYNAMIC_DRAW);
    }

    void drawLine(float red, float green, float blue, float alpha, float lineWidth)
    {
        shader->setUniform("pointMode", 0.0f);
        shader->setUniform("pointSize", 1.0f);
        shader->setUniform("colour", red, green, blue, alpha * (0.2f + alphaGlow * 0.8f));
        juce::gl::glLineWidth(lineWidth);
        drawLineBuffer(lineBuffer, juce::gl::GL_LINE_STRIP, (int) spectrumVertices.size());
    }

    void drawGlowPoints(float scale)
    {
        const float glowStrength = 0.25f + alphaGlow * 0.75f;

        shader->setUniform("pointMode", 1.0f);
        shader->setUniform("pointSize", juce::jmax(2.0f, 18.0f * scale));
        shader->setUniform("colour", 0.06f, 0.22f, 0.18f, 0.09f * glowStrength);
        drawLineBuffer(lineBuffer, juce::gl::GL_POINTS, (int)spectrumVertices.size());

        shader->setUniform("pointSize", juce::jmax(1.0f, 10.0f * scale));
        shader->setUniform("colour", 0.18f, 0.55f, 0.42f, 0.15f * glowStrength);
        drawLineBuffer(lineBuffer, juce::gl::GL_POINTS, (int)spectrumVertices.size());

        shader->setUniform("pointSize", juce::jmax(1.0f, 6.0f * scale));
        shader->setUniform("colour", 0.58f, 1.0f, 0.80f, 0.20f * glowStrength);
        drawLineBuffer(lineBuffer, juce::gl::GL_POINTS, (int)spectrumVertices.size());
    }

    void drawLineBuffer(GLuint buffer, GLenum primitive, int count)
    {
        juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, buffer);
        juce::gl::glEnableVertexAttribArray(positionAttribute->attributeID);
        juce::gl::glVertexAttribPointer(positionAttribute->attributeID, 2, juce::gl::GL_FLOAT,
                                        juce::gl::GL_FALSE, sizeof(LineVertex), nullptr);
        if (alphaAttribute != nullptr)
        {
            juce::gl::glDisableVertexAttribArray(alphaAttribute->attributeID);
            juce::gl::glVertexAttrib1f(alphaAttribute->attributeID, 1.0f);
        }
        juce::gl::glDrawArrays(primitive, 0, count);
        juce::gl::glDisableVertexAttribArray(positionAttribute->attributeID);
        juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, 0);
    }

    void drawBuffer(GLuint buffer, GLenum primitive, int count)
    {
        juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, buffer);
        juce::gl::glEnableVertexAttribArray(positionAttribute->attributeID);
        juce::gl::glVertexAttribPointer(positionAttribute->attributeID, 2, juce::gl::GL_FLOAT,
                                        juce::gl::GL_FALSE, sizeof(ParticleVertex), nullptr);
        if (alphaAttribute != nullptr)
        {
            juce::gl::glEnableVertexAttribArray(alphaAttribute->attributeID);
            juce::gl::glVertexAttribPointer(alphaAttribute->attributeID, 1, juce::gl::GL_FLOAT,
                                            juce::gl::GL_FALSE, sizeof(ParticleVertex),
                                            (const void*) offsetof(ParticleVertex, alpha));
        }
        juce::gl::glDrawArrays(primitive, 0, count);
        if (alphaAttribute != nullptr)
            juce::gl::glDisableVertexAttribArray(alphaAttribute->attributeID);
        juce::gl::glDisableVertexAttribArray(positionAttribute->attributeID);
        juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, 0);
    }

    AudioState& audioState;
    juce::OpenGLContext openGLContext;
    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    std::unique_ptr<juce::OpenGLShaderProgram::Attribute> positionAttribute;
    std::unique_ptr<juce::OpenGLShaderProgram::Attribute> alphaAttribute;
    GLuint lineBuffer { 0 };
    GLuint particleBuffer { 0 };
    juce::CriticalSection dataLock;
    std::vector<float> pendingSpectrum;
    std::vector<float> currentSpectrum;
    std::vector<LineVertex> spectrumVertices;
    std::vector<ParticleVertex> particleVertices;
    std::vector<Particle> particles;
    juce::Random particleRandom;
    bool spectrumDirty { false };
    double lastFrameTimeMs { 0.0 };
    float alphaGlow { 0.7f };
    float particleRadius { 1.0f };
    bool hasPendingData { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGLScopeView)
};
