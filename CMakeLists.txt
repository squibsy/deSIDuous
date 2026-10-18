cmake_minimum_required(VERSION 3.22)

project(deSIDuous VERSION 1.03 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(libs/JUCE)

# ---------------------------------------------------------------------------
# reSIDfp (from libsidplayfp, GPL v2+): cycle-exact MOS 6581/8580 emulation
# ---------------------------------------------------------------------------
set(RESIDFP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/libsidplayfp/src/builders/residfp-builder/residfp)

add_library(residfp STATIC
    ${RESIDFP_DIR}/Dac.cpp
    ${RESIDFP_DIR}/EnvelopeGenerator.cpp
    ${RESIDFP_DIR}/ExternalFilter.cpp
    ${RESIDFP_DIR}/Filter.cpp
    ${RESIDFP_DIR}/Filter6581.cpp
    ${RESIDFP_DIR}/Filter8580.cpp
    ${RESIDFP_DIR}/FilterModelConfig.cpp
    ${RESIDFP_DIR}/FilterModelConfig6581.cpp
    ${RESIDFP_DIR}/FilterModelConfig8580.cpp
    ${RESIDFP_DIR}/Integrator6581.cpp
    ${RESIDFP_DIR}/Integrator8580.cpp
    ${RESIDFP_DIR}/OpAmp.cpp
    ${RESIDFP_DIR}/SID.cpp
    ${RESIDFP_DIR}/Spline.cpp
    ${RESIDFP_DIR}/WaveformCalculator.cpp
    ${RESIDFP_DIR}/WaveformGenerator.cpp
    ${RESIDFP_DIR}/resample/SincResampler.cpp
    ${RESIDFP_DIR}/version.cc)

set_source_files_properties(${RESIDFP_DIR}/version.cc PROPERTIES LANGUAGE CXX)

target_include_directories(residfp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/config
        ${RESIDFP_DIR}
        ${RESIDFP_DIR}/resample
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/libsidplayfp/src)

target_compile_definitions(residfp PUBLIC HAVE_CXX17)
set_target_properties(residfp PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(residfp PRIVATE /W0 /O2 /fp:fast /EHsc)
endif()

# ---------------------------------------------------------------------------
# The plugin
# ---------------------------------------------------------------------------
juce_add_plugin(deSIDuous
    COMPANY_NAME "Bananalytics"
    BUNDLE_ID "ie.bananalytics.desiduous"
    PLUGIN_MANUFACTURER_CODE Bnly
    PLUGIN_CODE Sid8
    PRODUCT_NAME "deSIDuous"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    FORMATS VST3)

target_sources(deSIDuous PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Params.cpp
    Source/SidEngine.cpp
    Source/Presets.cpp
    Source/PresetsExtra.cpp
    Source/PresetsGM.cpp
    Source/SidLookAndFeel.cpp)

juce_add_binary_data(deSIDuousData SOURCES Resources/logo.png)

target_compile_definitions(deSIDuous PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(deSIDuous
    PRIVATE
        residfp
        deSIDuousData
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Offline test host (optional): renders every preset from the built VST3
# ---------------------------------------------------------------------------
option(DESIDUOUS_BUILD_TESTS "Build the RenderTest host" ON)
if(DESIDUOUS_BUILD_TESTS)
    juce_add_console_app(RenderTest PRODUCT_NAME "RenderTest")
    target_sources(RenderTest PRIVATE tools/RenderTest/Main.cpp tools/RenderTest/WinCapture.cpp)
    target_compile_definitions(RenderTest PRIVATE
        JUCE_PLUGINHOST_VST3=1
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)
    target_link_libraries(RenderTest PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_recommended_config_flags)
endif()

if(DESIDUOUS_BUILD_TESTS)
    add_executable(NoiseTest tools/NoiseTest/main.cpp)
    target_link_libraries(NoiseTest PRIVATE residfp)
endif()
