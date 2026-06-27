# ImageStereoMultiband — Developer Demo Guide

> **Purpose:** This guide is designed for a 30–45 minute technical demo for expert C++, JUCE, and audio plugin developers. Each section includes estimated time, files to show, key talking points, and anticipated questions.

---

## Presentation Index

| # | Section | Time | Files to Show |
|---|---------|------|---------------|
| 1 | Introduction | 2 min | — |
| 2 | Global Architecture | 3 min | `PluginProcessor.h` |
| 3 | LR4 Filter Cascade | 8 min | `MultibandSplitter.h/.cpp`, `Crossover.h` |
| 4 | Per-band Processing | 8 min | `Band.h/.cpp`, `MidSide.h/.cpp` |
| 5 | The Full processBlock | 10 min | `PluginProcessor.cpp` |
| 6 | FFT Analyzer | 5 min | `AudioAnalyzer.h/.cpp` |
| 7 | GUI and Visualization | 5 min | `PluginEditor.h/.cpp`, `Vectorscope.h/.cpp`, `SpectrumCrossoverControls.h/.cpp` |
| 8 | Q&A and Discussion | 5–10 min | — |

---

## 1. Introduction (2 min)

### What to say

"ImageStereoMultiband is a VST3 multiband stereo processing plugin. It takes a stereo signal, splits it into 5 frequency bands using cascaded 4th-order Linkwitz-Riley filters, and allows independent stereo width and gain control on each band. It also includes a real-time FFT analyzer, a Mid/Side vectorscope with correlation meter, and draggable crossover handles directly on the spectrum display."

### Target audience
- C++ developers with JUCE experience
- Audio engineers familiar with VST3 plugins
- People who understand Mid/Side processing, FFT, and LR4 filters

### Prerequisite knowledge (don't explain in detail)
- What an `AudioProcessor` and `AudioProcessorEditor` are in JUCE
- What `AudioProcessorValueTreeState` is
- Basic concepts of IIR filters, Linkwitz-Riley, Mid/Side
- The difference between audio thread and GUI thread

---

## 2. Global Architecture (3 min)

### Files to show
- `PluginProcessor.h`

### Talking points

1. **Open `PluginProcessor.h`** and point out the inheritance: `juce::AudioProcessor`.
2. **Key members:**
   - `MultibandSplitter splitter` — the band splitter
   - `std::array<Band, 5> bands` — 5 band processors
   - `AudioAnalyzer analyzer` — the FFT analyzer
   - `juce::SmoothedValue<float> bypassMix` — bypass crossfade
   - `juce::AudioProcessorValueTreeState apvts` — 25 parameters

3. **Key constant:** `static constexpr int numBands = 5`

4. **Explain ownership:**
   - The `Processor` owns everything (composition, not aggregation)
   - The `Editor` only receives a reference to the `Processor`
   - Bands are stored as `std::array<Band, 5>` — no heap allocation, contiguous memory

5. **Mention `BandScopeBuffer`:**
   - 512-sample circular buffer per band for visualization
   - `friend class ImageStereoMultibandAudioProcessor` — intentional design decision

### Anticipated question

**Q:** "Why `std::array` and not `std::vector` for the bands?"
**A:** Because the number of bands is fixed (5) and known at compile time. `std::array` guarantees contiguous memory, avoids heap allocation, and allows `constexpr`. If we wanted configurable band count at runtime, we'd use `std::vector`.

---

## 3. LR4 Filter Cascade (8 min)

### Files to show
- `MultibandSplitter.h` (complete)
- `MultibandSplitter.cpp` (complete)

### Talking points

#### 3.1 The `CrossoverPair` struct (private nested)

**Show `MultibandSplitter.h:19-74`:**

```cpp
struct CrossoverPair
{
    juce::dsp::LinkwitzRileyFilter<float> lowL, lowR;
    juce::dsp::LinkwitzRileyFilter<float> highL, highR;
    juce::SmoothedValue<float> frequency;
    // ...
};
```

**Explain:**
- 4 filters per crossover (lowL, lowR, highL, highR)
- JUCE's filters are mono — we need one per channel for stereo
- `SmoothedValue` with 50 ms ramp to prevent zipper noise during frequency automation

**Ask the audience:** "Does anyone know why we use 4 filters and not 2?"
**Answer:** Because `LinkwitzRileyFilter` processes a single channel. For stereo we need lowpass L/R and highpass L/R = 4 filters.

