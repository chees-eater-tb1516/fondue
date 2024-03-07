# fondue
IC Radio source selection and crossfading application for uninterrupted audio streaming.
Uses libraries from ffmpeg for demultiplexing/ decoding/ filtering / encoding and multiplexing

dependencies:

cmake
pkg-config

libavcodec,
libavformat,
libavfilter,
libavdevice,
libavutil,
libswresample

(depending on the system these libraries may be installed by default when installing ffmpeg with a package manager
however, i had to resort to building ffmpeg from source to be able to get everything i needed https://trac.ffmpeg.org/wiki/CompilationGuide/Ubuntu)

not cross platform! intended to be built in ubuntu/ raspbian (debian)
