// Offline test host: loads the built deSIDuous.vst3, renders every factory preset
// to a WAV file and prints level / brightness / CPU statistics.
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <cstdio>
#include <thread>
#include "WinCapture.h"

#ifndef _WIN32
 #define _popen popen
 #define _pclose pclose
#endif

namespace
{
    constexpr double sr = 48000.0;
    constexpr int block = 256;

    struct Stats { float peakDb, rmsDb, zcrHz, tailSec; double cpuPercent; };

    float toDb (float x) { return juce::Decibels::gainToDecibels (x, -120.0f); }

    juce::AudioProcessorParameter* findParam (juce::AudioPluginInstance& p, const juce::String& name)
    {
        for (auto* prm : p.getParameters())
            if (prm->getName (100) == name)
                return prm;
        return nullptr;
    }

    Stats render (juce::AudioPluginInstance& plugin, const std::vector<std::pair<int, int>>& notes, // (channel, note)
                  double holdSec, double totalSec, const juce::File& wavFile)
    {
        const int total = (int) (totalSec * sr);
        const int holdEnd = (int) (holdSec * sr);
        juce::AudioBuffer<float> out (2, total);
        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer midi;

        plugin.reset();
        const auto t0 = juce::Time::getHighResolutionTicks();

        for (int pos = 0; pos < total; pos += block)
        {
            const int n = juce::jmin (block, total - pos);
            midi.clear();
            if (pos == 0)
                for (auto [ch, nn] : notes) midi.addEvent (juce::MidiMessage::noteOn (ch, nn, (juce::uint8) 100), 0);
            if (pos <= holdEnd && holdEnd < pos + n)
                for (auto [ch, nn] : notes) midi.addEvent (juce::MidiMessage::noteOff (ch, nn), holdEnd - pos);

            buf.setSize (2, n, false, false, true);
            buf.clear();
            plugin.processBlock (buf, midi);
            for (int c = 0; c < 2; ++c)
                out.copyFrom (c, pos, buf, c, 0, n);
        }

        const double secs = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0);

        // stats on the left channel over the held portion
        const float* d = out.getReadPointer (0);
        double sumSq = 0.0; float peak = 0.0f; int crossings = 0;
        for (int i = 0; i < total; ++i)
        {
            peak = juce::jmax (peak, std::abs (d[i]));
            if (i < holdEnd) sumSq += (double) d[i] * d[i];
            if (i > 0 && i < holdEnd && (d[i - 1] <= 0.0f) != (d[i] <= 0.0f)) ++crossings;
        }
        int lastLoud = 0;
        for (int i = 0; i < total; ++i)
            if (std::abs (d[i]) > 0.001f) lastLoud = i;

        // short-term loudness: loudest 100 ms window
        const int win = (int) (0.1 * sr), hop = (int) (0.01 * sr);
        double loudest = 0.0;
        for (int st = 0; st + win <= total; st += hop)
        {
            double acc = 0.0;
            for (int i = st; i < st + win; ++i) acc += (double) d[i] * d[i];
            loudest = juce::jmax (loudest, acc / win);
        }

        Stats s;
        s.peakDb = toDb (peak);
        s.rmsDb = toDb ((float) std::sqrt (loudest));
        s.zcrHz = (float) (crossings * 0.5 / holdSec);
        s.tailSec = (float) ((lastLoud - holdEnd) / sr);
        s.cpuPercent = 100.0 * secs / totalSec;

        wavFile.deleteFile();
        juce::WavAudioFormat wav;
        if (auto stream = wavFile.createOutputStream())
            if (auto writer = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (stream.get(), sr, 2, 24, {}, 0)))
            {
                stream.release();
                writer->writeFromAudioSampleBuffer (out, 0, total);
            }
        return s;
    }
}

namespace
{
    // ---- Demo recording: plays tools/demo/demo-script.txt in real time and records the
    // audio, while ffmpeg (Desktop Duplication, Windows) records the plugin window.
    struct DemoEvent
    {
        enum Type { NoteOff = 0, Program = 1, NoteOn = 2 } type;
        double t;
        int a = 0, b = 0, ch = 1;
    };

    struct DemoSweep { double t0, t1; juce::String param; float v0, v1; juce::AudioProcessorParameter* p = nullptr; };

