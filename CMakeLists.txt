cmake_minimum_required(VERSION 3.20)

project(silicon_snooper C)

set(CMAKE_C_STANDARD 11)

# Includes (Header-Verzeichnis)
include_directories(include)

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

