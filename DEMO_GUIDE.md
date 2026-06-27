# ImageStereoMultiband — Guía de presentación para desarrolladores

> **Propósito:** Esta guía está diseñada para una demo técnica de 30–45 minutos frente a desarrolladores expertos en C++, JUCE y plugins de audio. Cada sección incluye el tiempo estimado, los archivos a mostrar, los talking points clave y las preguntas frecuentes anticipadas.

---

## Índice de la presentación

| # | Sección | Tiempo | Archivos a mostrar |
|---|---------|--------|-------------------|
| 1 | Introducción | 2 min | — |
| 2 | Arquitectura global | 3 min | `PluginProcessor.h` |
| 3 | Cadena de filtros LR4 | 8 min | `MultibandSplitter.h/.cpp`, `Crossover.h` |
| 4 | Procesamiento por banda | 8 min | `Band.h/.cpp`, `MidSide.h/.cpp` |
| 5 | El processBlock completo | 10 min | `PluginProcessor.cpp` |
| 6 | Analizador FFT | 5 min | `AudioAnalyzer.h/.cpp` |
| 7 | GUI y visualización | 5 min | `PluginEditor.h/.cpp`, `Vectorscope.h/.cpp`, `SpectrumCrossoverControls.h/.cpp` |
| 8 | Preguntas y discusión | 5–10 min | — |

---

## 1. Introducción (2 min)

### Qué vas a decir

"ImageStereoMultiband es un plugin VST3 de procesamiento estéreo multibanda. Toma una señal estéreo, la divide en 5 bandas de frecuencia usando filtros Linkwitz-Riley de 4to orden en cascada, y permite controlar el ancho estéreo y la ganancia de cada banda de forma independiente. También incluye un analizador FFT en tiempo real, un vectoscopio Mid/Side con medidor de correlación, y la capacidad de arrastrar los crossovers directamente sobre el espectro."

### Público objetivo
- Desarrolladores C++ con experiencia en JUCE
- Ingenieros de audio familiarizados con plugins VST3
- Personas que entienden procesamiento Mid/Side, FFT, y filtros LR4

### Conceptos que deben conocer de antemano (no explicar en detalle)
- Qué es un `AudioProcessor` y `AudioProcessorEditor` en JUCE
- Qué es `AudioProcessorValueTreeState`
- Conceptos básicos de filtros IIR, Linkwitz-Riley, Mid/Side
- Diferencia entre hilo de audio y hilo de GUI

---

## 2. Arquitectura global (3 min)

### Archivos a mostrar
- `PluginProcessor.h`

### Talking points

1. **Abrir `PluginProcessor.h`** y señalar la herencia: `juce::AudioProcessor`.
2. **Miembros principales:**
   - `MultibandSplitter splitter` — el divisor de bandas
   - `std::array<Band, 5> bands` — 5 procesadores de banda
   - `AudioAnalyzer analyzer` — el analizador FFT
   - `juce::SmoothedValue<float> bypassMix` — crossfade de bypass
   - `juce::AudioProcessorValueTreeState apvts` — 25 parámetros

3. **Constante clave:** `static constexpr int numBands = 5`

4. **Explicar el ownership:**
   - El `Processor` posee todo (composición, no agregación)
   - El `Editor` recibe solo una referencia al `Processor`
   - Las bandas se almacenan como `std::array<Band, 5>` — sin heap allocation, contiguo en memoria

5. **Mencionar `BandScopeBuffer`:**
   - Buffer circular de 512 samples por banda para visualización
   - `friend class ImageStereoMultibandAudioProcessor` — decisión de diseño intencional

### Pregunta anticipada

**P:** "¿Por qué `std::array` y no `std::vector` para las bandas?"
**R:** Porque el número de bandas es fijo (5) y conocido en tiempo de compilación. `std::array` da ubicación contigua garantizada, evita heap allocation, y permite `constexpr`. Si quisiéramos hacer el número de bandas configurable en runtime, usaríamos `std::vector`.

---

## 3. Cadena de filtros LR4 (8 min)

### Archivos a mostrar
- `MultibandSplitter.h` (completo)
- `MultibandSplitter.cpp` (completo)

### Talking points

#### 3.1 La estructura `CrossoverPair` (privada anidada)

**Mostrar `MultibandSplitter.h:19-74`:**

```cpp
struct CrossoverPair
{
    juce::dsp::LinkwitzRileyFilter<float> lowL, lowR;
    juce::dsp::LinkwitzRileyFilter<float> highL, highR;
    juce::SmoothedValue<float> frequency;
    // ...
};
```

