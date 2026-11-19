// Pack renderer: reads a manifest of jobs and renders each to a WAV (plus a MIDI file for loops).
//
//   job <name>                  starts a job; the name becomes the file name
//   kind loop|oneshot           loop (default) is rendered at exact bar length and loops seamlessly
//   bpm <n>                     tempo; also fed to the plugin as the host tempo (tempo-synced delay)
//   bars <n>                    loop length in bars of 4/4
//   key <text>                  first word names the key folder (Am); the rest is metadata. none = NoKey
//   part <name>                 instrument type folder (Bass, Pad, Lead, Drums, FX)
//   tail <seconds>              one-shots: longest tail kept after the last note (default 3)
//   normalize <dBFS>            peak-normalise the finished render to this level
//   track <preset name>         starts a track; the name is the program name, e.g. Drums: Kick
//   gain <dB>                   track level
//   param <name> <0..1>         normalised parameter value for this track (underscores = spaces)
//   lfo <name> <shape> <bars> <lo> <hi> [phase]
//                               tempo-synced automation of a parameter (normalised values). shape is sine, tri
//                               (both start at lo), saw (rises lo to hi, then jumps back) or rsaw. One cycle takes
//                               <bars> bars; for a seamless loop the loop length must be a whole number of cycles.
//                               phase 0..1 shifts the start. Overrides a param line for the same parameter.
//   note <beat> <beats> <midi> [velocity] [channel]
//   endjob
//
// Tracks are rendered one after another on the same plugin instance and summed. Effect
// parameters are put back to their defaults at the start of every track.
#include "Pack.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>

namespace
{
    constexpr double sr = 48000.0;
    constexpr int block = 256;
    constexpr int ticksPerBeat = 480;

    struct Note { double start, dur; int note, vel, ch; };

    struct Lfo { juce::String param, shape; double barsPerCycle, lo, hi, phase; };

    struct Track
    {
        juce::String preset;
        float gainDb = 0.0f;
        std::vector<std::pair<juce::String, float>> params;
        std::vector<Lfo> lfos;
        std::vector<Note> notes;
    };

    struct Job
    {
        juce::String name, key, part;
        bool loop = true;
        double bpm = 120.0, tail = 3.0;
        int bars = 2, firstLine = 0;
        bool normalise = false;
        float normaliseDb = -1.0f;
        std::vector<Track> tracks;
    };

