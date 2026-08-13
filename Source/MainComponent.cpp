#include "MainComponent.h"
#include "UIComponent.h"
#include "AudioState.h"

//==============================================================================
// Local audio callback implementation that copies input to output (monitoring)
// and renders the synth into the output buffer. Runs on the audio thread.
struct LocalAudioCallback  : public juce::AudioIODeviceCallback
{
    LocalAudioCallback (juce::Synthesiser& s1, juce::Synthesiser& s2, juce::Synthesiser& s3, AudioState& state)
        : synth1 (s1), synth2 (s2), synth3 (s3), monitorEnabled (state.monitorEnabled), cutoffHzRef (state.cutoffHz), vol1 (state.vol1), vol2 (state.vol2), vol3 (state.vol3), filterType (state.filterType)
    {}

    void audioDeviceAboutToStart (juce::AudioIODevice* device) override
    {
        sampleRate = device != nullptr ? device->getCurrentSampleRate() : 44100.0;
        if (sampleRate <= 0.0)
            sampleRate = 44100.0; // fallback to safe default

        // ensure synths know the current sample rate before any renderNextBlock calls
        synth1.setCurrentPlaybackSampleRate (sampleRate);
        synth2.setCurrentPlaybackSampleRate (sampleRate);
        synth3.setCurrentPlaybackSampleRate (sampleRate);

        // initialize coefficients (use safe sampleRate)
        lastCutoff = cutoffHzRef.load();
        const double sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        juce::IIRCoefficients coeffs = juce::IIRCoefficients::makeLowPass (sr, lastCutoff);
        // prepare filters will be resized in the first callback when numOutputChannels is known
        coeff = coeffs;
    }

