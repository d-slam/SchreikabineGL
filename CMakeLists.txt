cmake_minimum_required(VERSION 3.22)

project(SchreikabineGL VERSION 0.1)

add_subdirectory("../../juce-8.0.13-windows/JUCE" JUCE)

juce_add_gui_app(SchreikabineGL
    PRODUCT_NAME "SchreikabineGL"
)

juce_generate_juce_header(SchreikabineGL)

target_sources(SchreikabineGL
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/UIComponent.h
	Source/OpenGLScopeView.h

)

target_link_libraries(SchreikabineGL
    PRIVATE
        juce::juce_gui_extra
        juce::juce_audio_utils
        juce::juce_dsp
	juce::juce_opengl
)

target_compile_definitions(SchreikabineGL
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)