**Explicar:**
- 4 filtros por crossover (lowL, lowR, highL, highR)
- Los filtros de JUCE son monoaurales — necesitamos uno por canal para estéreo
- `SmoothedValue` con 50 ms de rampa para evitar zipper noise en automatización de frecuencia

**Pregunta para la audiencia:** "¿Alguien sabe por qué usamos 4 filtros y no 2?"
**Respuesta:** Porque `LinkwitzRileyFilter` procesa un solo canal. Para estéreo necesitamos lowpass L/R y highpass L/R = 4 filtros.

#### 3.2 La cascada en `MultibandSplitter::process()`

**Mostrar `MultibandSplitter.cpp:18-70`:**

Lo más importante aquí es **el procesamiento sample a sample**:

```cpp
for (int s = 0; s < numSamples; ++s)
{
    for (auto& crossover : crossovers)
        crossover.updateFrequency();

    float l = input.getSample(0, s);
    float r = input.getSample(1, s);

    auto [l0, r0] = crossovers[0].processLow(l, r);
    auto [lH0, rH0] = crossovers[0].processHigh(l, r);
    // ... cascada continúa ...
}
```

**Talking point clave:** "Cada crossover produce un lowpass (banda actual) y un highpass (alimenta el siguiente). La salida del highpass de cada etapa es la entrada del siguiente crossover. Así, el crossover N-1 produce la banda N (lowpass) y el residual pasa a la siguiente etapa."

**Dibujar en pizarra/diagrama:**

```
Input → Crossover[0]: lowpass → Band[0]
                       highpass → Crossover[1]: lowpass → Band[1]
                                                 highpass → Crossover[2]: lowpass → Band[2]
                                                           highpass → Crossover[3]: lowpass → Band[3]
                                                                     highpass → Band[4]
```

#### 3.3 ¿Por qué sample a sample y no por bloques?

**Talking point:** "La actualización de frecuencia es sample a sample porque `SmoothedValue` necesita ser evaluado en cada sample para producir una transición suave. Si actualizáramos la frecuencia una vez por bloque (ej. 512 samples), los cambios abruptos en la automatización producirían escalones audibles."

**Demostración conceptual:**
- Automatización lineal de 500 Hz a 2000 Hz en 100 ms
- A 44100 Hz, son ~4410 samples de transición
- Con procesamiento por bloques de 512, solo tendríamos ~8.6 puntos de actualización → escalones audibles
- Con sample a sample, 4410 pasos → transición imperceptible

#### 3.4 La clase `Crossover` independiente (mencionar brevemente)

**Mostrar `Crossover.h/.cpp`:**

"Existe una clase `Crossover` separada que no se usa actualmente — el `MultibandSplitter` tiene su propia versión interna `CrossoverPair`. La clase `Crossover` está lista para refactorizarse si queremos extraer la lógica a un módulo reutilizable."

---

## 4. Procesamiento por banda (8 min)

### Archivos a mostrar
- `Band.h`, `Band.cpp`
- `MidSide.h`, `MidSide.cpp`

### Talking points

#### 4.1 La clase `Band`

**Mostrar `Band.h` y `Band.cpp`:**

```cpp
class Band {
    Midside midSide;
    juce::SmoothedValue<float> bandGain;   // 20 ms ramp
    juce::SmoothedValue<float> levelGain;  // 20 ms ramp
    bool muted = false;
    bool solo = false;
};
```

**Concepto clave: dos SmoothedValues independientes**

"Tenemos `bandGain` (lo que mueve el usuario con el slider) y `levelGain` (controlado por la lógica de mute/solo). Separarlos permite que el mute/solo tenga su propia rampa de 20 ms independiente del slider de ganancia. Se multiplican en `process()`."

**En `Band::process()`:**

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

**Pregunta para la audiencia:** "¿Por qué multiplicamos en lugar de sumar?"
**Respuesta:** Porque son factores de ganancia lineal. En dB sumarías, pero en lineal es multiplicación. 6 dB + 6 dB = 12 dB → en lineal: 2.0 × 2.0 = 4.0.

#### 4.2 La clase `Midside` — El corazón del control de ancho estéreo

**Mostrar `MidSide.h` y `MidSide.cpp` completo:**

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

**Explicación matemática en 3 pasos:**

1. **Codificación:**
   - `Mid = (L + R) / √2` — lo común entre ambos canales
   - `Side = (L - R) / √2` — la diferencia (información estéreo)

