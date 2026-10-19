# Changelog

## 1.04

### Fixed: drum hits drifting in time and changing tone (the SID "ADSR delay bug")

A user building a drum track in FL Studio (four instances: kick, open hat, closed hat, synth bass) reported that the sound "changed tone and seemed to drift on clock speed". It turned out to be a quirk of the real SID chip, which deSIDuous reproduces because reSIDfp emulates the chip down to individual clock cycles.

#### How the SID envelope counts time

Each voice's envelope is driven by a 15-bit counter that ticks once per chip clock, about a million times a second. Each ADSR setting corresponds to a target count, the *rate period*: attack 0 (2 ms) is about 9 cycles, decay 7 (240 ms) about 313, and the slowest settings about 31,000. Every time the counter **equals** the period, the envelope moves one step and the counter goes back to zero.

#### The flaw

The chip checks for an exact match, not "reached or passed". If the period suddenly drops below where the counter already is, the counter has run past its target and never matches. It has to keep counting all the way to its maximum (32,767), wrap back to zero and count up again before the envelope moves at all.

Drums trigger this naturally:

1. A kick decays with a slow-ish setting, so the counter climbs into the hundreds.
2. The note ends, or a new one starts, and switches to a fast rate with a period of just 9.
3. The counter is already past 9, so the envelope freezes while it counts round the full 15-bit range.

That wait can be up to 32,768 cycles, about **33 ms**. How long it actually is depends on where the counter happened to be, so it changes from hit to hit and looks random. Measured before the fix, hits started anywhere from 0.1 ms to 34 ms late.

#### Why the tone changed too

The pitch sweep and the "hit wave" (the burst of noise at the start of the kick) are timed from the moment the note starts. When the envelope is late, they have already partly played while the voice was still silent. So a late kick loses its click and starts further down its pitch drop. The measured pitch of the kick wandered between 67 and 92 Hz: each hit really did sound different.

#### Why it showed up in FL Studio

Not because of FL Studio, the Mac, or running four instances. A new test (`RenderTest --multi`) runs four instances on four threads for a minute and compares them with the same pattern run one at a time: they differ only at noise level, so the instances don't interfere. A drum pattern simply makes the problem obvious: repeated, rhythmically exact hits make 30 ms of random lateness very audible, while on a pad you'd never notice.

#### How C64 musicians dealt with it

The classic trick is **hard restart**: a couple of video frames before each new note, the music player sets the ADSR to its fastest values with the gate off. That forces the counter down to a small number, so the next attack matches straight away. It works, but costs a little timing and isn't fully reliable. The plugin's **HARD RST** control does the same.

#### The fix

In Layered Poly mode each note gets a whole SID chip of its own. So when the chip chosen for a new note has gone completely silent, deSIDuous now resets it first, which zeroes the envelope counters. The new attack starts exactly on time, every time. It also restarts the noise generator, so hi-hats are consistent too. The chip was silent, so nothing audible is lost.

After the fix, all 118 kick hits in the one-minute test start within 0.1 ms of their notes, with identical pitch.

The one case this can't help is a note starting while the previous one on the same chip is still sounding, such as a mono bass line with overlapping notes. Resetting then would cut that sound off, so the original chip behaviour remains. In the test it was at most about 4 ms late, which isn't audible as drift.

## 1.03

- 256 factory presets (was 64): 64 new originals plus SID interpretations of all 128 General MIDI instruments. The original 64 keep their program numbers.
- Preset menu with a submenu per category; the General MIDI families sit under their own submenu.
- Fixed clicks at the start of notes:
  - Velocity is now a smoothed digital gain instead of rewriting the chip's volume register on every note, which on a real SID causes a DC step (the effect C64 "digi" sample playback relies on).
  - Chips that have been asleep settle silently for 2 ms before they are heard.
  - Chips are only paused once their output is really silent, and all chips are cleared on preset change.
- All presets level-matched automatically (253 of 256 within 1.5 dB).

## 1.02

- Renamed from inSIDious to deSIDuous, with a new logo.
- Builds for Windows, macOS (Apple Silicon and Intel) and Linux.
- User manual, online and as a PDF.

## 1.01

- First release (as inSIDious): 3-voice MOS 6581/8580 emulation via reSIDfp, all waveform combinations, ring mod, hard sync, the SID filter, three voice modes and 64 presets.
