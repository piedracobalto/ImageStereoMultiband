#pragma once

#include <JuceHeader.h>

class Vectorscope : public juce::Component
{
public:
    Vectorscope();

    void pushScopeData(const float* left, const float* right, int count);

    void paint(juce::Graphics& g) override;

private:
    static constexpr int maxSamples = 1024;
    std::array<float, maxSamples> scopeMid{};
    std::array<float, maxSamples> scopeSide{};
    int scopeCount = 0;
};