2. **Procesamiento:**
   - `mid *= smoothMid` — siempre 1.0 en esta versión
   - `side *= smoothSide` — controla el ancho estéreo (width)

3. **Decodificación:**
   - `L' = (Mid + Side) / √2`
   - `R' = (Mid - Side) / √2`

**Talking point: El factor √2**

"El factor √2 garantiza conservación de potencia. Si entra una señal mono (L = R = x), el Mid es 2x/√2 = x√2, el Side es 0, y a la salida tenemos L' = R' = x√2/√2 = x. Sin el √2, tendríamos L' = R' = 2x — el doble de amplitud."

**Demostración rápida en pizarra:**
```
L = R = 1 (señal mono)
Mid = (1+1)/√2 = 2/√2 = √2 ≈ 1.414
Side = (1-1)/√2 = 0
L' = (1.414 + 0)/√2 = 1 ✓
R' = (1.414 - 0)/√2 = 1 ✓
```

**Talking point: El IIR smoothing**

```cpp
smoothSide = smoothSide - (0.002f * (smoothSide - sideGain));
```

"Esto es un filtro IIR de primer orden, también llamado one-pole lowpass o leaky integrator. Es equivalente a:
`smoothSide += 0.002 × (sideGain - smoothSide)`

El coeficiente 0.002 produce una constante de tiempo τ ≈ 1 / (0.002 × 44100) ≈ 11.3 ms. Elegimos este enfoque en lugar de `SmoothedValue` porque la curva exponencial suena más natural que una rampa lineal — es análoga a un circuito RC."

---

## 5. El processBlock completo (10 min)

### Archivos a mostrar
- `PluginProcessor.cpp` (líneas 244–318)

### Talking points

**Leer el código de `processBlock` en voz alta, explicando cada paso:**

```cpp
void ImageStereoMultibandAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    updateParameters();
```

**Paso 1 — `ScopedNoDenormals`:**
"Prevención de denormales. Cuando los coeficientes de los filtros producen valores muy pequeños (cercanos a cero), la FPU x86 entra en modo lento. Este RAII wrapper configura el estado de la FPU para que trate los denormales como cero."

**Paso 2 — `updateParameters()`:**
"Sincroniza todos los parámetros desde el APVTS. Notar que los crossovers se limitan con un gap mínimo de 100 Hz para evitar que dos bandas colapsen:

```cpp
f2 = juce::jlimit(f1 + minBandWidth, 19700.0f, f2);
f3 = juce::jlimit(f2 + minBandWidth, 19800.0f, f3);
f4 = juce::jlimit(f3 + minBandWidth, 19900.0f, f4);
```

La dependencia en cadena asegura que f2 > f1 + 100, f3 > f2 + 100, etc."

```cpp
    dryBuffer.makeCopyOf(buffer);
```

**Paso 3 — Dry buffer:**
"Copia del buffer original para el crossfade de bypass. Tiene que hacerse **antes** de cualquier modificación."

```cpp
    splitter.process(buffer, bandBuffers);
```

**Paso 4 — Splitter:**
"Divide en 5 bandas. Ya vimos cómo funciona en la sección 3. Las bandas salen en `bandBuffers[0..4]`."

```cpp
    for (int i = 0; i < numBands; ++i)
        bands[i].process(bandBuffers[i]);
```

**Paso 5 — Band processing:**
"Cada banda aplica Mid/Side y ganancia. Ya vimos esto en la sección 4."

```cpp
    applySoloLogic();
```

**Paso 6 — Solo logic:**
"Aplica mute/solo mediante `setLevelTarget()`. Si hay algún solo activo, las bandas sin solo se silencian. Si no hay solos, respeta los mute."

**Explicar la lógica:**

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

"La ventaja de usar `setLevelTarget` con suavizado de 20 ms es que el mute/solo no produce clicks — la ganancia sube o baja gradualmente."

```cpp
    // Capturar scopes para visualización
    for (int b = 0; b < numBands; ++b) {
        // ... buffer circular por banda ...
    }

    buffer.clear();
    for (int band = 0; band < numBands; ++band) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            buffer.addFrom(ch, 0, bandBuffers[band], ch, 0, buffer.getNumSamples());
        }
    }
```

**Paso 7-8 — Suma de bandas:**
"Limpiamos el buffer de salida y sumamos todas las bandas procesadas. Como cada banda ocupa un rango espectral diferente (gracias a los filtros LR4), la suma reconstruye la señal completa sin artefactos."

