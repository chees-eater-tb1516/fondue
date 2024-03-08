cmake_minimum_required(VERSION 3.5)
project(fondue VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 14)

include(CTest)
enable_testing()

add_subdirectory(lib/FFmpeg)

list(APPEND SOURCES
    src/main.cpp
    #src/demux_decode.cpp
    src/open_codec_context.cpp
    #src/decode_packet.cpp
    #src/output_audio_frame.cpp
    #src/get_format_from_sample_fmt.cpp
    #src/encode_example.cpp
    #src/mux_example.cpp
    src/InputStream.cpp
    src/Fonduempeg.h
)

add_executable(fondue ${SOURCES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

target_link_libraries(fondue FFmpeg)
