# ImageStereoMultiband — Design Philosophy & Use Cases

> **Purpose:** This document explains why ImageStereoMultiband exists, what problems it solves, and how technical decisions were made. It's aimed at both developers wanting to understand the architecture and music producers wanting to understand what the plugin does and why it works the way it does.

---

## Table of Contents

1. [Why a Multiband Stereo Width Plugin?](#1-why-a-multiband-stereo-width-plugin)
2. [Architecture: The Signal Flow](#2-architecture-the-signal-flow)
3. [Why 4th-Order Linkwitz-Riley?](#3-why-4th-order-linkwitz-riley)
4. [Mid/Side: The Heart of Width Control](#4-midside-the-heart-of-width-control)
5. [Smoothing: Why It Matters](#5-smoothing-why-it-matters)
6. [The GUI as a Tool, Not an Ornament](#6-the-gui-as-a-tool-not-an-ornament)
7. [Real-World Use Cases](#7-real-world-use-cases)
8. [Frequently Asked Questions (Technical & Musical)](#8-frequently-asked-questions-technical--musical)

---

## 1. Why a Multiband Stereo Width Plugin?

### The Musical Problem

In modern music production, stereo width should rarely be uniform across the entire spectrum. Low frequencies sound better centered (mono) to maintain punch and compatibility with mono sound systems (clubs, radios, smartphones). High frequencies typically benefit from wider stereo to create a sense of space and air.

Traditional stereo width plugins apply the same processing to the entire signal. This creates a dilemma: if you widen everything, the lows become unfocused; if you center everything, the highs lose spaciousness.

**ImageStereoMultiband solves this by splitting the spectrum into 5 bands and allowing independent stereo width control on each.**

### The Technical Problem

Splitting audio into bands without artifacts requires special filters. Most simple crossovers introduce phase issues at the crossover region that distort the stereo image. 4th-order Linkwitz-Riley filters (LR4) are specifically designed for this: their sum is perfectly flat in magnitude, meaning there's no loss or emphasis at crossover frequencies when recombining the bands.

---

## 2. Architecture: The Signal Flow

```
Stereo Input → LR4 Splitter → 5 Independent Bands → Sum → Bypass → Output
```

Each stage exists for a reason:

| Stage | What it does | Why |
|-------|-------------|-----|
| **LR4 Splitter** | Divides the signal into 5 bands with cascaded Linkwitz-Riley filters | Phase-coherent splitting; flat response when summed |
| **MidSide per band** | Converts each band to Mid/Side, applies Side gain, converts back | Independent stereo width control per frequency range |
| **Gain per band** | Applies independent volume | Compensate level differences when modifying width |
| **Mute/Solo** | Silences or isolates bands | Diagnose which frequency range you're modifying |
| **Sum** | Recombines the 5 bands into stereo output | LR4 guarantees flat sum (0 dB at crossover) |
| **Bypass** | 50 ms linear crossfade | Prevents pops when enabling/disabling processing |

### Why 5 bands? Isn't that too many?

Five bands with default frequencies 120, 500, 2000, 8000 Hz cover the most musically relevant ranges:

- **Band 1 (sub-bass, < 120 Hz):** Deep lows. Best kept mono.
- **Band 2 (bass, 120–500 Hz):** Body and warmth. Caution when widening.
- **Band 3 (mids, 500–2000 Hz):** Instrument and vocal presence. Extreme width here can mask the lead vocal.
- **Band 4 (upper mids, 2–8 kHz):** Attack and definition. Moderate widening is beneficial.
- **Band 5 (highs, > 8 kHz):** Air and brilliance. Safe to widen aggressively.

Choosing 5 bands (rather than 3 or 8) was a balance between flexibility and usability. Enough to separate critical ranges, not so many as to overwhelm the user.

---

## 3. Why 4th-Order Linkwitz-Riley?

There are many ways to split the spectrum. Common alternatives and their problems:

### Butterworth Filters
The simplest filters. 12 dB/octave slope per stage. **Problem:** At the crossover frequency they have -3 dB. Summing lowpass + highpass produces a +3 dB peak. For a multiband plugin, this means crossover frequencies would have 3 dB more volume — audibly undesirable.

### Linkwitz-Riley Filters (LR4)
Two cascaded 2nd-order Butterworth = 4th-order, 24 dB/octave. **Key property:** At the crossover frequency they have -6 dB, but summing lowpass + highpass gives 0 dB — perfectly flat response.

### FIR Filters
Linear phase, no phase distortion. **Problem:** Achieving 24 dB/octave requires hundreds of taps. High latency (~1000 samples), high CPU cost. Not practical for real-time plugin use.

### Conclusion
LR4 is the industry standard for mastering and mixing crossover plugins for a reason: it provides the necessary slope separation without introducing audible artifacts in the sum, with minimal latency and low CPU cost.

### Implementation: Cascade vs. Parallel

This plugin uses **cascade mode**: each crossover receives the highpass from the previous one.

```
Input → Crossover 0 → lowpass = Band 0
                    → highpass → Crossover 1 → lowpass = Band 1
                                             → highpass → ... → Band 4
```

**Advantage:** Only 4 crossovers are needed for 5 bands (the final highpass is the highest band). Filters are reused — band N passes through fewer filters than band 0, but the total sum remains coherent.

**Disadvantage:** Phase accumulates (each band passes through a different number of filters). However, with LR4 this doesn't affect magnitude, and in practice it's inaudible because all bands are summed within the same processing block.

---

## 4. Mid/Side: The Heart of Width Control

### What is Mid/Side?

Mid/Side (M/S) is a stereo signal representation that separates:

- **Mid (M):** What sounds the same in both channels (L + R) — the monaural information
- **Side (S):** The difference between channels (L - R) — the stereo information

```
Mid  = (L + R) / √2
Side = (L - R) / √2
```

### Why Mid/Side for Width Control?

Because stereo width is controlled by adjusting the Side component's gain:

- **Side gain = 0** → Mid only → Mono
- **Side gain = 1** → Original Side → Original width
- **Side gain > 1** → Amplified Side → Wider signal

This is cleaner than using L/R balance or delays because it preserves phase relationships between channels.

### The Math Behind Power Conservation

The `√2` factor is not arbitrary. It guarantees a mono signal (L = R) passes through unchanged:

```
L = R = 1
Mid = (1+1)/√2 = √2 ≈ 1.414
Side = (1-1)/√2 = 0
L' = (1.414 + 0)/√2 = 1
R' = (1.414 + 0)/√2 = 1
```

Without `√2`, a mono signal would double in amplitude when passing through the encoder/decoder.

### Why isn't midGain exposed?

In the current version, `midGain` is always 1.0. This is a deliberate UI decision: stereo width control is what users need to adjust, and mid gain would add complexity without a clear benefit for the primary use case. The `Midside` class already supports `setMidGain()` for future versions.

---

## 5. Smoothing: Why It Matters

The plugin uses three types of smoothing to prevent audible artifacts:

### a) SmoothedValue on Crossovers (50 ms)

When you drag a crossover, the cutoff frequency doesn't change instantly. The `SmoothedValue` produces a 50 ms linear ramp. Without this, every movement would produce "zipper noise" — audible steps in the cutoff frequency.

**Why 50 ms?** Fast enough to follow real-time mouse movements, slow enough for the transition to be imperceptible. It's the standard in professional plugins.

### b) SmoothedValue on Gain (20 ms)

- `bandGain`: controlled by the gain slider
- `levelGain`: controlled by mute/solo logic

Separating them allows mute/solo to have its own 20 ms ramp independent of the gain slider. Muting isn't instantaneous — there's a 20 ms fade in/out that eliminates pops.

### c) IIR Smoothing on Mid/Side (~11 ms)

Mid/Side uses a first-order IIR filter (one-pole lowpass) instead of `SmoothedValue`:

```cpp
smoothSide += 0.002f * (sideGain - smoothSide)
```

**Why an IIR filter instead of a linear ramp?** Its exponential curve is analogous to an analog RC circuit, and sounds more natural to the human ear than a linear transition. The ~11 ms time constant is fast enough for real-time tracking, slow enough to prevent clicks.

---

## 6. The GUI as a Tool, Not an Ornament

Every visual element exists to solve a specific problem:

### FFT Spectrum with Draggable Crossovers
- **Problem:** Adjusting crossover frequencies blindly requires trial and error
- **Solution:** See the spectrum in real-time and drag crossover lines directly on it
- **Logarithmic scale** because the ear perceives frequency logarithmically
- **Minimum 100 Hz gap** between crossovers to prevent band collapse

### Mid/Side Vectorscope
- **Problem:** The ear doesn't always detect phase or stereo correlation issues
- **Solution:** Real-time visualization of the stereo image in Mid/Side space
- **Y axis = Mid, X axis = Side** — a perfectly mono signal appears as a vertical line, balanced stereo as a circle, out-of-phase content as points outside the circle

### Correlation Meter
- **Problem:** Knowing whether your signal will survive mono summing
- **Solution:** Color-coded bar (+1 green, 0 orange, -1 red) with numeric value

### Mute/Solo
- **Problem:** Not knowing which frequency range you're modifying
- **Solution:** Solo isolates a band to hear it alone; Mute temporarily removes it

### Why muted/dimmed bands fade to 5% opacity
It's immediate visual feedback: you can see which bands are active without reading labels. 5% is enough to still see the band's color but makes it obviously disabled.

### Width: 0–100 slider vs. 0.0–2.0
The internal parameter uses 0–100 (integers) and converts to side gain (0.0–2.0) via `width * 0.02f`. Range 0–100 is more intuitive (0=mono, 50=original, 100=maximum side) than a decimal 0.0–2.0 that needs explanation. The value shows on hover over the slider and can be edited with double-click.

---

## 7. Real-World Use Cases

### Mastering: Centered Lows, Wide Highs

**Problem:** A mix sounds narrow in highs but the bass is too wide, causing phase issues when summed to mono.

**Solution:**
1. Width on band 1 (sub-bass) → 0 (forced mono)
2. Width on band 2 (bass) → 0.3–0.5 (partially centered)
3. Width on bands 4 and 5 (highs) → 1.2–1.5 (widened)

**Result:** Solid, centered lows; wide but coherent highs; zero phase cancellation in mono.

### Mixing: Controlling Width on a Pad Masking the Vocal

**Problem:** A wide synth pad competes with the lead vocal in the center.

**Solution:**
1. Identify the vocal range (300 Hz – 3 kHz)
2. Adjust crossovers to isolate that range in band 2 or 3
3. Reduce Width on that band to 0.5–0.7
4. Keep Width high on the outer bands to preserve the pad's spaciousness

**Result:** The vocal cuts through without needing to increase its volume — just by reducing stereo competition in its range.

### Restoration: Phase Correction on Drums

**Problem:** A drum recording with out-of-phase microphones, sounding hollow.

**Solution:**
1. Observe the vectorscope — if correlation is negative in certain bands, there's cancellation
2. Use Solo to identify which band has the lowest correlation
3. Reduce Width to 0 (mono) on that band to force phase coherence

**Result:** Drums regain punch without losing stereo image in non-problematic bands.

### Sound Design: Extreme Creative Widening

**Problem:** A dramatic effect is needed for a breakdown or transition.

**Solution:**
1. Width on all bands → 1.5–2.0
2. Automate crossovers to create movement in the stereo image
3. Mute specific bands to create "spectral holes"

**Warning:** Extreme widening can cause listening fatigue and phase issues. Check correlation on the vectorscope.

---

## 8. Frequently Asked Questions (Technical & Musical)

### For Music Producers

**Q: Why should I care about per-band stereo width?**
A: Because club sound systems, radios, and smartphones are mono in the low frequencies. If your bass is wide, it will disappear on those systems. With per-band control, you can have centered lows (compatible) and wide highs (impressive).

**Q: When should I use Mute vs. Solo?**
A: Mute removes a band to hear how the rest sounds without it. Solo isolates a band to hear exactly what range you're modifying. Use Solo when adjusting width on a band and you want to hear only that band.

**Q: What does a correlation value of 0.3 mean?**
A: The signal has weak stereo correlation — it may sound wide and spacious, but it's at risk of cancellation when summed to mono. Below 0.3, the color turns orange/red as a warning.

**Q: Does the plugin introduce latency?**
A: No. All processing is sample-by-sample within the current block. The plugin reports 0 samples latency to the DAW.

### For Developers

**Q: Why `std::array` and not `std::vector` for the bands?**
A: The number of bands is fixed (5) and known at compile time. `std::array` guarantees contiguous memory, avoids heap allocation, and allows `constexpr`. For runtime-configurable band count, we'd use `std::vector`.

**Q: Why sample-by-sample processing in the crossovers?**
A: Because `SmoothedValue` needs to be evaluated on every sample for smooth transitions. If we updated frequency once per block (e.g., 512 samples), abrupt automation changes would produce audible stepping. With sample-by-sample and 50 ms ramps, the transition is imperceptible.

**Q: Why is `CrossoverPair` a private nested struct?**
A: Encapsulation. Nobody outside `MultibandSplitter` needs to know there are 4 filters per crossover. If we switch to FIR or biquads in the future, the change stays isolated within that class.

**Q: Why is the bypass crossfade linear instead of constant-power?**
A: Linear crossfade has a ~3 dB dip at center, but for bypass this is preferable to a pop. Constant-power crossfade (`out = wet·√mix + dry·√(1-mix)`) would be technically more correct but adds complexity for marginal benefit — bypass is engaged/disengaged occasionally, not used as a continuous crossfade.

**Q: Why FFT of 2048 and not 4096 or 1024?**
A: 2048 is the standard balance in commercial plugins: ~21.5 Hz/bin resolution (sufficient for sub-bass), hop of 512 samples (11.6 ms, sufficient for transients), and manageable O(n log n) cost. 4096 would be 2.2× more expensive for only double the resolution.

**Q: How is the audio thread synchronized with the GUI?**
A: Via `std::atomic<bool> ready` in `AudioAnalyzer`. The audio thread writes a snapshot and sets `ready = true`. The GUI timer (30 Hz) tries to consume the snapshot with `ready.exchange(false)` — a lock-free atomic RMW (Read-Modify-Write) operation safe for real-time use. If no new data is available (analysis is slower than 30 Hz or audio is stopped), the display simply doesn't update.

**Q: Why does the vectorscope no longer clear when there's no data?**
A: Previously, `clearScopes()` was called on every timer tick without new data, causing flicker during silences or pauses. Now a `signalHoldCounter` retains the image for 5 frames before fading, eliminating flicker without adding latency.

**Q: Why did the width range change from 0.0–2.0 to 0–100?**
A: Usability. A 0–100 slider with default 50 is immediately intuitive (0 = none/center, 50 = midpoint/default, 100 = maximum). The decimal range 0.0–2.0 required the user to know 1.0 = original, 0.5 = half, 2.0 = double. Internally the value converts via `width * 0.02f` to maintain the same effective side gain range (0.0–2.0).