#### 3.2 The cascade in `MultibandSplitter::process()`

**Show `MultibandSplitter.cpp:18-70`:**

The most important aspect here is **sample-by-sample processing**:

```cpp
for (int s = 0; s < numSamples; ++s)
{
    for (auto& crossover : crossovers)
        crossover.updateFrequency();

    float l = input.getSample(0, s);
    float r = input.getSample(1, s);

    auto [l0, r0] = crossovers[0].processLow(l, r);
    auto [lH0, rH0] = crossovers[0].processHigh(l, r);
    // ... cascade continues ...
}
```

**Key talking point:** "Each crossover produces a lowpass (current band) and a highpass (feeds the next one). The highpass output of each stage is the input to the next crossover. So crossover N-1 produces band N (lowpass) and the residual feeds the next stage."

**Draw on whiteboard/diagram:**

```
Input → Crossover[0]: lowpass → Band[0]
                       highpass → Crossover[1]: lowpass → Band[1]
                                                 highpass → Crossover[2]: lowpass → Band[2]
                                                           highpass → Crossover[3]: lowpass → Band[3]
                                                                     highpass → Band[4]
```

#### 3.3 Why sample-by-sample and not block-based?

**Talking point:** "Frequency updates are sample-by-sample because `SmoothedValue` needs to be evaluated on every sample to produce a smooth transition. If we updated the frequency once per block (e.g., 512 samples), abrupt automation changes would produce audible stepping."

**Conceptual demo:**
- Linear automation from 500 Hz to 2000 Hz over 100 ms
- At 44100 Hz, that's ~4410 samples of transition
- With block-based processing of 512 samples, only ~8.6 update points → audible steps
- With sample-by-sample, 4410 steps → imperceptible transition

#### 3.4 The standalone `Crossover` class (briefly mention)

**Show `Crossover.h/.cpp`:**

"There's a separate `Crossover` class that is currently unused — `MultibandSplitter` has its own internal `CrossoverPair` version. The `Crossover` class is ready for refactoring if we want to extract the logic to a reusable module."

---

## 4. Per-band Processing (8 min)

### Files to show
- `Band.h`, `Band.cpp`
- `MidSide.h`, `MidSide.cpp`

### Talking points

#### 4.1 The `Band` class

**Show `Band.h` and `Band.cpp`:**

```cpp
class Band {
    Midside midSide;
    juce::SmoothedValue<float> bandGain;   // 20 ms ramp
    juce::SmoothedValue<float> levelGain;  // 20 ms ramp
    bool muted = false;
    bool solo = false;
};
```

**Key concept: two independent SmoothedValues**

"We have `bandGain` (what the user adjusts with the slider) and `levelGain` (controlled by mute/solo logic). Separating them allows mute/solo to have its own 20 ms ramp independent of the gain slider. They are multiplied in `process()`."

**In `Band::process()`:**

```cpp
for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
{
    auto gain = bandGain.getNextValue();
    auto level = levelGain.getNextValue();
    auto totalGain = gain * level;
    left[sample] *= totalGain;
    right[sample] *= totalGain;
}
```

**Ask the audience:** "Why do we multiply instead of adding?"
**Answer:** Because these are linear gain factors. In dB you would add, but linearly it's multiplication. 6 dB + 6 dB = 12 dB → linearly: 2.0 × 2.0 = 4.0.

#### 4.2 The `Midside` class — The heart of stereo width control

**Show `MidSide.h` and complete `MidSide.cpp`:**

```cpp
void Midside::process(juce::AudioBuffer<float>& buffer)
{
    for (int sample = 0; sample < numSamples; ++sample)
    {
        auto left  = buffer.getSample(0, sample);
        auto right = buffer.getSample(1, sample);

        // IIR smoothing
        smoothMid = smoothMid - (0.002f * (smoothMid - midGain));
        smoothSide = smoothSide - (0.002f * (smoothSide - sideGain));

        // Encode
        auto mid  = (left + right) * smoothMid / std::sqrt(2.0f);
        auto side = (left - right) * smoothSide / std::sqrt(2.0f);

        // Decode
        auto newLeft  = (mid + side) / std::sqrt(2.0f);
        auto newRight = (mid - side) / std::sqrt(2.0f);

        buffer.setSample(0, sample, newLeft);
        buffer.setSample(1, sample, newRight);
    }
}
```

