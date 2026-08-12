cmake_minimum_required(VERSION 3.22)

project(Schwerhoerigkeit VERSION 0.1)

add_subdirectory("../../juce-8.0.13-windows/JUCE" JUCE)

juce_add_gui_app(Schwerhoerigkeit
    PRODUCT_NAME "Schwerhoerigkeit"
)

juce_generate_juce_header(Schwerhoerigkeit)

target_sources(Schwerhoerigkeit
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/UIComponent.h
        Source/MenuComponent.h
)

target_link_libraries(Schwerhoerigkeit
    PRIVATE
        juce::juce_gui_extra
)

target_compile_definitions(Schwerhoerigkeit
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)