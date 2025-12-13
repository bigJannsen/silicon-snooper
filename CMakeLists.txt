cmake_minimum_required(VERSION 3.20)

project(silicon_snooper C OBJC)

set(CMAKE_C_STANDARD 11)

# Includes (Header-Verzeichnis)
include_directories(include gui)

add_executable(silicon_snooper
        src/main.c
        src/cpu.c
        src/util.c
        src/deviceinfo.c
        src/telemetry_cpu.c
        src/telemetry_gpu.c
)


# macOS Frameworks für IORegistry + CFString + IOService
target_link_libraries(silicon_snooper
        "-framework CoreFoundation"
        "-framework IOKit"
)

set(GUI_SOURCES
        gui/gui_main.m
        gui/gui_view.m
        gui/gui_ringbuffer.c
        gui/gui_telemetry_bridge.c
        src/telemetry_cpu.c
        src/telemetry_gpu.c
        src/cpu.c
        src/deviceinfo.c
        src/util.c
)

add_executable(silicon_snooper_gui ${GUI_SOURCES})

set_source_files_properties(gui/gui_main.m gui/gui_view.m PROPERTIES COMPILE_FLAGS "-fobjc-arc")

target_link_libraries(silicon_snooper_gui
        "-framework Cocoa"
        "-framework CoreFoundation"
        "-framework IOKit"
)

