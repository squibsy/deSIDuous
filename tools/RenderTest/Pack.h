// Batch loop / one-shot renderer for building sample packs. See tools/pack/README.md.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Renders every job in the manifest to outDir. Returns the number of failed jobs.
int runPack (juce::AudioPluginInstance& plugin, const juce::File& manifest, const juce::File& outDir);