**Talking point: Por qué LR4 suma plano**

"La propiedad clave de Linkwitz-Riley: la suma del lowpass y highpass en la misma frecuencia produce una respuesta en magnitud plana (0 dB). Esto es porque los filtros LR4 tienen 0 dB en la frecuencia de cruce, a diferencia de Butterworth que tiene -3 dB. Así que al sumar las 5 bandas, la respuesta en frecuencia es plana si todas las ganancias están a 0 dB."

```cpp
    analyzer.process(buffer);
```

**Paso 9 — Analyzer:**
"Procesamos el FFT sobre el buffer post-ganancia pero pre-bypass. Así el análisis refleja el procesamiento del usuario."

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

**Paso 10 — Bypass crossfade:**
"Crossfade lineal de 50 ms. `bypassMix` es un `SmoothedValue` que rampa de 1.0 (wet) a 0.0 (dry) o viceversa. Esto evita el pop que ocurriría con un bypass abrupto."

**Pregunta para la audiencia:** "¿Qué problema tiene este crossfade lineal?"
**Respuesta:** El crossfade lineal no es de potencia constante — hay una atenuación de ~3 dB en el centro del crossfade. Para potencia constante se usaría `out = wet × √mix + dry × √(1-mix)`. Sin embargo, para bypass, la atenuación temporal de 3 dB es preferible a un pop.

---

## 6. Analizador FFT (5 min)

### Archivos a mostrar
- `AudioAnalyzer.h`
- `AudioAnalyzer.cpp`

### Talking points

#### Constantes

```cpp
static constexpr int fftOrder = 11;   // 2^11 = 2048
static constexpr int fftSize  = 2048;
static constexpr int numBins  = 1024;
static constexpr int scopeSize = 1024;
```

"FFT de 2048 puntos nos da ~21.5 Hz de resolución a 44100 Hz. Suficiente para ver sub-bajos, aunque en 48 kHz serían ~23 Hz/bin."

#### El FIFO circular

```cpp
std::array<float, fftSize> fifo{};
int fifoIndex = 0;
```

"En cada sample, escribimos la señal mono (L+R)/2 en `fifo[fifoIndex++ % 2048]`. Es un buffer circular — cuando llegamos al final, volvemos al principio sobrescribiendo los datos más viejos."

#### El hop y la ventana Hann

```cpp
fftHop = fftSize / 4;  // 512 → 75% overlap
```

"Cada 512 samples (en lugar de 2048) hacemos una FFT. Esto es 75% de overlap — cada FFT comparte 1536 samples con la anterior. Esto suaviza la visualización pero cuadruplica el costo computacional."

**Ventana Hann:**

```cpp
auto hann = 0.5f * (1.0f - std::cos(2.0f * pi * i / (fftSize - 1)));
```

"La ventana Hann reduce el leakage espectral (artefactos de discontinuidad en los bordes). Atenuación de lóbulos laterales de ~32 dB."

#### Cálculo de magnitud a dB

```cpp
// Bins 1 a N/2 - 1
auto real = fftData[2 * i];
auto imag = fftData[2 * i + 1];
auto mag = std::sqrt(real * real + imag * imag) / fftSize;
workingSnapshot.spectrum[i] = Decibels::gainToDecibels(mag, -120.0f);
```

"Piso de -120 dB para evitar log(0)."

#### Comunicación atómica con la GUI

```cpp
// Audio thread:
ready.store(true);

// GUI thread:
if (!ready.exchange(false))
    return false;  // No new data
// Safe copy from workingSnapshot
```

**Talking point clave:**

"`ready.exchange(false)` es una operación atómica RMW (Read-Modify-Write). Retorna el valor anterior y establece false en una instrucción. Esto es lock-free, no bloquea ningún hilo, y es seguro porque el hilo de audio solo escribe `workingSnapshot` cuando `ready` es false."

---

## 7. GUI y visualización (5 min)

### Archivos a mostrar
- `PluginEditor.cpp` (timerCallback)
- `Vectorscope.cpp` (paint)
- `SpectrumCrossoverControls.cpp` (paint, mouse handlers)

### Talking points

#### 7.1 El timer a 30 Hz

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

"Cada 33 ms, el timer consume el snapshot del analizador. El `consumeSnapshot` es atómico, así que no hay condiciones de carrera con el hilo de audio."

#### 7.2 El cálculo de correlación