**Mathematical explanation in 3 steps:**

1. **Encoding:**
   - `Mid = (L + R) / √2` — what's common between both channels
   - `Side = (L - R) / √2` — the difference (stereo information)

2. **Processing:**
   - `mid *= smoothMid` — always 1.0 in this version
   - `side *= smoothSide` — controls stereo width

3. **Decoding:**
   - `L' = (Mid + Side) / √2`
   - `R' = (Mid - Side) / √2`

**Talking point: The √2 factor**

"The √2 factor guarantees power conservation. If a mono signal enters (L = R = x), Mid is 2x/√2 = x√2, Side is 0, and at the output we get L' = R' = x√2/√2 = x. Without √2, we'd get L' = R' = 2x — double the amplitude."

**Quick whiteboard demo:**
```
L = R = 1 (mono signal)
Mid = (1+1)/√2 = 2/√2 = √2 ≈ 1.414
Side = (1-1)/√2 = 0
L' = (1.414 + 0)/√2 = 1 ✓
R' = (1.414 - 0)/√2 = 1 ✓
```

**Talking point: The IIR smoothing**

```cpp
smoothSide = smoothSide - (0.002f * (smoothSide - sideGain));
```

"This is a first-order IIR filter, also called a one-pole lowpass or leaky integrator. It's equivalent to:
`smoothSide += 0.002 × (sideGain - smoothSide)`

The coefficient 0.002 gives a time constant τ ≈ 1 / (0.002 × 44100) ≈ 11.3 ms. We chose this over `SmoothedValue` because the exponential curve sounds more natural than a linear ramp — it's analogous to an RC circuit."

---

## 5. The Full processBlock (10 min)

### Files to show
- `PluginProcessor.cpp` (lines 244–318)

### Talking points

**Read through `processBlock` code aloud, explaining each step:**

```cpp
void ImageStereoMultibandAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    updateParameters();
```

**Step 1 — `ScopedNoDenormals`:**
"Denormal prevention. When filter coefficients produce very small values (near zero), the x86 FPU enters slow mode. This RAII wrapper configures the FPU to flush denormals to zero."

**Step 2 — `updateParameters()`:**
"Syncs all parameters from APVTS. Note that crossovers are limited with a minimum 100 Hz gap to prevent bands from collapsing:

```cpp
f2 = juce::jlimit(f1 + minBandWidth, 19700.0f, f2);
f3 = juce::jlimit(f2 + minBandWidth, 19800.0f, f3);
f4 = juce::jlimit(f3 + minBandWidth, 19900.0f, f4);
```

The chain dependency ensures f2 > f1 + 100, f3 > f2 + 100, etc."

```cpp
    dryBuffer.makeCopyOf(buffer);
```

**Step 3 — Dry buffer:**
"Copy of the original buffer for the bypass crossfade. Must be done **before** any modifications."

```cpp
    splitter.process(buffer, bandBuffers);
```

**Step 4 — Splitter:**
"Splits into 5 bands. We already saw how this works in section 3. The bands come out in `bandBuffers[0..4]`."

```cpp
    for (int i = 0; i < numBands; ++i)
        bands[i].process(bandBuffers[i]);
```

**Step 5 — Band processing:**
"Each band applies Mid/Side and gain. We already covered this in section 4."

```cpp
    applySoloLogic();
```

**Step 6 — Solo logic:**
"Applies mute/solo via `setLevelTarget()`. If any solo is active, non-soloed bands are silenced. If no solos, it respects mute states."

**Explain the logic:**

```cpp
void applySoloLogic() {
    const bool anySolo = hasAnySolo();
    for (auto& band : bands) {
        float target = 1.0f;
        if (anySolo)
            target = band.isSolo() ? 1.0f : 0.0f;
        else
            target = band.isMuted() ? 0.0f : 1.0f;
        band.setLevelTarget(target);
    }
}
```

"The advantage of using `setLevelTarget` with 20 ms smoothing is that mute/solo produces no clicks — gain ramps up or down gradually."

```cpp
    // Capture scopes for visualization
    for (int b = 0; b < numBands; ++b) {
        // ... per-band circular buffer ...
    }

    buffer.clear();
    for (int band = 0; band < numBands; ++band) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            buffer.addFrom(ch, 0, bandBuffers[band], ch, 0, buffer.getNumSamples());
        }
    }
```