    void writeWav (const juce::AudioBuffer<float>& buffer, int numSamples, const juce::File& file)
    {
        file.deleteFile();
        juce::WavAudioFormat wav;
        if (auto stream = file.createOutputStream())
            if (auto writer = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (stream.get(), sr, 2, 24, {}, 0)))
            {
                stream.release();
                writer->writeFromAudioSampleBuffer (buffer, 0, numSamples);
            }
    }

    int runDemo (juce::AudioPluginInstance& plugin, const juce::File& script, const juce::File& outDir, const juce::String& ffmpeg)
    {
        std::vector<DemoEvent> events;
        std::vector<DemoSweep> sweeps;
        double endTime = 0.0;

        for (auto line : juce::StringArray::fromLines (script.loadFileAsString()))
        {
            line = line.trim();
            if (line.isEmpty() || line.startsWith ("#"))
                continue;
            auto tok = juce::StringArray::fromTokens (line, " \t", "");
            tok.removeEmptyStrings();
            const auto& cmd = tok[0];

            if (cmd == "preset")
                events.push_back ({ DemoEvent::Program, tok[1].getDoubleValue(), tok[2].getIntValue() });
            else if (cmd == "note")
            {
                const double t = tok[1].getDoubleValue(), dur = tok[2].getDoubleValue();
                const int note = tok[3].getIntValue();
                const int vel = tok.size() > 4 ? tok[4].getIntValue() : 100;
                const int ch = tok.size() > 5 ? tok[5].getIntValue() : 1;
                events.push_back ({ DemoEvent::NoteOn, t, note, vel, ch });
                events.push_back ({ DemoEvent::NoteOff, t + dur, note, 0, ch });
            }
            else if (cmd == "sweep")
                sweeps.push_back ({ tok[1].getDoubleValue(), tok[2].getDoubleValue(), tok[3].replace ("_", " "),
                                    tok[4].getFloatValue(), tok[5].getFloatValue() });
            else if (cmd == "end")
                endTime = tok[1].getDoubleValue();
        }

        std::stable_sort (events.begin(), events.end(), [] (const DemoEvent& x, const DemoEvent& y) {
            return x.t != y.t ? x.t < y.t : x.type < y.type;
        });
        for (auto& s : sweeps)
            if ((s.p = findParam (plugin, s.param)) == nullptr)
                std::cout << "warning: no parameter named " << s.param << "\n";

        juce::DocumentWindow window ("deSIDuous demo", juce::Colours::black, 0);
        window.setUsingNativeTitleBar (true);
        window.setContentOwned (plugin.createEditorAndMakeActive(), true);
        window.setTopLeftPosition (20, 20);
        window.setAlwaysOnTop (true);   // the capture reads the screen, so nothing may cover the window
        window.setVisible (true);
        window.toFront (true);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (1500);

        int cx = 0, cy = 0, cw = 0, chh = 0;
        if (! getClientScreenRect (window.getPeer()->getNativeHandle(), cx, cy, cw, chh) || cw == 0)
        {
            std::cout << "cannot read window position\n";
            return 1;
        }
        std::cout << "window client " << cw << "x" << chh << " at " << cx << "," << cy << "\n";

        // Sync marker: a white cover over the plugin that disappears at the instant the
        // music starts. make-video.ps1 finds the first non-white frame and aligns the audio to it.
        struct Cover : juce::Component { void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::white); } } cover;
        cover.setOpaque (true);
        cover.addToDesktop (juce::ComponentPeer::windowIsTemporary);
        cover.setBounds (window.getContentComponent()->getScreenBounds());
        cover.setAlwaysOnTop (true);
        cover.setVisible (true);
        cover.toFront (false);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (300);

        // ffmpeg's Desktop Duplication capture (GPU) records the window region at a steady 30 fps.
        const auto videoFile = outDir.getChildFile ("ui.mkv");
        const juce::String cmd = "\"\"" + ffmpeg + "\" -y -loglevel error -f lavfi -i \"ddagrab=output_idx=0:framerate=30:draw_mouse=0"
                               + ":offset_x=" + juce::String (cx) + ":offset_y=" + juce::String (cy)
                               + ":video_size=" + juce::String (cw) + "x" + juce::String (chh)
                               + ",hwdownload,format=bgra,scale=" + juce::String (cw / 2) + ":" + juce::String (chh / 2) + ":flags=area\""
                               + " -t " + juce::String (endTime + 6.0, 1)
                               + " -c:v utvideo -pix_fmt gbrp \"" + videoFile.getFullPathName() + "\"\"";
        FILE* pipe = _popen (cmd.toRawUTF8(), "w");
        if (pipe == nullptr)
        {
            std::cout << "cannot start ffmpeg\n";
            return 1;
        }
        juce::MessageManager::getInstance()->runDispatchLoopUntil (2500);   // ffmpeg start-up + white pre-roll

        const int total = (int) std::ceil (endTime * sr);
        juce::AudioBuffer<float> out (2, total + block);
        out.clear();
        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer midi;

        size_t next = 0;
        int pos = 0;
        plugin.reset();
        cover.setVisible (false);
        const double t0 = juce::Time::getMillisecondCounterHiRes();

        while (pos < total)
        {
            const double now = (juce::Time::getMillisecondCounterHiRes() - t0) * 0.001;
            const int target = juce::jmin (total, (int) (now * sr));

            while (pos < target)
            {
                const double bt0 = pos / sr, bt1 = (pos + block) / sr;
                midi.clear();
                for (; next < events.size() && events[next].t < bt1; ++next)
                {
                    const auto& e = events[next];
                    const int offset = juce::jlimit (0, block - 1, (int) ((e.t - bt0) * sr));
                    if (e.type == DemoEvent::Program)
                        plugin.setCurrentProgram (e.a);
                    else if (e.type == DemoEvent::NoteOn)
                        midi.addEvent (juce::MidiMessage::noteOn (e.ch, e.a, (juce::uint8) e.b), offset);
                    else
                        midi.addEvent (juce::MidiMessage::noteOff (e.ch, e.a), offset);
                }
                for (auto& s : sweeps)
                    if (s.p != nullptr && bt0 >= s.t0 && bt0 <= s.t1)
                        s.p->setValueNotifyingHost (s.v0 + (s.v1 - s.v0) * (float) ((bt0 - s.t0) / (s.t1 - s.t0)));

                buf.clear();
                plugin.processBlock (buf, midi);
                for (int c = 0; c < 2; ++c)
                    out.copyFrom (c, pos, buf, c, 0, block);
                pos += block;
            }

            juce::MessageManager::getInstance()->runDispatchLoopUntil (4);   // let the editor repaint
        }

        // keep the window up until ffmpeg has finished its fixed-length capture
        _pclose (pipe);
        writeWav (out, total, outDir.getChildFile ("audio.wav"));
        std::cout << "recorded " << endTime << " s of audio + " << videoFile.getFullPathName() << "\n";
        window.clearContentComponent();
        return 0;
    }
}

