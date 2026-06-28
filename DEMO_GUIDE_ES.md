# ImageStereoMultiband — Filosofía de diseño y casos de uso

> **Propósito:** Este documento explica por qué existe ImageStereoMultiband, qué problemas resuelve, y cómo se tomaron las decisiones técnicas. Está dirigido tanto a desarrolladores que quieren entender la arquitectura como a productores musicales que buscan entender qué hace el plugin y por qué funciona así.

---

## Índice

1. [¿Por qué un plugin de ancho estéreo multibanda?](#1-por-qué-un-plugin-de-ancho-estéreo-multibanda)
2. [Arquitectura: el flujo de la señal](#2-arquitectura-el-flujo-de-la-señal)
3. [¿Por qué Linkwitz-Riley de 4to orden?](#3-por-qué-linkwitz-riley-de-4to-orden)
4. [Mid/Side: el corazón del control de ancho](#4-midside-el-corazón-del-control-de-ancho)
5. [Suavizado: por qué es importante](#5-suavizado-por-qué-es-importante)
6. [La GUI como herramienta, no como adorno](#6-la-gui-como-herramienta-no-como-adorno)
7. [Casos de uso reales](#7-casos-de-uso-reales)
8. [Preguntas frecuentes (técnicas y musicales)](#8-preguntas-frecuentes-técnicas-y-musicales)

---

## 1. ¿Por qué un plugin de ancho estéreo multibanda?

### El problema musical

En la producción musical moderna, el ancho estéreo rara vez debería ser uniforme en todo el espectro. Los graves suenan mejor centrados (mono) para mantener pegada y compatibilidad con sistemas de sonido mono (clubes, radios, smartphones). Los agudos suelen beneficiarse de un estéreo más amplio para crear sensación de espacio y brillo.

Los plugins de ancho estéreo tradicionales (como los clásicos "width" o "stereo imager") aplican el mismo procesamiento a toda la señal. Esto crea un dilema: si ensanchas todo el espectro, los graves se desenfocan; si centras todo, los agudos pierden amplitud.

**ImageStereoMultiband resuelve esto dividiendo el espectro en 5 bandas y permitiendo controlar el ancho estéreo de cada una por separado.**

### El problema técnico

Dividir el audio en bandas sin artefactos requiere filtros especiales. La mayoría de los crossovers simples introducen problemas de fase en la región de cruce que distorsionan la imagen estéreo. Los filtros Linkwitz-Riley de 4to orden (LR4) están diseñados específicamente para esto: su suma es perfectamente plana en magnitud, lo que significa que al juntar las bandas de vuelta no hay pérdida ni énfasis en las frecuencias de cruce.

---

## 2. Arquitectura: el flujo de la señal

```
Entrada estéreo → Splitter LR4 → 5 bandas independientes → Suma → Bypass → Salida
```

Cada etapa tiene una razón de ser:

| Etapa | ¿Qué hace? | ¿Por qué? |
|-------|-----------|-----------|
| **Splitter LR4** | Divide la señal en 5 bandas con filtros Linkwitz-Riley en cascada | Sin artefactos de fase en los cruces; respuesta plana al sumar |
| **MidSide por banda** | Convierte cada banda a Mid/Side, aplica ganancia Side, reconvierte | Control de ancho estéreo independiente por rango frecuencial |
| **Ganancia por banda** | Aplica volumen independiente | Compensar diferencias de nivel al modificar el ancho estéreo |
| **Mute/Solo** | Silencia o aísla bandas | Diagnosticar qué rango frecuencial estás modificando |
| **Suma** | Recombina las 5 bandas en una salida estéreo | Los LR4 garantizan suma plana (0 dB en cruce) |
| **Bypass** | Crossfade lineal de 50 ms entre señal procesada (wet) y original (dry) | Evita pops; al activar pasa la señal limpia sin procesar. Un overlay oscuro cubre la interfaz y los visuales se detienen |

### ¿Por qué 5 bandas? ¿No son demasiadas?

Cinco bandas con frecuencias por defecto 120, 500, 2000, 8000 Hz cubren los rangos musicales más relevantes:

- **Banda 1 (sub-bajos, < 120 Hz):** Graves profundos. Mejor en mono.
- **Banda 2 (bajos, 120–500 Hz):** Cuerpo y calidez. Precaución al ensanchar.
- **Banda 3 (medios, 500–2000 Hz):** Presencia de instrumentos y voces. El ancho extremo aquí puede enmascarar la voz principal.
- **Banda 4 (medios-agudos, 2–8 kHz):** Ataque y definición. Beneficioso ensanchar moderadamente.
- **Banda 5 (agudos, > 8 kHz):** Aire y brillo. Seguro de ensanchar.

Elegir 5 bandas (en vez de 3 o 8) fue un balance entre flexibilidad y usabilidad. Suficientes para separar rangos críticos, no tantas como para abrumar al usuario.

---

## 3. ¿Por qué Linkwitz-Riley de 4to orden?

Hay muchas formas de dividir el espectro. Las alternativas comunes y sus problemas:

### Filtros Butterworth
Son los más simples. Pendiente de 12 dB/octava por etapa. **Problema:** En la frecuencia de cruce tienen -3 dB. Al sumar lowpass + highpass se obtiene un pico de +3 dB. Para un multibanda, esto significa que las frecuencias de cruce tendrían 3 dB más de volumen — audible como énfasis no deseado.

### Filtros Linkwitz-Riley (LR4)
Son dos Butterworth de 2do orden en cascada = 4to orden, 24 dB/octava. **Propiedad clave:** En la frecuencia de cruce tienen -6 dB, pero al sumar lowpass + highpass se obtiene 0 dB — respuesta perfectamente plana.

### FIR
Fase lineal, sin distorsión de fase. **Problema:** Para 24 dB/octava necesitas cientos de taps. Latencia alta (~1000 samples), CPU costoso. En un plugin donde el usuario espera respuesta en tiempo real, no es práctico.

### Conclusión
LR4 es el estándar de la industria para crossovers en plugins de mastering y mezcla por una razón: da la pendiente necesaria para separar bandas sin introducir artefactos audibles en la suma, con latencia mínima y bajo costo computacional.

### Implementación: cascade vs. parallel

Este plugin usa **cascada**: cada crossover recibe el highpass del anterior.

```
Entrada → Crossover 0 → lowpass = Banda 0
                      → highpass → Crossover 1 → lowpass = Banda 1
                                               → highpass → ... → Banda 4
```

**Ventaja:** Solo se necesitan 4 crossovers para 5 bandas (el último highpass es la banda más aguda). Los filtros son reutilizados — la banda N pasa por menos filtros que la banda 0, pero la suma total es coherente.

**Desventaja:** La fase se acumula (cada banda pasa por distinta cantidad de filtros). Sin embargo, en LR4 esto no afecta la magnitud y en la práctica no es audible porque todas las bandas se suman dentro del mismo bloque de procesamiento.

---

## 4. Mid/Side: el corazón del control de ancho

### ¿Qué es Mid/Side?

Mid/Side (M/S) es una representación de la señal estéreo que separa:

- **Mid (M):** Lo que suena igual en ambos canales (L + R) — la información monoaural
- **Side (S):** La diferencia entre canales (L - R) — la información estéreo

```
Mid  = (L + R) / √2
Side = (L - R) / √2
```

### ¿Por qué Mid/Side para control de ancho estéreo?

Porque el ancho estéreo se controla ajustando la ganancia del componente Side:

- **Side gain = 0** → Solo Mid → Mono
- **Side gain = 1** → Side original → Ancho original
- **Side gain > 1** → Side amplificado → Señal más ancha

Esto es más limpio que usar balance L/R o retardos porque preserva la relación de fase entre canales.

### La matemática detrás de la conservación de potencia

El factor `√2` no es arbitrario. Garantiza que una señal mono (L = R) pase sin cambio de nivel:

```
L = R = 1
Mid = (1+1)/√2 = √2 ≈ 1.414
Side = (1-1)/√2 = 0
L' = (1.414 + 0)/√2 = 1
R' = (1.414 + 0)/√2 = 1
```

Sin el `√2`, una señal mono se duplicaría en amplitud al pasar por el codificador/decodificador.

### ¿Por qué midGain no está expuesto?

En la versión actual, `midGain` es siempre 1.0. No exponerlo fue una decisión consciente de UI: el control de ancho estéreo es lo que los usuarios necesitan ajustar, y el mid gain añadiría complejidad sin un beneficio claro en el caso de uso principal. La clase `Midside` ya soporta `setMidGain()` para futuras versiones.

---

## 5. Suavizado: por qué es importante

El plugin usa tres tipos de suavizado para evitar artefactos audibles:

### a) SmoothedValue en crossovers (50 ms)

Cuando arrastras un crossover, la frecuencia de corte no cambia instantáneamente. El `SmoothedValue` produce una rampa lineal de 50 ms. Sin esto, cada movimiento produciría un "zipper noise" — escalones audibles en la frecuencia de corte.

**¿Por qué 50 ms?** Suficientemente rápido para seguir movimientos del ratón en tiempo real, suficientemente lento para que la transición sea imperceptible. Es el estándar en plugins profesionales.

### b) SmoothedValue en ganancia (20 ms)

- `bandGain`: lo que controla el slider de ganancia
- `levelGain`: lo que controla el mute/solo

Separarlos permite que mute/solo tenga su propia rampa independiente del slider. El mute no es instantáneo — hay 20 ms de fade in/out que eliminan los pops.

### c) IIR smoothing en Mid/Side (~11 ms)

El Mid/Side usa un filtro IIR de primer orden (one-pole lowpass) en lugar de `SmoothedValue`:

```cpp
smoothSide += 0.002f * (sideGain - smoothSide)
```

**¿Por qué un filtro IIR en vez de rampa lineal?** Porque su curva exponencial es análoga a un circuito RC analógico, y suena más natural al oído humano que una transición lineal. La constante de tiempo de ~11 ms es suficientemente rápida para seguimiento en tiempo real, suficientemente lenta para evitar clicks.

---

## 6. La GUI como herramienta, no como adorno

Cada elemento visual existe para resolver un problema específico:

### Espectro FFT con crossovers arrastrables
- **Problema:** Ajustar frecuencias de cruce a ciegas requiere prueba y error
- **Solución:** Ver el espectro en tiempo real y arrastrar líneas de cruce directamente sobre él
- **Escala logarítmica** porque el oído percibe la frecuencia así
- **Gap mínimo de 100 Hz** entre cruces para evitar que dos bandas colapsen

### Vectoscopio Mid/Side
- **Problema:** El oído no siempre detecta problemas de fase o correlación estéreo
- **Solución:** Visualización en tiempo real de la imagen estéreo en espacio Mid/Side
- **Eje Y = Mid, Eje X = Side** — una señal perfectamente mono aparece como una línea vertical, una estéreo balanceada como un círculo, una fuera de fase como puntos fuera del círculo

### Medidor de correlación
- **Problema:** Saber si tu señal sobrevive a la suma a mono
- **Solución:** Barra con código de colores (+1 verde, 0 naranja, -1 rojo) con el valor numérico

### Mute/Solo
- **Problema:** No saber qué rango frecuencial estás modificando
- **Solución:** Solo aísla una banda para escucharla en solitario; Mute la elimina temporalmente

### Por qué las bandas silenciadas se atenúan al 5% de opacidad
Es feedback visual inmediato: ves qué bandas están activas sin tener que leer etiquetas. El 5% es suficiente para ver el color de la banda pero es obvio que está desactivada.

### Width: slider 0–100 vs. 0.0–2.0
El parámetro interno usa un rango de 0–100 (enteros) y se convierte a side gain (0.0–2.0) mediante `width * 0.02f`. El rango 0–100 es más intuitivo para el usuario (0=mono, 50=original, 100=máximo side) que un valor decimal de 0.0 a 2.0 que requiere explicación. El valor se muestra al pasar el ratón sobre el slider, y se puede editar con doble click.

---

## 7. Casos de uso reales

### Masterización: graves centrados, agudos amplios

**Problema:** Una mezcla suena estrecha en agudos pero el bajo está demasiado ancho, causando problemas de fase al sumar a mono.

**Solución:** 
1. Width en banda 1 (sub-bajos) → 0 (mono forzado)
2. Width en banda 2 (bajos) → 0.3–0.5 (parcialmente centrado)
3. Width en bandas 4 y 5 (agudos) → 1.2–1.5 (ensanchado)

**Resultado:** Graves sólidos y centrados, agudos amplios pero coherentes, nula cancelación de fase al sumar a mono.

### Mezcla: control de anchura en un pad que enmascara la voz

**Problema:** Un sintetizador pad muy ancho compite con la voz principal en el centro.

**Solución:**
1. Identificar el rango de la voz (300 Hz – 3 kHz)
2. Ajustar crossovers para aislar ese rango en banda 2 o 3
3. Reducir Width en esa banda a 0.5–0.7
4. Mantener Width alto en las bandas extremas para preservar la amplitud del pad

**Resultado:** La voz se destaca sin necesidad de subir su volumen, simplemente reduciendo la competencia estéreo en su rango.

### Restauración: corrección de fase en baterías

**Problema:** Una grabación de batería con micrófonos fuera de fase, sonido hueco.

**Solución:**
1. Observar el vectoscopio — si la correlación es negativa en ciertas bandas, hay cancelación
2. Usar Solo para identificar qué banda tiene la correlación más baja
3. Reducir Width a 0 (mono) en esa banda para forzar coherencia de fase

**Resultado:** La batería recupera pegada sin perder imagen estéreo en las bandas no problemáticas.

### Diseño sonoro: efecto creativo de anchura extrema

**Problema:** Se necesita un efecto dramático para un breakdown o transición.

**Solución:**
1. Width en todas las bandas → 1.5–2.0
2. Automatizar crossovers para crear movimiento en la imagen estéreo
3. Mutear bandas específicas para crear "huecos espectrales"

**Advertencia:** El ensanchamiento extremo puede causar fatiga auditiva y problemas de fase. Verificar la correlación en el vectoscopio.

---

## 8. Preguntas frecuentes (técnicas y musicales)

### Para productores musicales

**P: ¿Por qué debería preocuparme por el ancho estéreo por banda?**
R: Porque los sistemas de sonido en clubes, radios y smartphones son mono en los graves. Si tu bajo está muy ancho, desaparecerá en esos sistemas. Con control por banda, puedes tener graves centrados (compatibles) y agudos amplios (impactantes).

**P: ¿Cuándo debería usar Mute vs. Solo?**
R: Mute elimina una banda para escuchar cómo suena el resto sin ella. Solo aísla una banda para escuchar exactamente qué rango estás modificando. Usa Solo cuando quieras ajustar el width de una banda y quieras oír solo esa banda.

**P: ¿Qué significa un valor de correlación de 0.3 en el vectoscopio?**
R: Que la señal tiene poca correlación estéreo — puede sonar ancha y espaciosa, pero también corre riesgo de cancelarse al sumar a mono. Por debajo de 0.3, el color se vuelve naranja/rojo como advertencia.

**P: ¿El plugin introduce latencia?**
R: No. Todo el procesamiento es sample a sample dentro del bloque actual. El plugin reporta 0 samples de latencia al DAW.

### Para desarrolladores

**P: ¿Por qué `std::array` y no `std::vector` para las bandas?**
R: El número de bandas es fijo (5) y conocido en compilación. `std::array` garantiza memoria contigua, evita heap allocation, y permite `constexpr`. Para un número configurable en runtime, usaríamos `std::vector`.

**P: ¿Por qué procesamiento sample a sample en los crossovers?**
R: Porque `SmoothedValue` necesita evaluarse en cada sample para producir transiciones suaves. Si actualizáramos la frecuencia una vez por bloque (ej. 512 samples), los cambios abruptos en automatización producirían escalones audibles. Con sample a sample y 50 ms de rampa, la transición es imperceptible.

**P: ¿Por qué `CrossoverPair` es una estructura privada anidada?**
R: Encapsulación. Nadie fuera de `MultibandSplitter` necesita saber que existen 4 filtros por crossover. Si en el futuro cambiamos a FIR o biquads, el cambio está aislado dentro de esa clase.

**P: ¿Qué pasa cuando activo el bypass?**
R: El plugin deja de procesar el audio y la señal pasa limpia, sin ninguna alteración. El crossfade lineal de 50 ms evita pops en la transición. Además, todos los visuales (espectro FFT, vectoscopio) se detienen y aparece un overlay oscuro con el texto "BYPASSED" para indicar que el procesamiento está desactivado. El overlay no cubre el header bar, así que puedes volver a clickear el botón de bypass para desactivarlo.

**P: ¿Por qué FFT de 2048 y no 4096 o 1024?**
R: 2048 es el balance estándar en plugins comerciales: ~21.5 Hz/bin de resolución (suficiente para sub-bajos), hop de 512 samples (11.6 ms, suficiente para transitorios), y costo computacional O(n log n) manejable. 4096 sería 2.2× más caro con solo el doble de resolución.

**P: ¿Cómo se sincroniza el hilo de audio con la GUI?**
R: Mediante `std::atomic<bool> ready` en el `AudioAnalyzer`. El hilo de audio escribe un snapshot y marca `ready = true`. El timer de la GUI (30 Hz) intenta consumir el snapshot con `ready.exchange(false)` — una operación atómica RMW (Read-Modify-Write) que es lock-free y segura para tiempo real. Si no hay datos nuevos (el análisis es más lento que 30 Hz o el audio está detenido), simplemente no se actualiza la visualización.

**P: ¿Por qué el vectorscopio ya no se limpia cuando no hay datos?**
R: En versiones anteriores, `clearScopes()` se llamaba en cada tick del timer sin datos nuevos, lo que causaba parpadeo en silencios o pausas. Ahora se usa un `signalHoldCounter`: la imagen se retiene durante 5 frames antes de desvanecer, eliminando el parpadeo sin añadir latencia.

**P: ¿Por qué el rango de width cambió de 0.0–2.0 a 0–100?**
R: Usabilidad. Un slider de 0 a 100 con valor por defecto en 50 es inmediatamente intuitivo (0 = nada/centro, 50 = mitad/default, 100 = máximo). El rango decimal 0.0–2.0 requería que el usuario supiera que 1.0 = original, 0.5 = mitad, 2.0 = doble. Internamente el valor se convierte con `width * 0.02f` para mantener el mismo rango efectivo de side gain (0.0–2.0).