    // Reports a fixed tempo and a running sample position, like a DAW's transport.
    class FixedPlayHead : public juce::AudioPlayHead
    {
    public:
        double bpm = 120.0;
        juce::int64 pos = 0;

        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo info;
            info.setBpm (bpm);
            info.setTimeInSamples (pos);
            info.setTimeInSeconds ((double) pos / sr);
            info.setPpqPosition ((double) pos / sr * bpm / 60.0);
            info.setIsPlaying (true);
            return info;
        }
    };

    juce::AudioProcessorParameter* findParam (juce::AudioPluginInstance& p, const juce::String& name)
    {
        for (auto* prm : p.getParameters())
            if (prm->getName (100) == name)
                return prm;
        return nullptr;
    }

    void settle (int ms) { juce::MessageManager::getInstance()->runDispatchLoopUntil (ms); }

    // ---- manifest parsing -------------------------------------------------------------
    std::vector<Job> parseManifest (const juce::File& file)
    {
        std::vector<Job> jobs;
        Job* job = nullptr;
        int lineNo = 0;

        for (auto line : juce::StringArray::fromLines (file.loadFileAsString()))
        {
            ++lineNo;
            line = line.trim();
            if (line.isEmpty() || line.startsWith ("#"))
                continue;

            auto tok = juce::StringArray::fromTokens (line, " \t", "");
            tok.removeEmptyStrings();
            const auto cmd = tok[0];
            const auto rest = line.fromFirstOccurrenceOf (" ", false, false).trim();

            if (cmd == "job")
            {
                jobs.emplace_back();
                job = &jobs.back();
                job->name = juce::File::createLegalFileName (rest);
                job->firstLine = lineNo;
            }
            else if (job == nullptr)
                std::printf ("line %d: '%s' outside a job, ignored\n", lineNo, cmd.toRawUTF8());
            else if (cmd == "kind")      job->loop = (tok[1] != "oneshot");
            else if (cmd == "bpm")       job->bpm = juce::jlimit (20.0, 400.0, tok[1].getDoubleValue());
            else if (cmd == "bars")      job->bars = juce::jmax (1, tok[1].getIntValue());
            else if (cmd == "key")       job->key = rest;
            else if (cmd == "part")      job->part = rest;
            else if (cmd == "tail")      job->tail = juce::jlimit (0.0, 30.0, tok[1].getDoubleValue());
            else if (cmd == "normalize") { job->normalise = true; job->normaliseDb = tok[1].getFloatValue(); }
            else if (cmd == "track")     { job->tracks.emplace_back(); job->tracks.back().preset = rest; }
            else if (cmd == "endjob")    job = nullptr;
            else if (job->tracks.empty())
                std::printf ("line %d: '%s' before any track, ignored\n", lineNo, cmd.toRawUTF8());
            else if (cmd == "gain")      job->tracks.back().gainDb = tok[1].getFloatValue();
            else if (cmd == "param")     job->tracks.back().params.emplace_back (tok[1].replace ("_", " "), tok[2].getFloatValue());
            else if (cmd == "lfo")
                job->tracks.back().lfos.push_back ({ tok[1].replace ("_", " "), tok[2], juce::jmax (0.0625, tok[3].getDoubleValue()),
                                                     tok[4].getDoubleValue(), tok[5].getDoubleValue(),
                                                     tok.size() > 6 ? tok[6].getDoubleValue() : 0.0 });
            else if (cmd == "note")
                job->tracks.back().notes.push_back ({ tok[1].getDoubleValue(), tok[2].getDoubleValue(), tok[3].getIntValue(),
                                                      tok.size() > 4 ? tok[4].getIntValue() : 100,
                                                      tok.size() > 5 ? tok[5].getIntValue() : 1 });
            else
                std::printf ("line %d: unknown command '%s'\n", lineNo, cmd.toRawUTF8());
        }
        return jobs;
    }

    // ---- rendering --------------------------------------------------------------------
    struct Event { int sample; int order; juce::MidiMessage msg; };   // order: note-offs before note-ons

    // 0..1 position in the cycle -> 0..1 value
    double lfoShape (const juce::String& shape, double phase)
    {
        if (shape == "tri")  return 1.0 - std::abs (2.0 * phase - 1.0);
        if (shape == "saw")  return phase;
        if (shape == "rsaw") return 1.0 - phase;
        return 0.5 - 0.5 * std::cos (2.0 * juce::MathConstants<double>::pi * phase);   // sine
    }

    struct ResolvedLfo { const Lfo* lfo; juce::AudioProcessorParameter* param; };

    std::vector<ResolvedLfo> resolveLfos (juce::AudioPluginInstance& plugin, const Track& tr, const Job& job)
    {
        std::vector<ResolvedLfo> out;
        for (auto& l : tr.lfos)
        {
            auto* p = findParam (plugin, l.param);
            if (p == nullptr) { std::printf ("  warning: no parameter named '%s' for lfo\n", l.param.toRawUTF8()); continue; }
            if (! juce::StringArray { "sine", "tri", "saw", "rsaw" }.contains (l.shape))
                std::printf ("  warning: unknown lfo shape '%s', using sine\n", l.shape.toRawUTF8());
            if (job.loop && std::abs (std::fmod (job.bars / l.barsPerCycle, 1.0)) > 1e-9)
                std::printf ("  warning: %s: %d bars is not a whole number of %.3g-bar cycles of '%s', the loop will not be seamless\n",
                             job.name.toRawUTF8(), job.bars, l.barsPerCycle, l.param.toRawUTF8());
            out.push_back ({ &l, p });
        }
        return out;
    }

    juce::AudioBuffer<float> renderEvents (juce::AudioPluginInstance& plugin, FixedPlayHead& ph,
                                           std::vector<Event> events, int totalSamples,
                                           const std::vector<ResolvedLfo>& lfos = {})
    {
        std::stable_sort (events.begin(), events.end(), [] (const Event& a, const Event& b) {
            return a.sample != b.sample ? a.sample < b.sample : a.order < b.order;
        });

        const int padded = ((totalSamples + block - 1) / block) * block;
        juce::AudioBuffer<float> out (2, padded);
        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer midi;
        size_t next = 0;

        plugin.reset();
        for (int pos = 0; pos < padded; pos += block)
        {
            midi.clear();
            for (; next < events.size() && events[next].sample < pos + block; ++next)
                midi.addEvent (events[next].msg, juce::jmax (0, events[next].sample - pos));
            ph.pos = pos;
            if (! lfos.empty())
            {
                const double beat = (double) pos / sr * ph.bpm / 60.0;
                for (auto& r : lfos)
                {
                    double cycle = beat / (r.lfo->barsPerCycle * 4.0) + r.lfo->phase;
                    cycle -= std::floor (cycle);
                    const double v = r.lfo->lo + (r.lfo->hi - r.lfo->lo) * lfoShape (r.lfo->shape, cycle);
                    r.param->setValueNotifyingHost ((float) juce::jlimit (0.0, 1.0, v));
                }
            }
            buf.clear();
            plugin.processBlock (buf, midi);
            for (int c = 0; c < 2; ++c)
                out.copyFrom (c, pos, buf, c, 0, block);
        }
        return out;
    }

    double beatsToSamples (double beats, double bpm) { return beats * 60.0 / bpm * sr; }

    void prepareTrack (juce::AudioPluginInstance& plugin, const Track& tr, int programIndex)
    {
        // A change to the program that is already selected can be ignored by the plugin wrapper, which would leave the
        // previous track's overrides (filter mode, automated parameters) in place. Go via another program to force a reload.
        plugin.setCurrentProgram ((programIndex + 1) % juce::jmax (2, plugin.getNumPrograms()));
        settle (20);
        plugin.setCurrentProgram (programIndex);
        settle (50);
        // effects are not part of a preset: put them back to their defaults
        for (auto* name : { "Chorus I", "Chorus II", "Delay On", "Delay Sync", "Delay Division", "Delay Time", "Delay Feedback",
                            "Delay Mix", "Reverb On", "Reverb Size", "Reverb Damping", "Reverb Mix" })
            if (auto* p = findParam (plugin, name))
                p->setValueNotifyingHost (p->getDefaultValue());
        for (auto& [name, value] : tr.params)
        {
            if (auto* p = findParam (plugin, name))
                p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, value));
            else
                std::printf ("  warning: no parameter named '%s'\n", name.toRawUTF8());
        }
        settle (80);   // chip rebuilds are asynchronous
    }

    juce::AudioBuffer<float> renderLoopTrack (juce::AudioPluginInstance& plugin, FixedPlayHead& ph, const Job& job, const Track& tr)
    {
        const int cycleLen = (int) std::lround (beatsToSamples (job.bars * 4.0, job.bpm));
        const double loopBeats = job.bars * 4.0;
        constexpr int cycles = 3;   // keep the middle one: it holds the previous cycle's tails, so it is steady state

        std::vector<Event> events;
        for (int c = 0; c < cycles; ++c)
            for (auto& n : tr.notes)
            {
                if (n.start < 0.0 || n.start >= loopBeats) continue;
                const double dur = juce::jmin (n.dur, loopBeats - n.start);   // a note cannot cross the loop point
                const int on  = c * cycleLen + (int) std::lround (beatsToSamples (n.start, job.bpm));
                const int off = c * cycleLen + (int) std::lround (beatsToSamples (n.start + dur, job.bpm));
                events.push_back ({ on, 1, juce::MidiMessage::noteOn (n.ch, n.note, (juce::uint8) n.vel) });
                events.push_back ({ off, 0, juce::MidiMessage::noteOff (n.ch, n.note) });
            }

        auto all = renderEvents (plugin, ph, events, cycles * cycleLen, resolveLfos (plugin, tr, job));
        juce::AudioBuffer<float> mid (2, cycleLen);
        for (int c = 0; c < 2; ++c)
            mid.copyFrom (c, 0, all, c, cycleLen, cycleLen);

        // How well does the loop repeat? Compare the start of the kept cycle with the start of the cycle after it. They are
        // identical when every source of sound (note phases, filters, modulation) is locked to the loop length.
        const int probe = juce::jmin (cycleLen, (int) (0.1 * sr));
        double sig = 0.0, err = 0.0;
        for (int i = 0; i < probe; ++i)
        {
            const double a = all.getSample (0, cycleLen + i), b = all.getSample (0, 2 * cycleLen + i);
            sig += a * a; err += (a - b) * (a - b);
        }
        const double repeatDb = 10.0 * std::log10 ((sig + 1e-20) / (err + 1e-20));
        if (sig > 1e-6 && repeatDb < 30.0)
            std::printf ("  %s [%s]: loop start differs from the next cycle (%.0f dB repeatability)\n", job.name.toRawUTF8(), tr.preset.toRawUTF8(), repeatDb);

        // Make the join itself continuous. The sample that follows the loop's last one is the first of the next cycle, so the
        // loop starts with that and cross-fades into the kept cycle over 20 ms (a fade this short only matters for tails).
        const int fade = juce::jmin (cycleLen / 4, (int) (0.02 * sr));
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < fade; ++i)
            {
                const float w = (float) i / (float) fade;
                mid.setSample (c, i, (1.0f - w) * all.getSample (c, 2 * cycleLen + i) + w * all.getSample (c, cycleLen + i));
            }
        return mid;
    }

    juce::AudioBuffer<float> renderOneShotTrack (juce::AudioPluginInstance& plugin, FixedPlayHead& ph, const Job& job, const Track& tr)
    {
        std::vector<Event> events;
        double lastOff = 0.0;
        for (auto& n : tr.notes)
        {
            const int on  = (int) std::lround (beatsToSamples (n.start, job.bpm));
            const int off = (int) std::lround (beatsToSamples (n.start + n.dur, job.bpm));
            lastOff = juce::jmax (lastOff, (double) off);
            events.push_back ({ on, 1, juce::MidiMessage::noteOn (n.ch, n.note, (juce::uint8) n.vel) });
            events.push_back ({ off, 0, juce::MidiMessage::noteOff (n.ch, n.note) });
        }
        return renderEvents (plugin, ph, events, (int) (lastOff + job.tail * sr), resolveLfos (plugin, tr, job));
    }

    void applyGain (juce::AudioBuffer<float>& b, float db)
    {
        if (db != 0.0f)
            b.applyGain (juce::Decibels::decibelsToGain (db));
    }

    float peakOf (const juce::AudioBuffer<float>& b)
    {
        return juce::jmax (b.getMagnitude (0, 0, b.getNumSamples()), b.getMagnitude (1, 0, b.getNumSamples()));
    }

    float rmsDbOf (const juce::AudioBuffer<float>& b)
    {
        double acc = 0.0;
        for (int c = 0; c < 2; ++c)
        {
            const float* d = b.getReadPointer (c);
            for (int i = 0; i < b.getNumSamples(); ++i) acc += (double) d[i] * d[i];
        }
        return juce::Decibels::gainToDecibels ((float) std::sqrt (acc / (2.0 * juce::jmax (1, b.getNumSamples()))), -120.0f);
    }

    void trimOneShot (juce::AudioBuffer<float>& b)
    {
        int last = 0;
        for (int c = 0; c < 2; ++c)
        {
            const float* d = b.getReadPointer (c);
            for (int i = 0; i < b.getNumSamples(); ++i)
                if (std::abs (d[i]) > 0.001f) last = juce::jmax (last, i);   // -60 dBFS
        }
        const int len = juce::jmin (b.getNumSamples(), last + (int) (0.02 * sr));
        b.setSize (2, juce::jmax (1, len), true);
        b.applyGainRamp (0, juce::jmax (0, len - (int) (0.01 * sr)), (int) (0.01 * sr), 1.0f, 0.0f);   // 10 ms fade out
    }

    bool writeWav (const juce::AudioBuffer<float>& b, const juce::File& file)
    {
        file.deleteFile();
        juce::WavAudioFormat wav;
        if (auto stream = file.createOutputStream())
            if (auto writer = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (stream.get(), sr, 2, 24, {}, 0)))
            {
                stream.release();
                return writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
            }
        return false;
    }

    void writeMidi (const Job& job, const juce::File& file)
    {
        juce::MidiFile mf;
        mf.setTicksPerQuarterNote (ticksPerBeat);
        const double loopBeats = job.bars * 4.0;

        juce::MidiMessageSequence conductor;
        conductor.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::lround (60000000.0 / job.bpm)).withTimeStamp (0));
        conductor.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4).withTimeStamp (0));
        conductor.addEvent (juce::MidiMessage::endOfTrack().withTimeStamp (loopBeats * ticksPerBeat));
        mf.addTrack (conductor);

        for (auto& tr : job.tracks)
        {
            juce::MidiMessageSequence seq;
            seq.addEvent (juce::MidiMessage::textMetaEvent (3, tr.preset).withTimeStamp (0));
            for (auto& n : tr.notes)
            {
                if (n.start < 0.0 || n.start >= loopBeats) continue;
                const double dur = juce::jmin (n.dur, loopBeats - n.start);
                seq.addEvent (juce::MidiMessage::noteOn (n.ch, n.note, (juce::uint8) n.vel).withTimeStamp (n.start * ticksPerBeat));
                seq.addEvent (juce::MidiMessage::noteOff (n.ch, n.note).withTimeStamp ((n.start + dur) * ticksPerBeat));
            }
            seq.addEvent (juce::MidiMessage::endOfTrack().withTimeStamp (loopBeats * ticksPerBeat));
            seq.sort();
            mf.addTrack (seq);
        }

        file.deleteFile();
        if (auto stream = file.createOutputStream())
            mf.writeTo (*stream);
    }
}

