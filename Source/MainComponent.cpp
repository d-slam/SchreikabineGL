#include "MainComponent.h"
#include "UIComponent.h"
#include "AudioState.h"

MainComponent::MainComponent()
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize(800, 600); // no-op patch applied

    audioState.reset(new AudioState);

    uiComp.reset(new UIComponent(*audioState));
    addAndMakeVisible(uiComp.get());

    // ensure UI has initial bounds immediately
    uiComp->setBounds(getLocalBounds());
    uiComp->repaint();

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired(juce::RuntimePermissions::recordAudio) && !juce::RuntimePermissions::isGranted(juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request(juce::RuntimePermissions::recordAudio,
            [&](bool granted)
            { setAudioChannels(granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        // register formats and prepare a polyphonic synth
        formatManager.registerBasicFormats();

        const int numVoices = 8; // polyphony
        for (int i = 0; i < numVoices; ++i)
        {
            synth1.addVoice(new juce::SamplerVoice());
            synth2.addVoice(new juce::SamplerVoice());
            synth3.addVoice(new juce::SamplerVoice());
        }

        setAudioChannels(2, 2);

        // audioState and uiComp were already created earlier to ensure UI is visible
        // during potential permission delays. Do not recreate them here.
        // The UI bindings to audioState are idempotent and were set above.

        // Callbacks (wire UI to MainComponent logic)
        uiComp->settingsComponent->loadButton1->onClick = [this]()
            {
                auto chooser = std::make_shared<juce::FileChooser>("Select a WAV file to load", juce::File(), "*.wav");
                chooser->launchAsync(juce::FileBrowserComponent::openMode, [this, chooser](const juce::FileChooser& fc)
                    {
                        auto f = fc.getResult();
                        juce::MessageManager::callAsync([this, f]()
                            {
                                addSampleFromFile(f, sampleNote1);
                                uiComp->settingsComponent->fileLabel1->setText(f.getFileName(), juce::dontSendNotification);
                            }); });
            };

        uiComp->settingsComponent->loadButton2->onClick = [this]()
            {
                auto chooser = std::make_shared<juce::FileChooser>("Select a WAV file to load", juce::File(), "*.wav");
                chooser->launchAsync(juce::FileBrowserComponent::openMode, [this, chooser](const juce::FileChooser& fc)
                    {
                        auto f = fc.getResult();
                        juce::MessageManager::callAsync([this, f]()
                            {
                                addSampleFromFile(f, sampleNote2);
                                uiComp->settingsComponent->fileLabel2->setText(f.getFileName(), juce::dontSendNotification);
                            }); });
            };

        uiComp->settingsComponent->loadButton3->onClick = [this]()
            {
                auto chooser = std::make_shared<juce::FileChooser>("Select a WAV file to load", juce::File(), "*.wav");
                chooser->launchAsync(juce::FileBrowserComponent::openMode, [this, chooser](const juce::FileChooser& fc)
                    {
                        auto f = fc.getResult();
                        juce::MessageManager::callAsync([this, f]()
                            {
                                addSampleFromFile(f, sampleNote3);
                                uiComp->settingsComponent->fileLabel3->setText(f.getFileName(), juce::dontSendNotification);
                            }); });
            };

        // Use constant velocity=1.0f and control loudness via audioState vol1/vol2/vol3 only
        uiComp->settingsComponent->playButton1->onClick = [this]() { synth1.noteOn(1, sampleNote1, 1.0f); };
        uiComp->settingsComponent->playButton2->onClick = [this]() { synth2.noteOn(1, sampleNote2, 1.0f); };
        uiComp->settingsComponent->playButton3->onClick = [this]() { synth3.noteOn(1, sampleNote3, 1.0f); };
        uiComp->settingsComponent->playBothButton->onClick = [this]()
            {
                synth1.noteOn(1, sampleNote1, 1.0f);
                synth2.noteOn(1, sampleNote2, 1.0f);
                synth3.noteOn(1, sampleNote3, 1.0f);
            };

        uiComp->settingsComponent->stopBothButton->onClick = [this]()
            {
                // try to stop the specific notes immediately
                synth1.noteOff(1, sampleNote1, 0.0f, false);
                synth2.noteOff(1, sampleNote2, 0.0f, false);
                synth3.noteOff(1, sampleNote3, 0.0f, false);
                // ensure any remaining voices on channel 1 are stopped
                synth1.allNotesOff(1, false);
                synth2.allNotesOff(1, false);
                synth3.allNotesOff(1, false);
            };

        // UI has its own elements; nothing else to do here

        // Create and register custom audio callback for zero-latency monitoring + synth mixing (aligned)
        customAudioCallback = std::make_unique<LocalAudioCallback>(synth1, synth2, synth3, *audioState);

        // no-op: ensure creation path uses concrete LocalAudioCallback unique_ptr (kept same)
        deviceManager.addAudioCallback(customAudioCallback.get());
        useCustomAudioCallback = true;

        // Versuch, beim Start Samples aus C:\\Samples automatisch zu laden
        juce::File defaultFolder("C:\\Samples");
        autoLoadSamplesFromFolder(defaultFolder);
    }
}

MainComponent::~MainComponent()
{
    // Remove custom audio callback if registered
    if (customAudioCallback) deviceManager.removeAudioCallback(customAudioCallback.get());

    // unique_ptr will clean up uiComp and audioState automatically

    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    // set sampler sample rate for both synths
    synth1.setCurrentPlaybackSampleRate(sampleRate);
    synth2.setCurrentPlaybackSampleRate(sampleRate);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
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
    synth1.renderNextBlock(*bufferToFill.buffer, emptyMidi, 0, bufferToFill.numSamples);
    synth2.renderNextBlock(*bufferToFill.buffer, emptyMidi, 0, bufferToFill.numSamples);
}

void MainComponent::releaseResources()
{
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setFont(juce::FontOptions(16.0f));
    g.setColour(juce::Colours::white);
    g.drawText("Hello World!", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    // delegate layout to UIComponent: make it fill the available area
    if (uiComp != nullptr) uiComp->setBounds(getLocalBounds());
}
// helper: add sample file as a SamplerSound to the synth
void MainComponent::addSampleFromFile(const juce::File& f, int rootNote)
{
    if (!f.existsAsFile()) return;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(f));
    if (reader.get() == nullptr) return;

    juce::BigInteger noteMap;
    noteMap.clear();
    // map this sound only to its root note so different samples don't override each other
    noteMap.setBit((unsigned int)rootNote);
    const double attack = 0.01, release = 0.1;

    // Route sounds to synth1/synth2/synth3 depending on rootNote
    if (rootNote == sampleNote1)        synth1.addSound(new juce::SamplerSound(f.getFileName(), *reader, noteMap, rootNote, attack, release, 10.0));
    else if (rootNote == sampleNote2)   synth2.addSound(new juce::SamplerSound(f.getFileName(), *reader, noteMap, rootNote, attack, release, 10.0));
    else if (rootNote == sampleNote3)   synth3.addSound(new juce::SamplerSound(f.getFileName(), *reader, noteMap, rootNote, attack, release, 10.0));
}

void MainComponent::autoLoadSamplesFromFolder(const juce::File& folder)
{
    if (!folder.exists() || !folder.isDirectory()) return;

    juce::Array<juce::File> files;
    folder.findChildFiles(files, juce::File::findFiles, false, "*.wav;*.WAV");

    if (files.size() > 0)
    {
        addSampleFromFile(files[0], sampleNote1);
        if (uiComp != nullptr) uiComp->settingsComponent->fileLabel1->setText(files[0].getFileName(), juce::dontSendNotification);
    }

    if (files.size() > 1)
    {
        addSampleFromFile(files[1], sampleNote2);
        if (uiComp != nullptr) uiComp->settingsComponent->fileLabel2->setText(files[1].getFileName(), juce::dontSendNotification);
    }

    if (files.size() > 2)
    {
        addSampleFromFile(files[2], sampleNote3);
        if (uiComp != nullptr) uiComp->settingsComponent->fileLabel3->setText(files[2].getFileName(), juce::dontSendNotification);
    }
}