```cpp
double sumL = 0.0, sumR = 0.0, sumLR = 0.0;
for (int i = 0; i < snap.scopeCount; ++i) {
    sumL += l*l; sumR += r*r; sumLR += l*r;
}
auto denom = std::sqrt(sumL * sumR);
correlation = static_cast<float>(sumLR / denom);
```

"Correlación = Σ(L×R) / √(ΣL² × ΣR²). Es el coseno del ángulo entre los dos vectores de señal. +1 = mono perfecto, 0 = no correlacionado, -1 = fase invertida."

#### 7.3 Vectorscope M/S

**Mostrar la transformación de coordenadas en `Vectorscope.cpp`:**

```cpp
auto x = cx + bs.side[i] * size * zoomFactor;
auto y = cy - bs.mid[i] * size * zoomFactor;
```

"Side → X, Mid → Y. Y está invertido porque en pantalla Y crece hacia abajo. El zoomFactor va de 0.5× a 8.0×."

**Transparencia progresiva:**
```cpp
auto alpha = juce::jmap<float>(i, 0, bs.count - 1, 0.08f, 0.85f);
```

"Los samples más viejos tienen alpha 8%, los más nuevos 85%. Crea un efecto de persistencia — como un osciloscopio analógico."

#### 7.4 Spectrum con handles arrastrables

**Mostrar `SpectrumCrossoverControls.cpp`:**

**Transformación logarítmica:**
```cpp
float valueToX(float frequency) {
    auto proportion = (log10(f) - log10(20)) / (log10(20000) - log10(20));
    return graphX + graphWidth * proportion;
}
```

"El oído humano percibe la frecuencia en escala logarítmica. Por eso mapeamos log10(f) linealmente al eje X."

**Drag de crossovers:**
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

"`beginChangeGesture()` y `endChangeGesture()` son importantes para la automatización del DAW. Sin ellos, el DAW no sabría cuándo empieza y termina un movimiento."

**Constraints:**
```cpp
float getConstrainedFrequency(int index, float frequency) {
    auto lower = (index > 0) ? frequencies[index-1] + 100 : 20;
    auto upper = (index < 3) ? frequencies[index+1] - 100 : 20000;
    return jlimit(lower, upper, frequency);
}
```

"Gap mínimo de 100 Hz entre cruces. Sin esto, dos bandas podrían colapsar en la misma frecuencia."

---

## 8. Preguntas y discusión (5–10 min)

### Preguntas frecuentes anticipadas con respuestas

#### "¿Por qué no usaste FIR en lugar de IIR para los crossovers?"

Los FIR (Filtros de Respuesta al Impulso Finita) tienen fase lineal, lo que elimina la distorsión de fase. Sin embargo, para obtener una pendiente de 24 dB/octava con FIR necesitarías cientos de taps, lo que introduce latencia significativa y alto costo computacional. Los filtros Linkwitz-Riley IIR de 4to orden dan 24 dB/octava con solo 2 etapas Butterworth en cascada por filtro, latencia mínima (fase, no grupo), y bajo costo de CPU.

El LR4 tiene la propiedad crucial de que la suma del lowpass y highpass es plana en magnitud (0 dB en la frecuencia de cruce), algo que no logran otros filtros IIR como Butterworth (-3 dB en el cruce).

#### "¿Por qué FFT de 2048 y no 4096 o 1024?"

2048 es un balance entre:
- **Resolución frecuencial:** 21.5 Hz/bin a 44.1 kHz (suficiente para ver sub-bajos)
- **Resolución temporal:** hop de 512 samples = 11.6 ms (suficiente para ver transitorios)
- **Costo computacional:** La FFT es O(n log n). 4096 sería ~2.2× más cara pero daría ~10.75 Hz/bin

Para un visualizador, 2048 con 75% overlap es el estándar de facto en plugins comerciales.

#### "¿Cómo evitarías el zippering en los crossovers?"

El zippering (ruido de escalones) al cambiar la frecuencia de corte se evita con el `SmoothedValue` de 50 ms. Al actualizar la frecuencia sample a sample, la transición es suave. La clave está en que `updateFrequency()` se llama **en cada sample**, no una vez por bloque.

#### "¿Por qué el gap mínimo de 100 Hz entre crossovers?"

Sin un gap mínimo, dos crossovers adyacentes podrían colapsar en la misma frecuencia, haciendo que una banda tenga ancho cero (no pasaría audio). El gap de 100 Hz es arbitrario pero práctico — suficientemente pequeño para no limitar al usuario, suficientemente grande para evitar bandas nulas.

