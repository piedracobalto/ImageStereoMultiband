#include "AudioAnalyzer.h"

void AudioAnalyzer::prepare(double sampleRate, int blockSize)
{
    juce::ignoreUnused(blockSize);
    reset();
    fftHop = fftSize / 4;
}

void AudioAnalyzer::reset()
{
    fifo.fill(0.0f);
    fftData.fill(0.0f);
    scopeWriteL.fill(0.0f);
    scopeWriteR.fill(0.0f);
    fifoIndex = 0;
    scopeWritePos = 0;
    samplesSinceFFT = 0;
    for (auto& s : workingSnapshot.spectrum)
        s = -120.0f;
    workingSnapshot.scopeCount = 0;
    ready.store(false);
}

void AudioAnalyzer::process(const juce::AudioBuffer<float>& buffer)
{
    auto numSamples = buffer.getNumSamples();
    auto* left = buffer.getReadPointer(0);
    auto* right = buffer.getReadPointer(1);

    for (int s = 0; s < numSamples; ++s)
    {
        auto l = left[s];
        auto r = right[s];

        fifo[fifoIndex] = (l + r) * 0.5f;
        fifoIndex = (fifoIndex + 1) % fftSize;
        samplesSinceFFT++;

        if (samplesSinceFFT >= fftHop)
        {
            samplesSinceFFT = 0;
            computeFFT();
        }

        scopeWriteL[scopeWritePos] = l;
        scopeWriteR[scopeWritePos] = r;
        scopeWritePos = (scopeWritePos + 1) % scopeSize;
    }
}

void AudioAnalyzer::computeFFT()
{
    for (int i = 0; i < fftSize; ++i)
    {
        auto hann = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (fftSize - 1)));
        fftData[i] = fifo[i] * hann;
    }
    std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);

    fft.performRealOnlyForwardTransform(fftData.data());

    // Bin 0 (DC)
    workingSnapshot.spectrum[0] = juce::Decibels::gainToDecibels(
        std::abs(fftData[0]) / static_cast<float>(fftSize), -120.0f);

    // Bins 1 to N/2 - 1
    for (int i = 1; i < numBins - 1; ++i)
    {
        auto real = fftData[2 * i];
        auto imag = fftData[2 * i + 1];
        auto mag = std::sqrt(real * real + imag * imag) / static_cast<float>(fftSize);
        workingSnapshot.spectrum[i] = juce::Decibels::gainToDecibels(mag, -120.0f);
    }

    // Bin N/2 (Nyquist)
    workingSnapshot.spectrum[numBins - 1] = juce::Decibels::gainToDecibels(
        std::abs(fftData[1]) / static_cast<float>(fftSize), -120.0f);

    auto oldestIdx = (scopeWritePos + 1) % scopeSize;
    for (int i = 0; i < scopeSize; ++i)
    {
        auto idx = (oldestIdx + i) % scopeSize;
        workingSnapshot.scopeLeft[i] = scopeWriteL[idx];
        workingSnapshot.scopeRight[i] = scopeWriteR[idx];
    }
    workingSnapshot.scopeCount = scopeSize;

    ready.store(true);
}

bool AudioAnalyzer::consumeSnapshot(Snapshot& output)
{
    if (!ready.exchange(false))
        return false;

    std::copy(std::begin(workingSnapshot.spectrum), std::end(workingSnapshot.spectrum), output.spectrum);

    output.scopeCount = workingSnapshot.scopeCount;
    for (int i = 0; i < output.scopeCount; ++i)
    {
        output.scopeLeft[i] = workingSnapshot.scopeLeft[i];
        output.scopeRight[i] = workingSnapshot.scopeRight[i];
    }

    return true;
}
