# ImageStereoMultiband

**Plugin VST3 de procesamiento estéreo multibanda** — Divide la señal de audio en 5 bandas de frecuencia con control independiente de ancho estéreo, ganancia, mute y solo.

Desarrollado por **Pedro Cuomo Ghio** — Versión **1.0.0**

---

## Características

- **5 bandas de frecuencia** con cruces configurables (120, 500, 2000, 8000 Hz por defecto)
- **Control de ancho estéreo** por banda (procesamiento Mid/Side)
- **Ganancia independiente** por banda (-24 dB a +24 dB)
- **Mute/Solo** por banda con lógica inteligente
- **Visualizador FFT** en tiempo real con curva espectral
- **Vectoscopio Mid/Side** con medidor de correlación de fase
- **Bypass suave** con crossfade de 50 ms entre wet y dry (señal limpia al bypassear)
- **Persistencia de estado** — el DAW guarda y restaura toda la configuración

---

## Instalación

### Usuarios

**Windows**

1. Descarga `ImageStereoMultiband_Installer.exe`
2. Ejecuta como administrador
3. El instalador coloca el plugin en:
   ```
   C:\Program Files\Common Files\VST3\ImageStereoMultiband.vst3\
   ```
4. Escanea la carpeta VST3 desde tu DAW (Cubase, Reaper, FL Studio, Studio One, etc.)

**macOS**

1. Descarga `ImageStereoMultiband.dmg`
2. Abre el DMG y ejecuta el paquete instalador
3. Consulta [HowToInstall.txt](HowToInstall.txt) para instrucciones detalladas

### Desarrolladores — Compilar desde fuente

#### Requisitos

- **Visual Studio 2026** (o compatible con proyectos VC++ v143)
- **JUCE 8.0.12** — ubicado en `../../juce-8.0.12-windows/JUCE/` relativo al proyecto
- **Inno Setup 6** (opcional, para generar instalador)

#### Pasos

```bash
# 1. Abrir la solución
Builds/VisualStudio2026/ImageStereoMultiband.sln

# 2. Seleccionar configuración Release, plataforma x64
# 3. Compilar → genera:
#    - VST3: x64/Release/VST3/ImageStereoMultiband.vst3/
#    - Standalone: x64/Release/Standalone Plugin/ImageStereoMultiband.exe

# 4. (Opcional) Generar instalador
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer.iss
```

---

## Guía de uso

### Panel de control por banda

Cada banda tiene cuatro controles:

| Control | Tipo | Descripción |
|---------|------|-------------|
| **Width** | Slider horizontal | Ancho estéreo (0 = mid/mono, 50 = original, 100 = side). Muestra valor al pasar el ratón; doble click para editar |
| **Gain** | Slider vertical | Ganancia de -24 dB a +24 dB. Valor editable con doble click |
| **M** | Botón | Silencia la banda |
| **S** | Botón | Aísla la banda (solo) |

### Espectro y cruces

El panel superior muestra el **espectro de frecuencia** en tiempo real. Las líneas verticales con indicador cuadrado son las **frecuencias de cruce** entre bandas. Puedes arrastrarlas con el ratón o hacer doble click sobre el valor numérico para editarlo — los cruces adyacentes mantienen un gap mínimo de 100 Hz.

### Vectoscopio

Visualiza la imagen estéreo en espacio **Mid/Side**:

- **Eje Y** = Mid (mono), **Eje X** = Side (estéreo)
- Señal mono → puntos en vertical
- Señal estéreo balanceada → círculo
- Señal out-of-phase → puntos fuera del círculo de referencia

Botones: **+/-** para zoom, **W/C** para alternar entre blanco y colores por banda. La banda muteada o sin solo (cuando hay otro solo activo) se atenúa al 5% de opacidad. Botones **−** (remove) y **+** (add) en el panel inferior permiten añadir/quitar bandas en tiempo real.

El medidor inferior muestra la **correlación de fase** (-1 a +1):
- Verde > 0.7 — Bien correlacionado
- Amarillo 0.3–0.7 — Parcialmente correlacionado
- Naranja -0.3–0.3 — No correlacionado / ancho
- Rojo < -0.3 — Fuera de fase (puede causar cancelación al sumar a mono)

---

## Estructura del proyecto

```
Source/
├── PluginProcessor.h/.cpp     → Procesamiento de audio (entrada del DAW)
├── PluginEditor.h/.cpp        → Interfaz gráfica (timer + layout)
├── Parameters/                → [stubs] IDs y layout de parámetros
├── DSP/
│   ├── MultibandSplitter/     → Divide en 5 bandas (filtros LR4 en cascada)
│   ├── Band/                  → Procesamiento de una banda (MidSide + Gain)
│   ├── Midside/               → Codificación/decodificación Mid/Side
│   ├── Crossover/             → Par de filtros Linkwitz-Riley 4to orden
│   ├── Analyzer/              → FFT 2048 + osciloscopio en tiempo real
│   ├── Gain/                  → [stub] Sin implementar
│   └── DryWet/                → [stub] Sin implementar
└── GUI/
    ├── LookAndFeel/           → Tema oscuro + rotary slider personalizado
    ├── Components/
    │   ├── HeaderBar/         → Título + botón de bypass
    │   ├── BandStrip/         → Controles por banda
    │   ├── SpectrumCrossoverControls/ → Espectro + handles de cruce
    │   └── Vectorscope/       → Visualizador Mid/Side + correlación
    └── BandColours.h          → Paleta de colores (5 bandas)
```

---

## Captura

![ImageStereoMultiband](Assets/screenshot.png)

---

## Licencia

Distribuido bajo licencia MIT. Ver [LICENSE](LICENSE) para más detalles.