**Steps 7-8 — Band summing:**
"We clear the output buffer and sum all processed bands. Since each band occupies a different spectral range (thanks to the LR4 filters), the sum reconstructs the full signal without artifacts."

**Talking point: Why LR4 sums flat**

"The key property of Linkwitz-Riley: the sum of lowpass and highpass at the same frequency produces a flat magnitude response (0 dB). This is because LR4 filters have 0 dB at the crossover frequency, unlike Butterworth which has -3 dB. So when summing all 5 bands, the frequency response is flat if all gains are at 0 dB."

```cpp
    analyzer.process(buffer);
```

**Step 9 — Analyzer:**
"We run the FFT on the post-gain but pre-bypass buffer. This way the analysis reflects the user's processing."

```cpp
    // Bypass crossfade
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        auto mix = bypassMix.getNextValue();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto wet = buffer.getSample(ch, sample);
            auto dry = dryBuffer.getSample(ch, sample);
            buffer.setSample(ch, sample, wet * mix + dry * (1.0f - mix));
        }
    }
```

**Step 10 — Bypass crossfade:**
"50 ms linear crossfade. `bypassMix` is a `SmoothedValue` that ramps from 1.0 (wet) to 0.0 (dry) or vice versa. This prevents the pop that would occur with abrupt bypass."

**Ask the audience:** "What's the problem with this linear crossfade?"
**Answer:** Linear crossfade is not constant-power — there's a ~3 dB dip at the center. For constant power you'd use `out = wet × √mix + dry × √(1-mix)`. However, for bypass, a temporary 3 dB attenuation is preferable to a pop."

---

## 6. FFT Analyzer (5 min)

### Files to show
- `AudioAnalyzer.h`
- `AudioAnalyzer.cpp`

### Talking points

#### Constants

```cpp
static constexpr int fftOrder = 11;   // 2^11 = 2048
static constexpr int fftSize  = 2048;
static constexpr int numBins  = 1024;
static constexpr int scopeSize = 1024;
```

"2048-point FFT gives us ~21.5 Hz resolution at 44100 Hz. Sufficient for sub-bass, though at 48 kHz it's ~23 Hz/bin."

#### The circular FIFO

```cpp
std::array<float, fftSize> fifo{};
int fifoIndex = 0;
```

"Every sample, we write the mono signal (L+R)/2 into `fifo[fifoIndex++ % 2048]`. It's a circular buffer — when we reach the end, we wrap around, overwriting the oldest data."

#### The hop and Hann window

```cpp
fftHop = fftSize / 4;  // 512 → 75% overlap
```

"Every 512 samples (instead of 2048) we compute an FFT. That's 75% overlap — each FFT shares 1536 samples with the previous one. This smooths the visualization but quadruples the computational cost."

**Hann window:**

```cpp
auto hann = 0.5f * (1.0f - std::cos(2.0f * pi * i / (fftSize - 1)));
```

"The Hann window reduces spectral leakage (artifacts from window boundary discontinuities). ~32 dB side-lobe attenuation."

#### Magnitude to dB calculation

```cpp
// Bins 1 to N/2 - 1
auto real = fftData[2 * i];
auto imag = fftData[2 * i + 1];
auto mag = std::sqrt(real * real + imag * imag) / fftSize;
workingSnapshot.spectrum[i] = Decibels::gainToDecibels(mag, -120.0f);
```

"-120 dB floor prevents log(0)."

#### Atomic communication with GUI

```cpp
// Audio thread:
ready.store(true);

// GUI thread:
if (!ready.exchange(false))
    return false;  // No new data
// Safe copy from workingSnapshot
```

**Key talking point:**

"`ready.exchange(false)` is an atomic RMW (Read-Modify-Write) operation. It returns the previous value and sets false in a single instruction. This is lock-free, doesn't block any thread, and is safe because the audio thread only writes `workingSnapshot` when `ready` is false."

---

## 7. GUI and Visualization (5 min)

### Files to show
- `PluginEditor.cpp` (timerCallback)
- `Vectorscope.cpp` (paint)
- `SpectrumCrossoverControls.cpp` (paint, mouse handlers)

### Talking points

#### 7.1 The 30 Hz timer