namespace
{
    // ---- Multi-instance test: several plugin instances playing a drum pattern, run once
    // in parallel threads (as a DAW's multi-core mixer does) and once serially. Reports
    // how far the two differ and whether hit timing or pitch drifts over time.
    struct Hit { double t, dur; int note; };

    struct Track
    {
        juce::String label;
        int program;
        std::vector<Hit> hits;
        std::unique_ptr<juce::AudioPluginInstance> plugin;
        juce::AudioBuffer<float> out;
    };

    void renderTrack (Track& tr, int total)
    {
        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer midi;
        tr.out.setSize (2, total);
        tr.out.clear();
        for (int pos = 0; pos < total; pos += block)
        {
            const double t0 = pos / sr, t1 = (pos + block) / sr;
            midi.clear();
            for (auto& h : tr.hits)
            {
                if (h.t >= t0 && h.t < t1)              midi.addEvent (juce::MidiMessage::noteOn (1, h.note, (juce::uint8) 100), (int) ((h.t - t0) * sr));
                if (h.t + h.dur >= t0 && h.t + h.dur < t1) midi.addEvent (juce::MidiMessage::noteOff (1, h.note), (int) ((h.t + h.dur - t0) * sr));
            }
            buf.clear();
            tr.plugin->processBlock (buf, midi);
            const int n = juce::jmin (block, total - pos);
            for (int c = 0; c < 2; ++c)
                tr.out.copyFrom (c, pos, buf, c, 0, n);
        }
    }

