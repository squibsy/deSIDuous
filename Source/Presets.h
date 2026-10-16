#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

struct Preset
{
    juce::String category, name;
    std::vector<std::pair<juce::String, float>> values;   // plain (un-normalised) parameter values

    juce::String displayName() const { return category + ": " + name; }
};

const std::vector<Preset>& getFactoryPresets();

// Resets every parameter to its default, then applies the preset's values.
void applyPreset (juce::AudioProcessorValueTreeState& state, const Preset& preset);
