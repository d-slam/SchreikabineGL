cmake_minimum_required(VERSION 3.22)

project(Schreikabine VERSION 0.1)

add_subdirectory("../../juce-8.0.13-windows/JUCE" JUCE)

juce_add_gui_app(Schreikabine
    PRODUCT_NAME "Schreikabine"
)

juce_generate_juce_header(Schreikabine)

target_sources(Schreikabine
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/UIComponent.h
	    Source/OpenGLDemo2D.h

)

target_link_libraries(Schreikabine
    PRIVATE
        juce::juce_gui_extra
        juce::juce_audio_utils
        juce::juce_dsp
	    juce::juce_opengl
)

target_compile_definitions(Schreikabine
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)