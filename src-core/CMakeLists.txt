cmake_minimum_required (VERSION 3.0.0)
include(CheckLibraryExists)
include(CheckIncludeFiles)
include(CheckCSourceCompiles)
include(CMakePushCheckState)
include(CheckCCompilerFlag)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

file(GLOB_RECURSE SatDump_core_CPPS  *.cpp 
                                     modules/common/aec/*.c
)

find_package(PkgConfig)

add_library(satdump_core SHARED ${SatDump_core_CPPS})
target_include_directories(satdump_core PUBLIC .)

# Threads
find_package(Threads REQUIRED)
target_link_libraries(satdump_core PUBLIC Threads::Threads)

# LibDSP
#find_library(DSP_LIBRARY dsp REQUIRED)
target_link_libraries(satdump_core PUBLIC dsp)

# LibCorrect
#find_library(CORRECT_LIBRARY correct REQUIRED)
target_link_libraries(satdump_core PUBLIC correct.dll)

# LibJpeg
#find_library(JPEG_LIBRARY jpeg REQUIRED)
target_link_libraries(satdump_core PUBLIC jpeg)

# FMT
find_package(fmt REQUIRED CONFIG)
target_include_directories(satdump_core PUBLIC ${fmt_INCLUDE_DIRS})
target_link_libraries(satdump_core PUBLIC fmt.dll)

# FMT
#find_package(volk REQUIRED CONFIG)
#target_include_directories(satdump_core PUBLIC ${volk_INCLUDE_DIRS})
target_link_libraries(satdump_core PUBLIC volk.dll)

# LibPNG
if(WIN32 AND NOT MINGW)
    find_package(libpng CONFIG REQUIRED)
else()
    find_package(PNG REQUIRED)
endif()
target_link_libraries(satdump_core PUBLIC libpng16.dll)