    void audioDeviceStopped() override {}

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData, int numInputChannels,
                                float* const* outputChannelData, int numOutputChannels,
                                int numSamples,
                                const juce::AudioIODeviceCallbackContext& /*context*/) override
    {
        // ensure filter arrays match output channels
        if (monitorFilters.size() != (size_t) numOutputChannels)
        {
            monitorFilters.clear(); sample1Filters.clear();
            monitorFilters.resize ((size_t) numOutputChannels);
            sample1Filters.resize ((size_t) numOutputChannels);
            for (int ch = 0; ch < numOutputChannels; ++ch)
            {
                monitorFilters[(size_t) ch].reset();
                sample1Filters[(size_t) ch].reset();
                monitorFilters[(size_t) ch].setCoefficients (coeff);
                sample1Filters[(size_t) ch].setCoefficients (coeff);
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
            if (currentType == 0)
                newc = juce::IIRCoefficients::makeLowPass (sr, lastCutoff);
            else
                newc = juce::IIRCoefficients::makeHighPass (sr, lastCutoff);

            for (auto& f : monitorFilters) f.setCoefficients (newc);
            for (auto& f : sample1Filters) f.setCoefficients (newc);
        }

        // prepare temporary buffers
        juce::AudioBuffer<float> inBuf (numOutputChannels, numSamples);
        juce::AudioBuffer<float> s1Buf (numOutputChannels, numSamples);
        juce::AudioBuffer<float> s2Buf (numOutputChannels, numSamples);
        juce::AudioBuffer<float> s3Buf (numOutputChannels, numSamples);

        // clear outputs
        for (int ch = 0; ch < numOutputChannels; ++ch)
            if (outputChannelData[ch] != nullptr)
                juce::FloatVectorOperations::clear (outputChannelData[ch], numSamples);

        // prepare inBuf: clear and copy input only when monitoring enabled
        inBuf.clear();
        if (monitorEnabled.load())
        {
            for (int ch = 0; ch < juce::jmin (numInputChannels, numOutputChannels); ++ch)
                if (inputChannelData[ch] != nullptr)
                    inBuf.copyFrom (ch, 0, inputChannelData[ch], numSamples);
        }

        // render synth1, synth2 and synth3 into their buffers
        juce::MidiBuffer emptyMidi;
        s1Buf.clear(); s2Buf.clear(); s3Buf.clear();
        synth1.renderNextBlock (s1Buf, emptyMidi, 0, numSamples);
        synth2.renderNextBlock (s2Buf, emptyMidi, 0, numSamples);
        synth3.renderNextBlock (s3Buf, emptyMidi, 0, numSamples);

        // apply filters: monitor (inBuf) and sample1 (s1Buf)
        if (monitorEnabled.load())
        {
            for (int ch = 0; ch < numOutputChannels; ++ch)
                monitorFilters[(size_t) ch].processSamples (inBuf.getWritePointer (ch), numSamples);
        }

        for (int ch = 0; ch < numOutputChannels; ++ch)
            sample1Filters[(size_t) ch].processSamples (s1Buf.getWritePointer (ch), numSamples);

        // mix inBuf + s1Buf + s2Buf + s3Buf into outputs with per-sample gain multipliers
        for (int ch = 0; ch < numOutputChannels; ++ch)
        {
            float* out = outputChannelData[ch];
            const float* inP = inBuf.getReadPointer (ch);
            const float* s1P = s1Buf.getReadPointer (ch);
            const float* s2P = s2Buf.getReadPointer (ch);
            const float* s3P = s3Buf.getReadPointer (ch);
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


MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600); // no-op patch applied

    audioState.reset(new AudioState);
    auto localUi = std::make_unique<UIComponent>(*audioState);


    uiComp = std::move (localUi);
    addAndMakeVisible (uiComp.get());
    // ensure UI has initial bounds immediately
    uiComp->setBounds (getLocalBounds());
    uiComp->repaint();

    // initialize UI controls from audioState
    uiComp->settingsComponent->volSlider1.setValue (audioState->vol1.load(), juce::dontSendNotification);
    uiComp->settingsComponent->volSlider1.onValueChange = [this]() { audioState->vol1.store ((float) uiComp->settingsComponent->volSlider1.getValue()); };

    uiComp->settingsComponent->volSlider2.setValue (audioState->vol2.load(), juce::dontSendNotification);
    uiComp->settingsComponent->volSlider2.onValueChange = [this]() { audioState->vol2.store ((float) uiComp->settingsComponent->volSlider2.getValue()); };

    uiComp->settingsComponent->volSlider3.setValue (audioState->vol3.load(), juce::dontSendNotification);
    uiComp->settingsComponent->volSlider3.onValueChange = [this]() { audioState->vol3.store ((float) uiComp->settingsComponent->volSlider3.getValue()); };

    uiComp->settingsComponent->cutoffSlider.setValue (audioState->cutoffHz.load(), juce::dontSendNotification);
    uiComp->settingsComponent->cutoffSlider.onValueChange = [this]() { audioState->cutoffHz.store ((float) uiComp->settingsComponent->cutoffSlider.getValue()); uiComp->settingsComponent->cutoffLabel.setText (juce::String ((int) audioState->cutoffHz.load()) + " Hz", juce::dontSendNotification); };

    uiComp->settingsComponent->filterTypeBox.setSelectedId (audioState->filterType.load() == 0 ? 1 : 2, juce::dontSendNotification);
    uiComp->settingsComponent->filterTypeBox.onChange = [this]() { audioState->filterType.store (uiComp->settingsComponent->filterTypeBox.getSelectedId() == 1 ? 0 : 1); };

    uiComp->settingsComponent->monitorButton.onClick = [this]() { audioState->monitorEnabled.store (! audioState->monitorEnabled.load()); uiComp->settingsComponent->monitorButton.setButtonText (audioState->monitorEnabled.load() ? "Monitor: ON" : "Monitor: OFF"); };

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        // register formats and prepare a polyphonic synth
        formatManager.registerBasicFormats();

        const int numVoices = 8; // polyphony
        for (int i = 0; i < numVoices; ++i)
        {
            synth1.addVoice (new juce::SamplerVoice());
            synth2.addVoice (new juce::SamplerVoice());
            synth3.addVoice (new juce::SamplerVoice());
        }

        setAudioChannels (2, 2);

        // audioState and uiComp were already created earlier to ensure UI is visible
        // during potential permission delays. Do not recreate them here.
        // The UI bindings to audioState are idempotent and were set above.

        // Callbacks (wire UI to MainComponent logic)
        uiComp->settingsComponent->loadButton1.onClick = [this]()
        {
            auto chooser = std::make_shared<juce::FileChooser> ("Select a WAV file to load", juce::File(), "*.wav");
            chooser->launchAsync (juce::FileBrowserComponent::openMode, [this, chooser] (const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                juce::MessageManager::callAsync ([this, f]()
                {
                    addSampleFromFile (f, sampleNote1);
                    uiComp->settingsComponent->fileLabel1.setText (f.getFileName(), juce::dontSendNotification);
                });
            });
        };

        uiComp->settingsComponent->loadButton2.onClick = [this]()
        {
            auto chooser = std::make_shared<juce::FileChooser> ("Select a WAV file to load", juce::File(), "*.wav");
            chooser->launchAsync (juce::FileBrowserComponent::openMode, [this, chooser] (const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                juce::MessageManager::callAsync ([this, f]()
                {
                    addSampleFromFile (f, sampleNote2);
                    uiComp->settingsComponent->fileLabel2.setText (f.getFileName(), juce::dontSendNotification);
                });
            });
        };

        // Use constant velocity=1.0f and control loudness via audioState vol1/vol2/vol3 only
        uiComp->settingsComponent->playButton1.onClick = [this]() { synth1.noteOn (1, sampleNote1, 1.0f); };
        uiComp->settingsComponent->playButton2.onClick = [this]() { synth2.noteOn (1, sampleNote2, 1.0f); };
        uiComp->settingsComponent->playButton3.onClick = [this]() { synth3.noteOn (1, sampleNote3, 1.0f); };
        uiComp->settingsComponent->playBothButton.onClick = [this]()
        {
            synth1.noteOn (1, sampleNote1, 1.0f);
            synth2.noteOn (1, sampleNote2, 1.0f);
            synth3.noteOn (1, sampleNote3, 1.0f);
        };

        uiComp->settingsComponent->stopBothButton.onClick = [this]()
        {
            // try to stop the specific notes immediately
            synth1.noteOff (1, sampleNote1, 0.0f, false);
            synth2.noteOff (1, sampleNote2, 0.0f, false);
            synth3.noteOff (1, sampleNote3, 0.0f, false);
            // ensure any remaining voices on channel 1 are stopped
            synth1.allNotesOff (1, false);
            synth2.allNotesOff (1, false);
            synth3.allNotesOff (1, false);
        };

        // UI has its own elements; nothing else to do here

        // Create and register custom audio callback for zero-latency monitoring + synth mixing (aligned)
        customAudioCallback = std::make_unique<LocalAudioCallback> (synth1, synth2, synth3, *audioState);

        // no-op: ensure creation path uses concrete LocalAudioCallback unique_ptr (kept same)
        deviceManager.addAudioCallback (customAudioCallback.get());
        useCustomAudioCallback = true;

        // Versuch, beim Start Samples aus C:\\Samples automatisch zu laden
        juce::File defaultFolder ("C:\\Samples");
        autoLoadSamplesFromFolder (defaultFolder);

    }
}

