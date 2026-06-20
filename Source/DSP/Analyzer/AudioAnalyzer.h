#pragma once

#include <JuceHeader.h>

class AudioAnalyzer
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder;
    static constexpr int numBins  = fftSize / 2;
    static constexpr int scopeSize = 1024;

    struct Snapshot
    {
        float spectrum[numBins]{};
        float scopeLeft[scopeSize]{};
        float scopeRight[scopeSize]{};
        int scopeCount = 0;
    };

    void prepare(double sampleRate, int blockSize);
    void process(const juce::AudioBuffer<float>& buffer);
    void reset();
    bool consumeSnapshot(Snapshot& output);

private:
    void computeFFT();

    juce::dsp::FFT fft{fftOrder};
    std::array<float, fftSize> fifo{};
    int fifoIndex = 0;
    std::array<float, 2 * fftSize> fftData{};

    std::array<float, scopeSize> scopeWriteL{}, scopeWriteR{};
    int scopeWritePos = 0;
    int samplesSinceFFT = 0;
    int fftHop = 0;

    Snapshot workingSnapshot;
    std::atomic<bool> ready{false};
};
