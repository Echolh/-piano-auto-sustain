#pragma once
#ifndef PAS_AU_MIDI
#define PAS_AU_MIDI 0
#endif
#include <juce_audio_utils/juce_audio_utils.h>
#include "PedalEngine.h"
#include "PluginState.h"
#include <atomic>

class PianoAutoSustainProcessor final : public juce::AudioProcessor {
public:
    PianoAutoSustainProcessor();
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void prepareToPlay(double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Piano Auto Sustain"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Automatic sustain"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<bool> uiPedalDown{false};
    std::atomic<int> uiNotes{0}, uiCc64{0};
    std::atomic<int> uiAction{0}; // 1: repedal, 2: release
private:
    void updateSettings();
    void process(juce::AudioBuffer<float>&, juce::MidiBuffer&, bool);
    float value(const char* id) const { return parameters.getRawParameterValue(id)->load(); }
    pas::PedalEngine engine;
    juce::MidiBuffer output;
    pas::PedalEngine::Time clock = 0;
    bool resetPending = true, wasPlaying = false;
    std::int64_t expectedHostSample = -1;
};
