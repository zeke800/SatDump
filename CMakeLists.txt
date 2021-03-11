cmake_minimum_required (VERSION 3.0.0)

# ...set this right below the cmake_minimum_required
set(CMAKE_TOOLCHAIN_FILE ./vcpkg/scripts/buildsystems/vcpkg.cmake)

# ...somewhere after the call to project

include_directories(./vcpkg/installed/x64-windows/include)
link_directories(./vcpkg/installed/x64-windows/lib)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    include_directories(/usr/local/include)
    include_directories(/usr/local/opt/jpeg-turbo/include)
    include_directories(/usr/local/Cellar/fftw/3.3.9/include)
    link_directories(/usr/local/lib)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif()

if(UNIX)
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
else()
    set(CMAKE_CXX_FLAGS "-Wall")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
endif()

project (SatDump)

add_subdirectory(src-core)
add_subdirectory(src-cli)
add_subdirectory(src-ui)
add_subdirectory(src-testing)