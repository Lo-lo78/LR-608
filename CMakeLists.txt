cmake_minimum_required(VERSION 3.22)

project(LR608 VERSION 0.1.0 LANGUAGES C CXX)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_binary_data(LR608Data SOURCES
    "LR-608.jsfx.rpl"
    "Documentation/LR608Help.html"
)

juce_add_plugin(LR608
    COMPANY_NAME "Lo-lo"
    BUNDLE_ID "org.lolo.lr608"
    PLUGIN_MANUFACTURER_CODE LoLo
    PLUGIN_CODE Lr68
    FORMATS VST3
    PRODUCT_NAME "LR-608"
    DESCRIPTION "Accessible drum synthesizer"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(LR608 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/ParameterCatalog.cpp
    Source/ParameterCatalog.h
    Source/PresetManager.cpp
    Source/PresetManager.h
    Source/ScreenReaderAnnouncer.cpp
    Source/ScreenReaderAnnouncer.h
    Source/GeneratedParameters.h
    Source/GeneratedPages.h
    Source/DrumTimingEngine.cpp
    Source/DrumTimingEngine.h
    Source/Kick808Voice.cpp
    Source/Kick808Voice.h
    Source/KickOtherVoices.cpp
    Source/KickOtherVoices.h
    Source/KickEngineBanks.cpp
    Source/KickEngineBanks.h
    Source/SnareEngineBanks.cpp
    Source/SnareEngineBanks.h
    Source/SnareVoice.cpp
    Source/SnareVoice.h
    Source/ClapRimEngineBanks.cpp
    Source/ClapRimEngineBanks.h
    Source/ClapRimVoices.cpp
    Source/ClapRimVoices.h
    Source/TomEngineBanks.cpp
    Source/TomEngineBanks.h
    Source/TomVoice.cpp
    Source/TomVoice.h
    Source/HatCymbalEngineBanks.cpp
    Source/HatCymbalEngineBanks.h
    Source/HatCymbalVoices.cpp
    Source/HatCymbalVoices.h
    Source/SaikeMetal.cpp
    Source/SaikeMetal.h
    Source/MaracasEngineBanks.cpp
    Source/MaracasEngineBanks.h
    Source/MaracasVoice.cpp
    Source/MaracasVoice.h
    Source/CowbellEngineBanks.cpp
    Source/CowbellEngineBanks.h
    Source/CowbellVoice.cpp
    Source/CowbellVoice.h
    Source/ZapEngineBanks.cpp
    Source/ZapEngineBanks.h
    Source/ZapVoice.cpp
    Source/ZapVoice.h
    Source/PolyphonicVoice.cpp
    Source/PolyphonicVoice.h
    Source/OutputStage.cpp
    Source/OutputStage.h
)
target_compile_features(LR608 PRIVATE cxx_std_20)
target_compile_definitions(LR608 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(LR608
    PRIVATE juce::juce_audio_utils
    PUBLIC LR608Data juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags
)
if(WIN32)
    target_link_libraries(LR608 PRIVATE Uiautomationcore OleAut32)
endif()

option(LR608_BUILD_CATALOG_TEST "Build catalogue audit test" ON)
if(LR608_BUILD_CATALOG_TEST)
    add_executable(LR608CatalogTest Tests/CatalogAudit.cpp)
    target_compile_features(LR608CatalogTest PRIVATE cxx_std_20)
    target_include_directories(LR608CatalogTest PRIVATE Source)
    target_link_libraries(LR608CatalogTest PRIVATE juce::juce_core)
    enable_testing()
    add_test(NAME LR608CatalogAudit COMMAND LR608CatalogTest)
    juce_add_console_app(LR608PresetTest PRODUCT_NAME "LR-608 Preset Audit")
    target_sources(LR608PresetTest PRIVATE Tests/PresetAudit.cpp)
    target_compile_features(LR608PresetTest PRIVATE cxx_std_20)
    target_include_directories(LR608PresetTest PRIVATE Source)
    target_link_libraries(LR608PresetTest PRIVATE LR608 juce::juce_audio_utils)
    add_test(NAME LR608PresetAudit COMMAND LR608PresetTest)
    add_executable(LR608TimingTest Tests/TimingAudit.cpp Source/DrumTimingEngine.cpp)
    target_compile_features(LR608TimingTest PRIVATE cxx_std_20)
    target_include_directories(LR608TimingTest PRIVATE Source)
    add_test(NAME LR608TimingAudit COMMAND LR608TimingTest)
    add_executable(LR608VoiceTest Tests/VoiceAudit.cpp Source/Kick808Voice.cpp Source/KickOtherVoices.cpp Source/SnareVoice.cpp Source/ClapRimVoices.cpp Source/TomVoice.cpp Source/HatCymbalVoices.cpp Source/SaikeMetal.cpp Source/MaracasVoice.cpp Source/CowbellVoice.cpp Source/ZapVoice.cpp Source/PolyphonicVoice.cpp Source/OutputStage.cpp)
    target_compile_features(LR608VoiceTest PRIVATE cxx_std_20)
    target_include_directories(LR608VoiceTest PRIVATE Source)
    target_link_libraries(LR608VoiceTest PRIVATE juce::juce_core)
    add_test(NAME LR608VoiceAudit COMMAND LR608VoiceTest)
    add_executable(LR608PerformanceProbe Tests/PerformanceProbe.cpp Source/KickOtherVoices.cpp Source/SnareVoice.cpp Source/SaikeMetal.cpp Source/CowbellVoice.cpp)
    target_compile_features(LR608PerformanceProbe PRIVATE cxx_std_20)
    target_include_directories(LR608PerformanceProbe PRIVATE Source)
    target_link_libraries(LR608PerformanceProbe PRIVATE juce::juce_core juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)
endif()
