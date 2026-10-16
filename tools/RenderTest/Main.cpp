// Offline test host: loads the built SIDSynth.vst3, renders every factory preset
// to a WAV file and prints level / brightness / CPU statistics.
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

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

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;

    if (argc < 3)
    {
        std::cout << "usage: RenderTest <path to SIDSynth.vst3> <output dir>\n";
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

    if (argc > 3 && juce::String (argv[3]) == "--ui")
    {
        // Show the editor in a window and feed it audio from a timer so the scope
        // and register monitor animate. Used for screenshots.
        plugin->setCurrentProgram (argc > 4 ? juce::String (argv[4]).getIntValue() : 20);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (200);

        juce::DocumentWindow window ("SIDSynth UI test", juce::Colours::black, 0);
        window.setUsingNativeTitleBar (true);
        window.setContentOwned (plugin->createEditorAndMakeActive(), true);
        window.setTopLeftPosition (20, 20);
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
                if (count == 0)
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
    for (int i = 0; i < plugin->getNumPrograms(); ++i)
    {
        plugin->setCurrentProgram (i);
        // let the async chip rebuild (clock / quality changes) settle
        juce::MessageManager::getInstance()->runDispatchLoopUntil (50);

        const auto name = plugin->getProgramName (i);
        const bool chordy = name.startsWith ("Pad") || name.startsWith ("Keys");
        const bool bass = name.startsWith ("Bass");
        const int root = bass ? 36 : 60;

        std::vector<std::pair<int, int>> notes { { 1, root } };
        if (chordy) notes = { { 1, 60 }, { 1, 64 }, { 1, 67 } };

        const auto file = outDir.getChildFile (juce::String (i).paddedLeft ('0', 2) + "_" + juce::File::createLegalFileName (name.replace (": ", "_")) + ".wav");
        const auto s = render (*plugin, notes, 1.2, 3.5, file);

        const bool silent = s.rmsDb < -36.0f;
        const bool hot = s.peakDb > -0.5f;
        if (silent || hot) ++problems;
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
    std::cout << "\n" << problems << " preset(s) flagged\n";
    return 0;
}
