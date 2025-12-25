cmake_minimum_required(VERSION 3.20)

project(silicon_snooper C OBJC)

set(CMAKE_C_STANDARD 11)

include_directories(include src/core gui)

set(CORE_SOURCES
        src/core/cpu.c
        src/core/gpu.c
        src/core/system_info.c
        src/core/system_metrics.c
        src/core/telemetry.c
        src/core/timeutil.c)

add_library(snooper_core ${CORE_SOURCES})
target_link_libraries(snooper_core
        "-framework CoreFoundation"
        "-framework IOKit")

add_executable(silicon_snooper
        src/cli/main_cli.c
        src/cli/cli_args.c
        src/cli/cli_format_table.c
        src/cli/cli_format_json.c)

target_link_libraries(silicon_snooper snooper_core)

add_executable(silicon_snooper_gui
        gui/gui_main.m
        gui/gui_view.m
        gui/gui_ringbuffer.c
        gui/gui_bridge.c)

set_source_files_properties(gui/gui_main.m gui/gui_view.m PROPERTIES COMPILE_FLAGS "-fobjc-arc")

target_link_libraries(silicon_snooper_gui
        snooper_core
        "-framework Cocoa"
        "-framework CoreFoundation"
        "-framework IOKit")
