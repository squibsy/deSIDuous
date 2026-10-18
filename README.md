<p align="center"><img src="Resources/logo.png" alt="deSIDuous" width="640"></p>

# deSIDuous

A VST3 instrument built on a cycle-exact emulation of the MOS 6581 and 8580 SID chips (Commodore 64), using **reSIDfp** from libsidplayfp and **JUCE 8**.

The plugin writes real SID registers ($D400 to $D418) at a 1 kHz control rate. Everything you hear (oscillators, combined waveforms, ring mod, hard sync, the ADSR envelopes including their quirks, and the 6581/8580 filters) comes from the emulated chip itself.

## Download

Get the latest version from the **[Releases page](https://github.com/squibsy/deSIDuous/releases/latest)**. Each zip contains only the `deSIDuous.vst3` plugin; you don't need the source code to use it.

| Platform | File | Copy `deSIDuous.vst3` to |
|---|---|---|
| Windows 10/11 (64-bit) | `deSIDuous-<version>-windows-x64-vst3.zip` | `C:\Program Files\Common Files\VST3` |
| macOS 10.13+ (Apple Silicon and Intel) | `deSIDuous-<version>-macos-universal-vst3.zip` | `/Library/Audio/Plug-Ins/VST3` or `~/Library/Audio/Plug-Ins/VST3` |
| Linux (64-bit) | `deSIDuous-<version>-linux-x64-vst3.zip` | `~/.vst3` |

Then rescan plugins in your DAW. deSIDuous appears as an instrument from "Bananalytics".

📖 **[Read the user manual](docs/MANUAL.md)** ([PDF](docs/deSIDuous-Manual.pdf)) for a guide to every control, sound design recipes and the preset list.

**macOS:** the plugin isn't notarised by Apple, so macOS may block it the first time. After copying it, run this once in Terminal (adjust the path if you used `~/Library`):

```bash
xattr -dr com.apple.quarantine /Library/Audio/Plug-Ins/VST3/deSIDuous.vst3
```

**Linux:** built on Ubuntu 22.04. It should work on any distribution from about 2022 onwards.

## Features

**Per voice (x3)**
- Triangle, Saw, Pulse and Noise toggles. Any combination is allowed, so all 15 waveform combinations are available.
- Ring mod and hard sync, with the hardware routing: V1 is modulated by V3, V2 by V1, V3 by V2. A voice that is switched off keeps its oscillator running, so it can still act as a silent sync or ring source.
- Test bit, filter routing, key tracking (off gives a fixed pitch for drums), and per-voice arpeggio enable.
- 12-bit pulse width with a PWM sweep (rate and depth).
- 4-bit ADSR. The values are the data sheet register values, with rates from 2 ms to 8 s (attack) and 6 ms to 24 s (decay and release).
- "Hit wave" (a different waveform for the first few ms) and a pitch sweep, the classic SID tricks for drums and plucks.

**Filter (data sheet registers)**
- 11-bit cutoff, 4-bit resonance, LP / BP / HP (combinable), and 3OFF.
- A software envelope, key tracking, velocity and LFO all modulate the cutoff register.
- Filter Bias sets the reSIDfp filter curve. Real chips vary a lot from one to the next.

**Chip and global**
- 6581 or 8580 model, PAL or NTSC clock, and combined-waveform strength (Average, Weak, Strong).
- Voice modes:
  - **Layered Poly**: every note gets its own SID chip with all 3 voices, up to 8 chips.
  - **Paraphonic**: one chip, one note per voice, 3 notes maximum.
  - **MIDI Ch 1-3**: MIDI channel N drives voice N.
- Glide and legato, pitch bend range, mod wheel to vibrato, sustain pedal.
- Hard restart (works around the SID ADSR delay bug) and an oscillator reset (test-bit pulse) on each note.
- LFO (triangle, sine, square, saw or random) to vibrato with delay, cutoff and pulse width.
- Arpeggio: chord shapes, or cycling through held notes. The step length is set in video frames (50 Hz or 60 Hz), like a C64 tracker.

**UI**: C64-style panels, an oscilloscope, a live register monitor and an on-screen keyboard.

## Presets

There are 256 factory presets, available through the preset menu (grouped into submenus by category) or your DAW's program list. Categories: Bass, Lead, Pad, Arp, Keys, Drums, FX and a Waveform showcase, plus SID interpretations of all 128 General MIDI instruments.

All presets were rendered and level-matched automatically: `RenderTest` measures each one and `tools/level-presets.ps1` writes the gain corrections in `Source/PresetTrims.inc`. Short-term RMS is about -18 dBFS for single-note patches and -16 dBFS for chords, with peaks below -3 dBFS. `tools/audition.ps1` joins the renders into one MP3 per category for listening.

## CPU

Cycle-exact emulation runs each chip at about 1 MHz. On the development machine, one chip costs about 8 to 10% of a CPU core at 48 kHz. Only chips that are sounding are clocked; idle chips cost nothing. Polyphony defaults to 4. Pads use 6, which is about 55% of a core with all 6 notes held.

## Building (Windows)

You need Visual Studio 2022 Build Tools (C++ workload) and CMake 3.22 or later. JUCE and libsidplayfp are git submodules in `libs/`, pinned to JUCE 8.0.15 and libsidplayfp 2.16.1. Clone with `--recursive`, or run `git submodule update --init` after a normal clone.

```bash
git clone --recursive https://github.com/squibsy/deSIDuous.git
cd deSIDuous
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target deSIDuous_VST3
```

The output is `build/deSIDuous_artefacts/Release/VST3/deSIDuous.vst3`. To install it, copy that folder into `C:\Program Files\Common Files\VST3`.

### Tests

```bash
cmake --build build --config Release --target RenderTest NoiseTest
build/RenderTest_artefacts/Release/RenderTest.exe build/deSIDuous_artefacts/Release/VST3/deSIDuous.vst3 renders
```

`RenderTest` loads the built VST3 the way a DAW would. It renders every preset to `renders/*.wav`, prints peak level, short-term RMS, brightness, tail length and CPU per preset, and checks the voice modes and the state save/restore. Pass `--ui <program>` to open the editor in a window instead. `NoiseTest` drives reSIDfp directly, as a reference. `tools/screenshots.ps1` regenerates the manual's screenshots from the built plugin (Windows).

## Source layout

| File | Purpose |
|---|---|
| `Source/SidEngine.*` | SID chip wrapper (reSIDfp, register shadow, demand-driven output FIFO) and the engine: voice allocation, gate sequencing, modulation, register writes |
| `Source/Params.*` | Parameter layout and data sheet tables |
| `Source/Presets.*` | Factory presets (builder style) |
| `Source/PluginProcessor.*` | JUCE processor, programs, state |
| `Source/PluginEditor.*`, `Source/SidLookAndFeel.*` | UI |
| `config/` | Generated `siddefs-fp.h` and `sidcxx11.h` for building reSIDfp without autotools |

## Licence

reSIDfp is GPL v2 or later, and JUCE 8 is used under its AGPLv3 open-source licence. This plugin is therefore distributed under the **AGPLv3**. Releasing it closed-source would need a commercial JUCE licence and a replacement for reSIDfp.
