cmake_minimum_required(VERSION 3.22)

# Must be set before project(). Universal (Apple Silicon + Intel), macOS 14+. Ignored on other platforms.
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures")
set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum macOS version")

project(CLIP VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# CPM brings in JUCE. Downloaded once into the build dir.
set(CPM_DOWNLOAD_VERSION 0.40.2)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if (NOT EXISTS ${CPM_DOWNLOAD_LOCATION})
    message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
    file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif ()

# Share downloaded packages (JUCE) across build dirs so a clean rebuild doesn't refetch
if (NOT DEFINED CPM_SOURCE_CACHE AND NOT DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_SOURCE_CACHE "$ENV{HOME}/.cache/CPM" CACHE PATH "CPM package download cache")
endif ()

include(${CPM_DOWNLOAD_LOCATION})

CPMAddPackage(
    NAME juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
)

set(SourceFiles
        Source/Parameters.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/dsp/ClipChain.h
        Source/dsp/Clipper.h
        Source/dsp/Oversampler.h
        Source/dsp/PeakFifo.h
        Source/dsp/TransferCurves.h
        Source/ui/Knob.h
        Source/ui/MainPanel.h
)

juce_add_plugin(CLIP
    COMPANY_NAME "GeorgesPlugins"
    BUNDLE_ID "com.georgesplugins.Clip"
    PLUGIN_NAME "CLIP"
    PLUGIN_MANUFACTURER_CODE "Gplg"
    PLUGIN_CODE "Clip"
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "CLIP"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Distortion
)

# Generates JuceHeader.h for the modules linked below
juce_generate_juce_header(CLIP)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SourceFiles})
target_sources(CLIP PRIVATE ${SourceFiles})

target_compile_definitions(CLIP
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(CLIP
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Offline tests (no DAW): ./build/CLIPTests_artefacts/Debug/CLIPTests
juce_add_console_app(CLIPTests PRODUCT_NAME "CLIPTests")
juce_generate_juce_header(CLIPTests)
target_sources(CLIPTests PRIVATE
        Tests/TestMain.cpp
        Tests/ParametersTests.cpp
        Tests/ClipperTests.cpp
        Tests/OversamplerTests.cpp
        Tests/ClipChainTests.cpp
        Tests/ProcessorTests.cpp
        Tests/PeakFifoTests.cpp
        Tests/EditorTests.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)
target_include_directories(CLIPTests PRIVATE Source)
target_compile_definitions(CLIPTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JUCE_ENABLE_ALLOCATION_HOOKS=1 JucePlugin_Name="CLIP")
target_link_libraries(CLIPTests PRIVATE
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
