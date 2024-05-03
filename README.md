# fondue
IC Radio source selection and crossfading application for uninterrupted audio streaming.
Uses libraries from ffmpeg for demultiplexing/ decoding/ filtering / encoding and multiplexing

dependencies:

cmake
pkg-config
nlohmann/json

libavcodec,
libavformat,
libavfilter,
libavdevice,
libavutil,
libswresample
(depending on the system these libraries may be installed by default when installing ffmpeg with a package manager
however, i had to resort to building ffmpeg from source to be able to get everything i needed https://trac.ffmpeg.org/wiki/CompilationGuide/Ubuntu)

I have found that package managers such as apt may install old versions of these libraries (depending on what your linux OS version is). Exactly what are the oldest versions that are compatibile with the implementation here is not clear to me. 

I built the libraries using the source code for ffmpeg 7.0 'Dijkstra'. I would suggest you do the same.

not cross platform! intended to be built in ubuntu/ raspbian (debian)