    // Per hit: onset delay after the note-on, and the pitch from zero crossings 30-90 ms in
    void analyseHits (const Track& tr, int total)
    {
        const float* d = tr.out.getReadPointer (0);
        double minDelay = 1e9, maxDelay = -1e9, minHz = 1e9, maxHz = -1e9;
        std::vector<double> hzList;
        for (auto& h : tr.hits)
        {
            const int start = (int) (h.t * sr);
            int onset = -1;
            for (int i = start; i < juce::jmin (total, start + (int) (0.05 * sr)); ++i)
                if (std::abs (d[i]) > 0.01f) { onset = i; break; }
            if (onset < 0) continue;
            const double delayMs = (onset - start) * 1000.0 / sr;
            const int a = onset + (int) (0.03 * sr), b = onset + (int) (0.09 * sr);
            int crossings = 0;
            for (int i = a + 1; i < juce::jmin (total, b); ++i)
                if ((d[i - 1] <= 0.0f) != (d[i] <= 0.0f)) ++crossings;
            const double hz = crossings * 0.5 / 0.06;
            minDelay = juce::jmin (minDelay, delayMs); maxDelay = juce::jmax (maxDelay, delayMs);
            minHz = juce::jmin (minHz, hz); maxHz = juce::jmax (maxHz, hz);
            hzList.push_back (hz);
        }
        const int n = (int) hzList.size();
        auto avg = [&] (int from, int to) { double s = 0; for (int i = from; i < to; ++i) s += hzList[(size_t) i]; return s / juce::jmax (1, to - from); };
        std::printf ("  %-14s hits %3d  onset delay %.2f..%.2f ms  pitch first 8 hits %.1f Hz, last 8 hits %.1f Hz (range %.1f..%.1f)\n",
                     tr.label.toRawUTF8(), n, minDelay, maxDelay, avg (0, juce::jmin (8, n)), avg (juce::jmax (0, n - 8), n), minHz, maxHz);
    }

    int runMulti (juce::AudioPluginFormatManager& fm, const juce::PluginDescription& desc, const juce::File& outDir, double seconds)
    {
        const int total = (int) (seconds * sr);
        std::vector<Track> tracks (4);
        tracks[0] = { "Kick", 43, {} };
        tracks[1] = { "Open hat", 46, {} };
        tracks[2] = { "Closed hat", 45, {} };
        tracks[3] = { "Synth bass", 128 + 38, {} };   // GM 039 Synth Bass 1

        const int bassLine[8] = { 33, 33, 45, 33, 31, 31, 43, 31 };
        for (double t = 0.0; t < seconds - 1.0; t += 0.125)        // 16th notes at 120 BPM
        {
            const int step = (int) std::lround (t / 0.125);
            if (step % 4 == 0) tracks[0].hits.push_back ({ t, 0.1, 36 });
            if (step % 4 == 2) tracks[1].hits.push_back ({ t, 0.1, 46 });
            else               tracks[2].hits.push_back ({ t, 0.05, 42 });
            if (step % 2 == 0) tracks[3].hits.push_back ({ t, 0.2, bassLine[(step / 2) % 8] });
        }

        auto setup = [&] {
            for (auto& tr : tracks)
            {
                juce::String err;
                tr.plugin = fm.createPluginInstance (desc, sr, block, err);
                if (tr.plugin == nullptr) { std::cout << "load failed: " << err << "\n"; return false; }
                tr.plugin->setPlayConfigDetails (0, 2, sr, block);
                tr.plugin->prepareToPlay (sr, block);
                tr.plugin->setCurrentProgram (tr.program);
            }
            juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
            return true;
        };

        // ---- parallel: one thread per instance ----
        if (! setup()) return 1;
        const auto t0 = juce::Time::getHighResolutionTicks();
        {
            std::vector<std::thread> threads;
            for (auto& tr : tracks)
                threads.emplace_back ([&tr, total] { renderTrack (tr, total); });
            for (auto& th : threads) th.join();
        }
        const double parSecs = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0);
        std::vector<juce::AudioBuffer<float>> parallelOut;
        for (auto& tr : tracks) { parallelOut.push_back (tr.out); writeWav (tr.out, total, outDir.getChildFile ("multi_parallel_" + tr.label.replace (" ", "_") + ".wav")); }

        std::printf ("Parallel run: %.0f s of audio in %.1f s\n", seconds, parSecs);
        for (auto& tr : tracks) analyseHits (tr, total);

        // ---- serial: same instances recreated, one at a time ----
        for (auto& tr : tracks) tr.plugin.reset();
        if (! setup()) return 1;
        for (auto& tr : tracks) renderTrack (tr, total);
        std::printf ("Serial run:\n");
        for (auto& tr : tracks) analyseHits (tr, total);

