cmake_minimum_required(VERSION 3.22)

if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal Mac build" FORCE)
endif()
project(MiniBloberto VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Windows (MSVC; JUCE 9 does not support MinGW): static runtime, so no VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# JUCE: installed package (~/JUCE/install) on macOS; source tree (-DJUCE_SOURCE_DIR=...) on Windows.
set(JUCE_SOURCE_DIR "$ENV{HOME}/JUCE" CACHE PATH "JUCE source tree")
if(WIN32 OR CMAKE_CROSSCOMPILING)
    add_subdirectory("${JUCE_SOURCE_DIR}" JUCE)
else()
    find_package(JUCE 9.0.2 CONFIG REQUIRED HINTS "$ENV{HOME}/JUCE/install")
endif()

if(APPLE)
    set(MB_FORMATS AU VST3 Standalone)
else()
    set(MB_FORMATS VST3 Standalone)
endif()

set(MB_SOURCES
    Source/BeatTracker.cpp
    Source/Brain.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

# Sprite sheets: 8 frames x 10 dances, 110x128 native pixels per cell, one sheet per colour.
juce_add_binary_data(MiniBlobertoArt SOURCES
    resources/Bloberto_Lavender.png
    resources/Bloberto_Bluebell.png
    resources/Bloberto_Periwinkle.png
    resources/Bloberto_Lilac.png)

juce_add_plugin(MiniBloberto
    COMPANY_NAME "Woolley"
    BUNDLE_ID com.woolley.minibloberto
    PLUGIN_MANUFACTURER_CODE Wool
    PLUGIN_CODE Mblb
    FORMATS ${MB_FORMATS}
    PRODUCT_NAME "Mini Bloberto"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon_1024.png"   # his face, 80 px grid at 12x
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE   # Standalone: he listens to the input for the beat
    MICROPHONE_PERMISSION_TEXT "Mini Bloberto listens to the audio input so he can dance to the beat. Nothing is recorded."
    COPY_PLUGIN_AFTER_BUILD FALSE   # install explicitly with tools/install.sh
    VST3_CATEGORIES Fx Tools
    AU_MAIN_TYPE kAudioUnitType_Effect)

target_sources(MiniBloberto PRIVATE ${MB_SOURCES})

target_compile_definitions(MiniBloberto PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MiniBloberto
    PRIVATE
        MiniBlobertoArt
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Offline harness: no DAW needed. Pass-through null test, and a PNG of the window at any
# beat position with any settings.
juce_add_console_app(MiniBlobertoSnapshot PRODUCT_NAME "MiniBlobertoSnapshot")
target_sources(MiniBlobertoSnapshot PRIVATE tools/Snapshot.cpp ${MB_SOURCES})
target_compile_definitions(MiniBlobertoSnapshot PRIVATE
    MB_HEADLESS=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
target_link_libraries(MiniBlobertoSnapshot
    PRIVATE
        MiniBlobertoArt
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
