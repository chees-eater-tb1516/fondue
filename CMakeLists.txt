cmake_minimum_required(VERSION 3.5)
project(fondue VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 14)

include(CTest)
enable_testing()

add_subdirectory(lib/FFmpeg)

list(APPEND SOURCES
    src/main.cpp
    src/load_frame.cpp
    src/save_frame.cpp
)

add_executable(fondue ${SOURCES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

target_link_libraries(fondue FFmpeg)