También hay una razón perceptual: el oído humano puede distinguir cambios de frecuencia de ~3 Hz en el rango bajo, pero en cruces de banda, 100 Hz es imperceptible como "hueco".

#### "¿Cómo escalaría el plugin a 8 bandas?"

El diseño está preparado para escalar. Habría que:
1. Cambiar `numBands` y agregar más `CrossoverPair` y `Band`
2. La cascada se extiende naturalmente — cada nuevo crossover toma el highpass del anterior
3. El número de filtros LR4 sería N-1 para N bandas
4. El costo computacional escala linealmente con las bandas

La limitación principal sería el espacio en la GUI para 8 `BandStrip`.

#### "¿Por qué midGain no está expuesto como parámetro?"

Es una decisión de diseño de v1.0 para mantener la UI simple. La clase `Midside` ya tiene `setMidGain()` implementado. Exponerlo requeriría:
1. Agregar el parámetro en `createParameters()`
2. Leerlo en `updateParameters()`
3. Agregar un slider en `BandStrip`

Es la feature de mayor prioridad en la hoja de ruta.

#### "¿Por qué `CrossoverPair` es una estructura anidada privada?"

Por encapsulación. `CrossoverPair` es un detalle de implementación del `MultibandSplitter` — nadie fuera de esa clase necesita saber que existen 4 filtros por crossover, ni cómo se actualizan. Si en el futuro quisiéramos cambiar la implementación (ej. a filtros FIR o biquads), el cambio estaría completamente aislado dentro de `MultibandSplitter`.

#### "¿Por qué `BandScopeBuffer::pos` es privado con `friend`?"

Para mantener encapsulación: solo `PluginProcessor` puede escribir en `pos`, protegiendo la integridad del buffer circular. El `friend` evita hacer público un setter que nadie más debería llamar. Es un uso legítimo de `friend` (acceso controlado a implementación privada).

#### "¿Cómo se comporta el plugin con buffers de distinto tamaño?"

`prepareToPlay()` configura todos los submódulos con `spec.maximumBlockSize`. Los `bandBuffers` y `dryBuffer` se redimensionan en `prepareToPlay()`. El `processBlock()` itera sobre el tamaño real del buffer recibido (`buffer.getNumSamples()`), no un tamaño fijo. Esto significa que el plugin se adapta a cualquier tamaño de bloque (64, 128, 256, 512, 1024 samples) sin problemas.

#### "¿Hay latencia de fase en los LR4 y cómo se compensa?"

Los filtros Linkwitz-Riley de 4to orden tienen latencia de fase (no cero), pero no latencia de grupo constante. Sin embargo, como el plugin procesa la señal completa dentro del mismo bloque (no hay routing paralelo externo), la fase relativa entre bandas se mantiene coherente — todas las bandas pasan por la misma cantidad de filtros (cada banda pasa por algunos lowpass y highpass, pero la suma reconstruye la fase original).

El plugin reporta 0 samples de latencia al DAW porque no introducimos latencia de grupo en el sentido tradicional — todo el procesamiento es sample a sample dentro del bloque actual.

---

## Apéndice: Conceptos clave para mencionar

| Concepto | Dónde mencionarlo | Explicación breve |
|----------|-------------------|-------------------|
| **RAII** | `ScopedNoDenormals`, `Attachments` | "Resource Acquisition Is Initialization — el destructor libera automáticamente" |
| **Composición vs Agregación** | Arquitectura | "El Processor posee sus submódulos (composición), el Editor solo referencia (agregación)" |
| **Lock-free** | `std::atomic<bool> ready` | "Sincronización entre hilos sin mutex, seguro para tiempo real" |
| **In-place processing** | `Midside::process()` | "El buffer se modifica directamente — ahorra memoria y copias" |
| **ODR** | `BandColours.h` con `inline` | "One Definition Rule — las funciones inline en headers no violan ODR" |
| **Ramp time** | `SmoothedValue::reset()` | "Tiempo que tarda el valor suavizado en ir del valor actual al target" |
| **Spectral leakage** | Ventana Hann | "Artefactos de discontinuidad en los bordes de la ventana FFT" |

---

## Referencias para el presentador

- Para explicar LR4: https://en.wikipedia.org/wiki/Linkwitz%E2%80%93Riley_filter
- Para explicar Mid/Side: https://en.wikipedia.org/wiki/M/S_stereo
- Para explicar FFT: https://en.wikipedia.org/wiki/Hann_function
- API de JUCE: https://docs.juce.com/master/
