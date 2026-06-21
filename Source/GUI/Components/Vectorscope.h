#pragma once

#include <JuceHeader.h>
#include "../BandColours.h"

class Vectorscope : public juce::Component
{
public:
    Vectorscope();
    ~Vectorscope() override;

    void pushScopeData(const float* left, const float* right, int count);
    void pushBandScope(int bandIndex, const float* left, const float* right, int count);
    void setCorrelation(float corr) { targetCorrelation = corr; }
    void setHasSignal(bool s) { hasSignal = s; }
    void tickSmoothing();
    void clearScopes();

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    static constexpr int maxSamples = 1024;
    static constexpr int numBands = 5;

    struct BandScope
    {
        std::array<float, maxSamples> mid{};
        std::array<float, maxSamples> side{};
        int count = 0;
    };

    std::array<BandScope, numBands> bandScopes;
    int anyScopeCount = 0;

    float zoomFactor = 6.5f;
    bool multiColor = false;

    juce::TextButton zoomInBtn;
    juce::TextButton zoomOutBtn;
    juce::TextButton colorBtn;

    float targetCorrelation = 0.0f;
    float displayCorrelation = 0.0f;
    bool hasSignal = false;
};
