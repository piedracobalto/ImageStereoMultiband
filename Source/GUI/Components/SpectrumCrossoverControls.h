#pragma once

#include <JuceHeader.h>
#include "../BandColours.h"

class SpectrumCrossoverControls : public juce::Component,
                                  private juce::AudioProcessorValueTreeState::Listener,
                                  private juce::AsyncUpdater
{
public:
    explicit SpectrumCrossoverControls(juce::AudioProcessorValueTreeState& apvts);
    ~SpectrumCrossoverControls() override;

    void pushSpectrum(const float* data, int numBins, float maxFrequencyHz);
    void setBandStates(const bool* muted, const bool* soloed, int numBands);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;

    juce::Rectangle<int> getGraphBounds() const;
    float valueToX(float frequency) const;
    float xToValue(float x) const;
    float getConstrainedFrequency(int index, float frequency) const;
    void setCrossoverFrequency(int index, float frequency);
    int findNearestHandle(juce::Point<int> position) const;
    juce::String formatFrequency(float frequency) const;

    void startEditingCrossover(int index);
    void finishEditingCrossover();

    juce::AudioProcessorValueTreeState& valueTreeState;
    juce::Label titleLabel;
    juce::Label crossoverEditLabel;
    int editingCrossover = -1;

    std::array<juce::String, 4> parameterIDs;
    std::array<std::atomic<float>, 4> frequencies;
    int activeHandle = -1;

    static constexpr int maxBins = 1024;
    std::array<float, maxBins> spectrum{};
    int numBins = 0;
    float maxSpectrumFreq = 20000.0f;

    std::array<bool, 5> bandMuted{};
    std::array<bool, 5> bandSoloed{};
    int numBands = 5;

    static constexpr float minFrequency = 20.0f;
    static constexpr float maxFrequency = 20000.0f;
    static constexpr float minDb = -72.0f;
    static constexpr float maxDb = 0.0f;
    static constexpr float minGapHz = 100.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumCrossoverControls)
};
