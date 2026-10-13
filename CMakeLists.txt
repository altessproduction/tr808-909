cmake_minimum_required(VERSION 3.22)

project(TR808_909 VERSION 1.0.0
        DESCRIPTION "TR-808 / TR-909 drum machine VST3 clone (hybrid DSP)"
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ────────────── JUCE ──────────────
set(JUCE_PATH "${CMAKE_SOURCE_DIR}/extern/JUCE")
if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
  message(FATAL_ERROR "JUCE introuvable dans ${JUCE_PATH}. Clonez https://github.com/juce-framework/JUCE.git dans extern/JUCE.")
endif()

add_subdirectory(${JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE_build EXCLUDE_FROM_ALL)

# ─────── Config des formats ────────
juce_add_plugin(TR808_909
  PRODUCT_NAME            "TR808_909"
  COMPANY_NAME            "Z-Labs"
  BUNDLE_ID               "com.zlabs.tr808909"
  PLUGIN_MANUFACTURER_CODE Zlbs
  PLUGIN_CODE             Tr80
  IS_SYNTH                TRUE
  NEEDS_MIDI_INPUT        TRUE
  NEEDS_MIDI_OUTPUT       FALSE
  IS_MIDI_EFFECT          FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
  FORMATS                 VST3 Standalone
  VST3_CATEGORIES         Instrument Synth Drum
  TAIL_LENGTH             0
)

# ───── Sources du plugin ─────
target_sources(TR808_909 PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginProcessor.h
  Source/PluginEditor.cpp
  Source/PluginEditor.h
  Source/dsp/DspCommon.h
  Source/dsp/TR808Voices.h
  Source/dsp/TR909Voices.h
  Source/dsp/DistortionStage.h
  Source/dsp/DrumEngine.h
  Source/sequencer/StepSequencer.h
  Source/params/ParameterManager.h
  Source/midi/MidiLearn.h
)

target_compile_definitions(TR808_909 PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_ALLOW_EXTERNAL_BUILTIN=1
)

target_link_libraries(TR808_909 PRIVATE
  juce_audio_utils
  juce_audio_plugin_client
  juce_dsp
  juce_gui_extra
PUBLIC
  juce::juce_audio_utils
  juce::juce_audio_plugin_client
  juce::juce_dsp
  juce::juce_gui_extra
)

target_include_directories(TR808_909 PRIVATE Source)

# ───── Standalone target settings ─────
if(TARGET TR808_909_Standalone)
  set_target_properties(TR808_909_Standalone PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
endif()