```cpp
void ImageStereoMultibandAudioProcessorEditor::timerCallback()
{
    AudioAnalyzer::Snapshot snap;
    if (audioProcessor.getAnalyzer().consumeSnapshot(snap))
    {
        // Push band scopes to vectorscope
        // Calculate correlation
        // Push spectrum
        // Set band states
        spectrumCrossoverControls.repaint();
    }
    vectorscope.tickSmoothing();
}
```

"Every 33 ms, the timer consumes the analyzer's snapshot. `consumeSnapshot` is atomic, so there are no race conditions with the audio thread."

#### 7.2 Correlation calculation

```cpp
double sumL = 0.0, sumR = 0.0, sumLR = 0.0;
for (int i = 0; i < snap.scopeCount; ++i) {
    sumL += l*l; sumR += r*r; sumLR += l*r;
}
auto denom = std::sqrt(sumL * sumR);
correlation = static_cast<float>(sumLR / denom);
```

"Correlation = Σ(L×R) / √(ΣL² × ΣR²). It's the cosine of the angle between the two signal vectors. +1 = perfectly mono, 0 = uncorrelated, -1 = phase inverted."

#### 7.3 M/S Vectorscope

**Show coordinate transformation in `Vectorscope.cpp`:**

```cpp
auto x = cx + bs.side[i] * size * zoomFactor;
auto y = cy - bs.mid[i] * size * zoomFactor;
```

"Side → X, Mid → Y. Y is inverted because screen Y grows downward. zoomFactor ranges from 0.5× to 8.0×."

**Progressive transparency:**
```cpp
auto alpha = juce::jmap<float>(i, 0, bs.count - 1, 0.08f, 0.85f);
```

"Older samples have 8% alpha, newer samples 85%. Creates a persistence effect — like an analog oscilloscope."

#### 7.4 Spectrum with draggable handles

**Show `SpectrumCrossoverControls.cpp`:**

**Logarithmic transformation:**
```cpp
float valueToX(float frequency) {
    auto proportion = (log10(f) - log10(20)) / (log10(20000) - log10(20));
    return graphX + graphWidth * proportion;
}
```

"The human ear perceives frequency logarithmically. That's why we map log10(f) linearly to the X axis."

**Crossover dragging:**
```cpp
void mouseDown() {
    activeHandle = findNearestHandle(event.getPosition());
    if (activeHandle >= 0)
        parameters[activeHandle]->beginChangeGesture();
}
void mouseDrag() {
    setCrossoverFrequency(activeHandle, xToValue(event.x));
}
void mouseUp() {
    if (activeHandle >= 0)
        parameters[activeHandle]->endChangeGesture();
    activeHandle = -1;
}
```

"`beginChangeGesture()` and `endChangeGesture()` are important for DAW automation. Without them, the DAW wouldn't know when a drag starts and ends."

**Constraints:**
```cpp
float getConstrainedFrequency(int index, float frequency) {
    auto lower = (index > 0) ? frequencies[index-1] + 100 : 20;
    auto upper = (index < 3) ? frequencies[index+1] - 100 : 20000;
    return jlimit(lower, upper, frequency);
}
```

"Minimum 100 Hz gap between crossovers. Without this, two bands could collapse at the same frequency."

---

## 8. Q&A and Discussion (5–10 min)

### Anticipated questions with answers

#### "Why didn't you use FIR instead of IIR for the crossovers?"

FIRs (Finite Impulse Response filters) have linear phase, eliminating phase distortion. However, achieving a 24 dB/octave slope with FIR requires hundreds of taps, introducing significant latency and high computational cost. 4th-order Linkwitz-Riley IIR filters give 24 dB/octave with only 2 cascaded Butterworth stages per filter, minimal phase latency, and low CPU cost.

LR4 has the critical property that the sum of lowpass and highpass is flat in magnitude (0 dB at the crossover frequency), something other IIR filters like Butterworth (-3 dB at crossover) don't achieve.

#### "Why FFT of 2048 and not 4096 or 1024?"

2048 is a balance between:
- **Frequency resolution:** 21.5 Hz/bin at 44.1 kHz (sufficient for sub-bass)
- **Temporal resolution:** hop of 512 samples = 11.6 ms (sufficient for transients)
- **Computational cost:** FFT is O(n log n). 4096 would be ~2.2× more expensive but give ~10.75 Hz/bin

For a visualizer, 2048 with 75% overlap is the de facto standard in commercial plugins.