        std::printf ("Parallel vs serial difference:\n");
        for (size_t i = 0; i < tracks.size(); ++i)
        {
            const float* a = parallelOut[i].getReadPointer (0);
            const float* b = tracks[i].out.getReadPointer (0);
            double diff = 0.0, sig = 0.0; float peakDiff = 0.0f;
            for (int k = 0; k < total; ++k) { const double e = a[k] - b[k]; diff += e * e; sig += (double) b[k] * b[k]; peakDiff = juce::jmax (peakDiff, (float) std::abs (e)); }
            std::printf ("  %-14s difference %.1f dB below the signal, peak difference %.1f dBFS\n", tracks[i].label.toRawUTF8(),
                         10.0 * std::log10 (sig / juce::jmax (1e-20, diff)), toDb (peakDiff));
        }
        for (auto& tr : tracks) tr.plugin.reset();
        return 0;
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;

    if (argc < 3)
    {
        std::cout << "usage: RenderTest <path to deSIDuous.vst3> <output dir>\n";
        return 1;
    }

    juce::AudioPluginFormatManager fm;
    fm.addFormat (std::make_unique<juce::VST3PluginFormat>());

    juce::OwnedArray<juce::PluginDescription> descs;
    juce::VST3PluginFormat vst3;
    vst3.findAllTypesForFile (descs, argv[1]);
    if (descs.isEmpty()) { std::cout << "no plugin found\n"; return 1; }

    juce::String err;
    auto plugin = fm.createPluginInstance (*descs[0], sr, block, err);
    if (plugin == nullptr) { std::cout << "load failed: " << err << "\n"; return 1; }

    plugin->setPlayConfigDetails (0, 2, sr, block);
    plugin->prepareToPlay (sr, block);

    if (argc > 3 && juce::String (argv[3]) == "--multi")
    {
        plugin.reset();
        return runMulti (fm, *descs[0], juce::File (argv[2]), argc > 4 ? juce::String (argv[4]).getDoubleValue() : 60.0);
    }

    if (argc > 5 && juce::String (argv[3]) == "--demo")
        return runDemo (*plugin, juce::File (argv[4]), juce::File (argv[2]), argv[5]);

    if (argc > 3 && juce::String (argv[3]) == "--ui")
    {
        // Show the editor in a window and feed it audio from a timer so the scope
        // and register monitor animate. Used for screenshots.
        plugin->setCurrentProgram (argc > 4 ? juce::String (argv[4]).getIntValue() : 20);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (200);

        juce::DocumentWindow window ("deSIDuous UI test", juce::Colours::black, 0);
        window.setUsingNativeTitleBar (true);
        window.setContentOwned (plugin->createEditorAndMakeActive(), true);
        window.setTopLeftPosition (20, 20);
        window.setAlwaysOnTop (true);   // screenshots must capture this window, not whatever is in front
        window.setVisible (true);
        window.toFront (true);

        struct Feeder : juce::Timer
        {
            juce::AudioPluginInstance& p; int count = 0;
            explicit Feeder (juce::AudioPluginInstance& pp) : p (pp) { startTimer (10); }
            void timerCallback() override
            {
                juce::AudioBuffer<float> b (2, 480);
                juce::MidiBuffer m;
                if (count == 10)   // after the preset's parameters reach the processor
                    for (int n : { 48, 55, 60, 64 }) m.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
                ++count;
                p.processBlock (b, m);
            }
        } feeder (*plugin);

        juce::MessageManager::getInstance()->runDispatchLoopUntil (8000);
        window.clearContentComponent();
        return 0;
    }

    const juce::File outDir (argv[2]);
    outDir.createDirectory();

    std::cout << "Loaded " << plugin->getName() << ", " << plugin->getNumPrograms() << " programs, "
              << plugin->getParameters().size() << " parameters\n\n";
    std::printf ("%-36s %8s %8s %8s %7s %6s\n", "PRESET", "PEAK dB", "ST-RMS", "ZCR Hz", "TAIL s", "CPU%");