int runPack (juce::AudioPluginInstance& plugin, const juce::File& manifest, const juce::File& outDir)
{
    if (! manifest.existsAsFile())
    {
        std::printf ("manifest not found: %s\n", manifest.getFullPathName().toRawUTF8());
        return 1;
    }
    outDir.createDirectory();

    std::map<juce::String, int> programs;
    for (int i = 0; i < plugin.getNumPrograms(); ++i)
        programs[plugin.getProgramName (i)] = i;

    FixedPlayHead playHead;
    plugin.setPlayHead (&playHead);

    const auto jobs = parseManifest (manifest);
    std::printf ("%d job(s) in %s\n\n", (int) jobs.size(), manifest.getFileName().toRawUTF8());
    std::printf ("%-44s %-8s %5s %5s %8s %8s %6s\n", "JOB", "KIND", "BPM", "SEC", "PEAK dB", "RMS dB", "NOTES");

    // The index is incremental: rows of an earlier run stay unless this run renders the same file again, so a second manifest
    // can add jobs to a pack without losing the first one's rows.
    const juce::String indexHeader = "file\tkind\tpart\tbpm\tbars\tkey\ttracks\tseconds\tpeak_db\trms_db\tpatches\n";
    const auto indexFile = outDir.getChildFile ("pack_index.tsv");
    juce::StringArray earlierRows;
    if (indexFile.existsAsFile())
    {
        earlierRows = juce::StringArray::fromLines (indexFile.loadFileAsString());
        if (earlierRows.size() > 0) earlierRows.remove (0);   // the header
    }
    std::set<juce::String> redone;
    juce::String index;   // rows made by this run
    int failed = 0;

    for (auto& job : jobs)
    {
        if (job.tracks.empty()) { std::printf ("%-44s no tracks, skipped\n", job.name.toRawUTF8()); ++failed; continue; }

        bool ok = true;
        for (auto& tr : job.tracks)
            if (programs.find (tr.preset) == programs.end())
            {
                std::printf ("%-44s unknown preset '%s', skipped\n", job.name.toRawUTF8(), tr.preset.toRawUTF8());
                ok = false;
            }
        if (! ok) { ++failed; continue; }

        playHead.bpm = job.bpm;
        juce::AudioBuffer<float> mix;
        int noteCount = 0;

        for (auto& tr : job.tracks)
        {
            prepareTrack (plugin, tr, programs[tr.preset]);
            auto part = job.loop ? renderLoopTrack (plugin, playHead, job, tr) : renderOneShotTrack (plugin, playHead, job, tr);
            applyGain (part, tr.gainDb);
            noteCount += (int) tr.notes.size();

            if (mix.getNumSamples() == 0)
                mix = part;
            else
            {
                if (part.getNumSamples() > mix.getNumSamples())
                    mix.setSize (2, part.getNumSamples(), true, true, false);
                for (int c = 0; c < 2; ++c)
                    mix.addFrom (c, 0, part, c, 0, part.getNumSamples());
            }
        }

        if (! job.loop)
            trimOneShot (mix);

        // normalise on request; otherwise only pull a render down if it would clip
        float peak = peakOf (mix);
        if (job.normalise && peak > 0.0f)
            applyGain (mix, job.normaliseDb - juce::Decibels::gainToDecibels (peak));
        else if (peak > 0.98f)
        {
            applyGain (mix, -1.0f - juce::Decibels::gainToDecibels (peak));
            std::printf ("  %s: summed tracks clipped, pulled down to -1 dBFS\n", job.name.toRawUTF8());
        }
        peak = peakOf (mix);

        // Loops:     Loops/<bpm>bpm/<key>/<part>/WAV and /MIDI
        //            Loops/<bpm>bpm/<part>/WAV and /MIDI    (key none: drums)
        // One-shots: OneShots/<part>
        const auto part = juce::File::createLegalFileName (job.part.isEmpty() ? "Other" : job.part);
        const auto keyName = juce::File::createLegalFileName (job.key.upToFirstOccurrenceOf (" ", false, false).trim());
        const bool keyless = keyName.isEmpty() || keyName.equalsIgnoreCase ("none");
        auto loopDir = outDir.getChildFile ("Loops").getChildFile (juce::String (juce::roundToInt (job.bpm)) + "bpm");
        if (! keyless) loopDir = loopDir.getChildFile (keyName);
        const auto base = job.loop ? loopDir.getChildFile (part) : outDir.getChildFile ("OneShots").getChildFile (part);
        const auto wavDir = job.loop ? base.getChildFile ("WAV") : base;
        wavDir.createDirectory();
        const auto wavFile = wavDir.getChildFile (job.name + ".wav");

        const bool wrote = writeWav (mix, wavFile);
        if (job.loop)
        {
            const auto midiDir = base.getChildFile ("MIDI");
            midiDir.createDirectory();
            writeMidi (job, midiDir.getChildFile (job.name + ".mid"));
        }
        if (! wrote) { std::printf ("%-44s could not write the WAV\n", job.name.toRawUTF8()); ++failed; continue; }

        const double seconds = mix.getNumSamples() / sr;
        const float peakDb = juce::Decibels::gainToDecibels (peak, -120.0f), rmsDb = rmsDbOf (mix);
        std::printf ("%-44s %-8s %5.0f %5.2f %8.1f %8.1f %6d%s\n", job.name.toRawUTF8(), job.loop ? "loop" : "oneshot", job.bpm, seconds,
                     peakDb, rmsDb, noteCount, rmsDb < -50.0f ? "  << SILENT" : "");
        const auto relPath = wavFile.getRelativePathFrom (outDir).replace ("\\", "/");
        redone.insert (relPath);
        index << relPath << "\t" << (job.loop ? "loop" : "oneshot") << "\t" << part << "\t"
              << juce::String (job.bpm, 1) << "\t"
              << (job.loop ? juce::String (job.bars) : juce::String()) << "\t" << job.key << "\t" << (int) job.tracks.size() << "\t"
              << juce::String (seconds, 3) << "\t" << juce::String (peakDb, 2) << "\t" << juce::String (rmsDb, 2) << "\t";
        for (size_t t = 0; t < job.tracks.size(); ++t)
            index << (t > 0 ? " + " : "") << job.tracks[t].preset;
        index << "\n";
    }

    plugin.setPlayHead (nullptr);
    juce::String fullIndex = indexHeader;
    int kept = 0;
    for (auto& row : earlierRows)
    {
        const auto key = row.upToFirstOccurrenceOf ("\t", false, false);
        if (key.isNotEmpty() && redone.count (key) == 0) { fullIndex << row << "\n"; ++kept; }
    }
    fullIndex << index;
    indexFile.replaceWithText (fullIndex);
    if (kept > 0)
        std::printf ("\nkept %d rows of the earlier index\n", kept);
    std::printf ("\n%d job(s) rendered, %d failed. Index: %s\n", (int) jobs.size() - failed, failed,
                 outDir.getChildFile ("pack_index.tsv").getFullPathName().toRawUTF8());
    return failed;
}
