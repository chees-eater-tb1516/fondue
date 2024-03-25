cmake_minimum_required(VERSION 3.5)
project(fondue VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)

include(CTest)
enable_testing()

add_subdirectory(lib/FFmpeg)

list(APPEND SOURCES
    src/main.cpp
    src/InputStream.cpp
    src/OutputStream.cpp
    src/Fonduempeg.h
    src/misc_functions.cpp
    src/DefaultInputStream.cpp
)

add_executable(fondue ${SOURCES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

target_link_libraries(fondue FFmpeg)