    int problems = 0;
    juce::String levels;   // name, chord/single, peak dB, short-term RMS dB: input for tools/level-presets.ps1
    for (int i = 0; i < plugin->getNumPrograms(); ++i)
    {
        plugin->setCurrentProgram (i);
        // let the async chip rebuild (clock / quality changes) settle
        juce::MessageManager::getInstance()->runDispatchLoopUntil (50);

        const auto name = plugin->getProgramName (i);
        const auto category = name.upToFirstOccurrenceOf (":", false, false);
        // chords for the families normally played polyphonically, low notes for basses
        const bool chordy = juce::StringArray { "Pad", "Keys", "GM Piano", "GM Organ", "GM Ensemble", "GM Synth Pad" }.contains (category);
        const bool bass = category == "Bass" || category == "GM Bass";
        const int root = bass ? 36 : 60;

        std::vector<std::pair<int, int>> notes { { 1, root } };
        if (chordy) notes = { { 1, 60 }, { 1, 64 }, { 1, 67 } };

        const auto file = outDir.getChildFile (juce::String (i).paddedLeft ('0', 3) + "_" + juce::File::createLegalFileName (name.replace (": ", "_")) + ".wav");
        const auto s = render (*plugin, notes, 1.2, 3.5, file);

        const bool silent = s.rmsDb < -36.0f;
        const bool hot = s.peakDb > -0.5f;
        if (silent || hot) ++problems;
        levels << name << "\t" << (chordy ? "chord" : "single") << "\t" << juce::String (s.peakDb, 2) << "\t" << juce::String (s.rmsDb, 2) << "\n";
        std::printf ("%-36s %8.1f %8.1f %8.0f %7.2f %6.1f %s\n", name.toRawUTF8(), s.peakDb, s.rmsDb, s.zcrHz, s.tailSec, s.cpuPercent,
                     silent ? "<< QUIET" : (hot ? "<< CLIPS" : ""));
    }

    // ---- voice mode checks on the init patch ----
    plugin->setCurrentProgram (0);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (50);
    if (auto* vm = findParam (*plugin, "Voice Mode"))
    {
        std::cout << "\nVoice modes (Init Saw):\n";
        vm->setValueNotifyingHost (0.0f);
        auto s = render (*plugin, { { 1, 60 }, { 1, 64 }, { 1, 67 }, { 1, 71 } }, 1.0, 2.0, outDir.getChildFile ("mode_layered.wav"));
        std::printf ("  layered poly 4 notes   rms %.1f dB  cpu %.1f%%\n", s.rmsDb, s.cpuPercent);

        vm->setValueNotifyingHost (0.5f);
        if (auto* v2 = findParam (*plugin, "V2 On")) v2->setValueNotifyingHost (1.0f);
        if (auto* v3 = findParam (*plugin, "V3 On")) v3->setValueNotifyingHost (1.0f);
        s = render (*plugin, { { 1, 60 }, { 1, 64 }, { 1, 67 } }, 1.0, 2.0, outDir.getChildFile ("mode_paraphonic.wav"));
        std::printf ("  paraphonic 3 notes     rms %.1f dB  cpu %.1f%%\n", s.rmsDb, s.cpuPercent);

        vm->setValueNotifyingHost (1.0f);
        s = render (*plugin, { { 1, 48 }, { 2, 55 }, { 3, 64 } }, 1.0, 2.0, outDir.getChildFile ("mode_channels.wav"));
        std::printf ("  midi ch 1-3            rms %.1f dB  cpu %.1f%%\n", s.rmsDb, s.cpuPercent);
        s = render (*plugin, { { 5, 60 } }, 1.0, 2.0, outDir.getChildFile ("mode_channels_ch5.wav"));
        std::printf ("  midi ch 5 (ignored)    rms %.1f dB\n", s.rmsDb);

        // resampling quality cost, 4 chips
        vm->setValueNotifyingHost (0.0f);
        if (auto* q = findParam (*plugin, "Resampling"))
        {
            q->setValueNotifyingHost (1.0f);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (200);
            s = render (*plugin, { { 1, 60 }, { 1, 64 }, { 1, 67 }, { 1, 71 } }, 1.0, 2.0, outDir.getChildFile ("mode_layered_fast.wav"));
            std::printf ("  layered 4 notes, fast  rms %.1f dB  cpu %.1f%%\n", s.rmsDb, s.cpuPercent);
            q->setValueNotifyingHost (0.0f);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (200);
        }
    }

    // ---- state round trip ----
    {
        plugin->setCurrentProgram (5);
        juce::MemoryBlock mb;
        plugin->getStateInformation (mb);
        plugin->setCurrentProgram (0);
        plugin->setStateInformation (mb.getData(), (int) mb.getSize());
        std::cout << "\nState round trip: program " << plugin->getCurrentProgram() << " (expected 5), "
                  << mb.getSize() << " bytes\n";
    }

    plugin->releaseResources();
    outDir.getChildFile ("levels.tsv").replaceWithText (levels);
    std::cout << "\n" << problems << " preset(s) flagged\n";
    return 0;
}
