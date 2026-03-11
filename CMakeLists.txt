cmake_minimum_required(VERSION 3.21)
project(PogoLoco VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.7
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(PogoLoco
    VERSION 1.0.0
    COMPANY_NAME "ReinhoutDev"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Rgbd
    PLUGIN_CODE Plco
    FORMATS VST3
    PRODUCT_NAME "Pogo Loco"
)

juce_generate_juce_header(PogoLoco)

juce_add_binary_data(PogoAssets SOURCES
    Assets/skin.png
)

target_sources(PogoLoco PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/PitchCurveEditor.cpp
)

target_compile_definitions(PogoLoco PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(PogoLoco
    PRIVATE
        juce::juce_audio_utils
        PogoAssets
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
