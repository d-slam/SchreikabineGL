#pragma once
#include <JuceHeader.h>

//==============================================================================
// Local audio callback implementation that copies input to output (monitoring)
// and renders the synth into the output buffer. Runs on the audio thread.
struct LocalAudioCallback : public juce::AudioIODeviceCallback
{
    LocalAudioCallback(juce::Synthesiser& s1, juce::Synthesiser& s2, juce::Synthesiser& s3, AudioState& state)
        : synth1(s1), synth2(s2), synth3(s3), monitorEnabled(state.monitorEnabled), cutoffHzRef(state.cutoffHz), vol1(state.vol1), vol2(state.vol2), vol3(state.vol3), filterType(state.filterType)
    {
    }

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override
    {
        sampleRate = device != nullptr ? device->getCurrentSampleRate() : 44100.0;
        if (sampleRate <= 0.0) sampleRate = 44100.0; // fallback to safe default

        // ensure synths know the current sample rate before any renderNextBlock calls
        synth1.setCurrentPlaybackSampleRate(sampleRate);
        synth2.setCurrentPlaybackSampleRate(sampleRate);
        synth3.setCurrentPlaybackSampleRate(sampleRate);

        // initialize coefficients (use safe sampleRate)
        lastCutoff = cutoffHzRef.load();
        const double sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        juce::IIRCoefficients coeffs = juce::IIRCoefficients::makeLowPass(sr, lastCutoff);
        // prepare filters will be resized in the first callback when numOutputChannels is known
        coeff = coeffs;
    }

    void audioDeviceStopped() override {}

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
        float* const* outputChannelData, int numOutputChannels, int numSamples,
        const juce::AudioIODeviceCallbackContext& /*context*/) override
    {
        // ensure filter arrays match output channels
        if (monitorFilters.size() != (size_t)numOutputChannels)
        {
            monitorFilters.clear();
            sample1Filters.clear();
            monitorFilters.resize((size_t)numOutputChannels);
            sample1Filters.resize((size_t)numOutputChannels);
            for (int ch = 0; ch < numOutputChannels; ++ch)
            {
                monitorFilters[(size_t)ch].reset();
                sample1Filters[(size_t)ch].reset();
                monitorFilters[(size_t)ch].setCoefficients(coeff);
                sample1Filters[(size_t)ch].setCoefficients(coeff);
            }
        }

        // update coefficients if cutoff or filter type changed
        float currentCut = cutoffHzRef.load();
        int currentType = filterType.load();
        if (currentCut != lastCutoff || currentType != lastFilterType)
        {
            lastCutoff = currentCut;
            lastFilterType = currentType;
            juce::IIRCoefficients newc;
            const double sr = (sampleRate > 0.0) ? sampleRate : 44100.0;
            if (currentType == 0)                newc = juce::IIRCoefficients::makeLowPass(sr, lastCutoff);
            else                                 newc = juce::IIRCoefficients::makeHighPass(sr, lastCutoff);

            for (auto& f : monitorFilters)                f.setCoefficients(newc);
            for (auto& f : sample1Filters)                f.setCoefficients(newc);
        }

        // prepare temporary buffers
        juce::AudioBuffer<float> inBuf(numOutputChannels, numSamples);
        juce::AudioBuffer<float> s1Buf(numOutputChannels, numSamples);
        juce::AudioBuffer<float> s2Buf(numOutputChannels, numSamples);
        juce::AudioBuffer<float> s3Buf(numOutputChannels, numSamples);

        // clear outputs        <<- meh, if sollte vorn for loop sein....check to fix
        for (int ch = 0; ch < numOutputChannels; ++ch)
            if (outputChannelData[ch] != nullptr)
                juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);

        // prepare inBuf: clear and copy input only when monitoring enabled
        inBuf.clear();
        if (monitorEnabled.load())
        {
            for (int ch = 0; ch < juce::jmin(numInputChannels, numOutputChannels); ++ch)
                if (inputChannelData[ch] != nullptr)
                    inBuf.copyFrom(ch, 0, inputChannelData[ch], numSamples);
        }

        // render synth1, synth2 and synth3 into their buffers
        juce::MidiBuffer emptyMidi;
        s1Buf.clear();
        s2Buf.clear();
        s3Buf.clear();
        synth1.renderNextBlock(s1Buf, emptyMidi, 0, numSamples);
        synth2.renderNextBlock(s2Buf, emptyMidi, 0, numSamples);
        synth3.renderNextBlock(s3Buf, emptyMidi, 0, numSamples);

        // apply filters: monitor (inBuf) and sample1 (s1Buf)
        if (monitorEnabled.load())
        {
            for (int ch = 0; ch < numOutputChannels; ++ch)
                monitorFilters[(size_t)ch].processSamples(inBuf.getWritePointer(ch), numSamples);
        }

        for (int ch = 0; ch < numOutputChannels; ++ch)
            sample1Filters[(size_t)ch].processSamples(s1Buf.getWritePointer(ch), numSamples);

        // mix inBuf + s1Buf + s2Buf + s3Buf into outputs with per-sample gain multipliers
        for (int ch = 0; ch < numOutputChannels; ++ch)
        {
            float* out = outputChannelData[ch];
            const float* inP = inBuf.getReadPointer(ch);
            const float* s1P = s1Buf.getReadPointer(ch);
            const float* s2P = s2Buf.getReadPointer(ch);
            const float* s3P = s3Buf.getReadPointer(ch);
            const float g1 = vol1.load();
            const float g2 = vol2.load();
            const float g3 = vol3.load();
            for (int i = 0; i < numSamples; ++i)
                out[i] = inP[i] + s1P[i] * g1 + s2P[i] * g2 + s3P[i] * g3;
        }
    }

    juce::Synthesiser& synth1;
    juce::Synthesiser& synth2;
    juce::Synthesiser& synth3;
    std::atomic<bool>& monitorEnabled;
    std::atomic<float>& cutoffHzRef;
    std::atomic<float>& vol1;
    std::atomic<float>& vol2;
    std::atomic<float>& vol3;
    std::atomic<int>& filterType;

    std::vector<juce::IIRFilter> monitorFilters, sample1Filters;
    double sampleRate = 44100.0;
    float lastCutoff = 0.0f;
    int lastFilterType = 0;
    juce::IIRCoefficients coeff;
};