#### "How would you prevent zippering in the crossovers?"

Zippering (stepping noise) when changing cutoff frequency is prevented by the 50 ms `SmoothedValue`. By updating the frequency sample by sample, the transition is smooth. The key is that `updateFrequency()` is called **on every sample**, not once per block.

#### "Why the minimum 100 Hz gap between crossovers?"

Without a minimum gap, two adjacent crossovers could collapse at the same frequency, making a band have zero width (no audio would pass). The 100 Hz gap is somewhat arbitrary but practical — small enough not to limit the user, large enough to prevent null bands.

There's also a perceptual reason: the human ear can distinguish frequency changes of ~3 Hz in the low range, but at crossover points, 100 Hz is imperceptible as a "gap."

#### "How would the plugin scale to 8 bands?"

The design is ready for scaling. You would:
1. Change `numBands` and add more `CrossoverPair` and `Band`
2. The cascade extends naturally — each new crossover takes the highpass from the previous one
3. Number of LR4 filters would be N-1 for N bands
4. Computational cost scales linearly with bands

The main limitation would be GUI space for 8 `BandStrip` components.

#### "Why isn't midGain exposed as a parameter?"

It's a v1.0 design decision to keep the UI simple. The `Midside` class already has `setMidGain()` implemented. Exposing it would require:
1. Adding the parameter in `createParameters()`
2. Reading it in `updateParameters()`
3. Adding a slider in `BandStrip`

It's the highest priority feature on the roadmap.

#### "Why is `CrossoverPair` a private nested struct?"

Encapsulation. `CrossoverPair` is an implementation detail of `MultibandSplitter` — nobody outside that class needs to know there are 4 filters per crossover, or how they are updated. If we wanted to change the implementation in the future (e.g., to FIR filters or biquads), the change would be completely isolated within `MultibandSplitter`.

#### "Why is `BandScopeBuffer::pos` private with `friend`?"

To maintain encapsulation: only `PluginProcessor` can write to `pos`, protecting the circular buffer's integrity. The `friend` avoids making a setter public that nobody else should call. This is a legitimate use of `friend` (controlled access to private implementation).

#### "How does the plugin behave with different buffer sizes?"

`prepareToPlay()` configures all submodules with `spec.maximumBlockSize`. The `bandBuffers` and `dryBuffer` are resized in `prepareToPlay()`. `processBlock()` iterates over the actual received buffer size (`buffer.getNumSamples()`), not a fixed size. This means the plugin adapts to any block size (64, 128, 256, 512, 1024 samples) without issues.

#### "Is there phase latency from the LR4 filters and how is it compensated?"

4th-order Linkwitz-Riley filters have phase latency (non-zero), but not constant group delay. However, since the plugin processes the entire signal within the same block (no external parallel routing), the relative phase between bands remains coherent — all bands pass through the same number of filters (each band goes through some lowpass and highpass stages, but the sum reconstructs the original phase).

The plugin reports 0 samples latency to the DAW because we don't introduce group delay in the traditional sense — all processing is sample-by-sample within the current block.

---

## Appendix: Key Concepts to Mention

| Concept | Where to mention | Brief explanation |
|---------|-----------------|-------------------|
| **RAII** | `ScopedNoDenormals`, `Attachments` | "Resource Acquisition Is Initialization — destructor automatically releases resources" |
| **Composition vs Aggregation** | Architecture | "Processor owns its submodules (composition), Editor only references (aggregation)" |
| **Lock-free** | `std::atomic<bool> ready` | "Inter-thread synchronization without mutexes, safe for real-time" |
| **In-place processing** | `Midside::process()` | "Buffer is modified directly — saves memory and copies" |
| **ODR** | `BandColours.h` with `inline` | "One Definition Rule — inline functions in headers don't violate ODR" |
| **Ramp time** | `SmoothedValue::reset()` | "Time for the smoothed value to go from current to target" |
| **Spectral leakage** | Hann window | "Artifacts from discontinuity at FFT window boundaries" |

---

## Presenter References

- LR4 explanation: https://en.wikipedia.org/wiki/Linkwitz%E2%80%93Riley_filter
- Mid/Side explanation: https://en.wikipedia.org/wiki/M/S_stereo
- FFT / Hann window: https://en.wikipedia.org/wiki/Hann_function
- JUCE API docs: https://docs.juce.com/master/