MainComponent::~MainComponent()
{
    // Remove custom audio callback if registered
    if (customAudioCallback)
        deviceManager.removeAudioCallback (customAudioCallback.get());

    // unique_ptr will clean up uiComp and audioState automatically

    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // set sampler sample rate for both synths
    synth1.setCurrentPlaybackSampleRate (sampleRate);
    synth2.setCurrentPlaybackSampleRate (sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{

    // If we're using the custom audio callback, it handles rendering and monitoring.
    if (useCustomAudioCallback)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    // Fallback: render both synths into the buffer (original behaviour)
    bufferToFill.clearActiveBufferRegion();
    juce::MidiBuffer emptyMidi;
    synth1.renderNextBlock (*bufferToFill.buffer, emptyMidi, 0, bufferToFill.numSamples);
    synth2.renderNextBlock (*bufferToFill.buffer, emptyMidi, 0, bufferToFill.numSamples);
}

void MainComponent::releaseResources()
{

}


//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setFont (juce::FontOptions (16.0f));
    g.setColour (juce::Colours::white);
    g.drawText ("Hello World!", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    // delegate layout to UIComponent: make it fill the available area
    if (uiComp != nullptr)
        uiComp->setBounds (getLocalBounds());
}
// helper: add sample file as a SamplerSound to the synth
void MainComponent::addSampleFromFile (const juce::File& f, int rootNote)
{
    if (! f.existsAsFile())
        return;

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (f));
    if (reader.get() == nullptr)
        return;

    juce::BigInteger noteMap;
    noteMap.clear();
    // map this sound only to its root note so different samples don't override each other
    noteMap.setBit ((unsigned int) rootNote);
    const double attack = 0.01, release = 0.1;

    // Route sounds to synth1/synth2/synth3 depending on rootNote
    if (rootNote == sampleNote1)
        synth1.addSound (new juce::SamplerSound (f.getFileName(), *reader, noteMap, rootNote, attack, release, 10.0));
    else if (rootNote == sampleNote2)
        synth2.addSound (new juce::SamplerSound (f.getFileName(), *reader, noteMap, rootNote, attack, release, 10.0));
    else if (rootNote == sampleNote3)
        synth3.addSound (new juce::SamplerSound (f.getFileName(), *reader, noteMap, rootNote, attack, release, 10.0));
}

void MainComponent::autoLoadSamplesFromFolder (const juce::File& folder)
{
    if (! folder.exists() || ! folder.isDirectory())
        return;

    juce::Array<juce::File> files;
    folder.findChildFiles (files, juce::File::findFiles, false, "*.wav;*.WAV");

    if (files.size() > 0)
    {
        addSampleFromFile (files[0], sampleNote1);
        if (uiComp != nullptr)
            uiComp->settingsComponent->fileLabel1.setText (files[0].getFileName(), juce::dontSendNotification);
    }

    if (files.size() > 1)
    {
        addSampleFromFile (files[1], sampleNote2);
        if (uiComp != nullptr)
            uiComp->settingsComponent->fileLabel2.setText (files[1].getFileName(), juce::dontSendNotification);
    }

    if (files.size() > 2)
    {
        addSampleFromFile (files[2], sampleNote3);
        if (uiComp != nullptr)
            uiComp->settingsComponent->fileLabel3.setText (files[2].getFileName(), juce::dontSendNotification);
    }
}
