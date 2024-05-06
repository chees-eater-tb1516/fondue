/*
* Written by Tom Briggs in 2024
*
* Fonduempeg is a C++ API for audio demuxing/ decoding/ encoding/ muxing calling upon the C API of FFmpeg. 
* 
* It aims to provide an easier user experience by handling much of the boilerplate 
* code in the C API within the constructor functions of the classes defined here.  
* It also uses frames as its smallest unit of audio rather the individual samples.
* The frame size is dictated by the codec used but is typically of the order of 1000 samples.
*  
* InputStream and OutputStream should follow RAII paradigm
*
*/

#ifndef FONDUEMPEG_H
#define FONDUEMPEG_H

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
#include <libavutil/avassert.h>
#include<libavutil/avutil.h>
#include<libavutil/channel_layout.h>
#include<libavutil/audio_fifo.h>
#include<libavfilter/avfilter.h>
#include<libavdevice/avdevice.h>
} 

#include<iostream>
#include<ctime>
#include<chrono>
#include<cmath>
#include<cstdlib>
#include<thread>
#include<functional>
#include<mutex>
#include<cstring>
#include<memory>
#include<utility>

#define DEFAULT_BIT_RATE 192000
#define DEFAULT_SAMPLE_RATE 44100
#define DEFAULT_FRAME_SIZE 10000
#define DEFAULT_FADE_MS 2000
#define DEFAULT_LOOP_TIME_OFFSET_SAMPLES 3

enum class DefaultSourceModes {silence, white_noise};

/*overload the unary + operator to cast the enum class DefaultSourceModes 
* to int for e.g. switch statements*/
constexpr auto operator+(DefaultSourceModes m) noexcept
{
    return static_cast<std::underlying_type_t<DefaultSourceModes>>(m); 
}

enum class SourceTimingModes {realtime, freetime};

/*overload the unary + operator to cast the enum class SourceTimingModes 
* to int for e.g. switch statements*/
constexpr auto operator+(SourceTimingModes m) noexcept
{
    return static_cast<std::underlying_type_t<SourceTimingModes>>(m);
}

struct ControlFlags
{
    bool stop {false};
    bool normal_streaming {true};
};


char* av_error_to_string(int error_code);

struct timespec get_timespec_from_ticks(int ticks);

void fondue_sleep(std::chrono::_V2::steady_clock::time_point &end_time, 
        const std::chrono::duration<double> &loop_duration, const SourceTimingModes& timing_mode);

class InputStream;

class FFMPEGString; 

/* provides methods to encode and mux raw audio data, one frame at a time
* doesn't allocate memory to store audio samples, rather shares a pointer to a frame
* allocated elsewhere, by an instance of InputStream or otherwise*/
class OutputStream
{
    private:
        std::string m_destination_url {};
        AVFormatContext* m_output_format_context {}; 
        const AVOutputFormat* m_output_format {}; 
        AVCodecContext* m_output_codec_context {};
        AVStream* m_audio_stream {};
        int m_samples_count {};
        int m_ret {};
        int m_nb_samples {};
        AVFrame* m_frame {};
        AVPacket* m_pkt {};
        AVDictionary* m_output_options {};
        int m_sample_rate, m_bit_rate;


    public: 
        /*normal constructor*/
        OutputStream(std::string destination_url, AVDictionary* output_options, 
            int sample_rate, int bit_rate);
        
        /*alternative constructor from ffmpeg prompt*/
        OutputStream(FFMPEGString &prompt_string);

        /*destructor*/
        ~OutputStream();

        AVCodecContext& get_output_codec_context() const {return *m_output_codec_context;}

        /*encodes and muxes one frame of audio data*/
        int write_frame (InputStream& source);

        /*closes the file and does other end of stream tasks*/
        void finish_streaming ();

        int get_frame_length_milliseconds();    
};

/*provides methods to demux and decode audio data and provide frames of the correct size, 
* sample rate and sample format for the output stream
* has the capability to synthesise audio data if the input source is invalid
* also has the capability to crossfade to a new source*/
class InputStream
{
    private:
        std::string m_source_url{};
        AVFormatContext* m_format_ctx{};
        AVDictionary* m_options{};
        AVCodecContext* m_input_codec_ctx{};
        AVCodecContext m_output_codec_ctx;
        AVFrame* m_frame{};
        AVFrame* m_temp_frame{};       
        AVPacket* m_pkt{};
        bool m_got_frame = false;
        int m_ret{};
        int m_stream_index{};
        struct SwrContext* m_swr_ctx {};
        struct SwrContext* m_swr_ctx_xfade{};
        int m_dst_nb_samples{};
        int m_default_frame_size{};
        int m_output_frame_size{};
        int m_actual_nb_samples{};
        AVAudioFifo* m_queue{};
        int m_number_buffered_samples{};
        std::chrono::duration<double> m_loop_duration {};
        SourceTimingModes m_timing_mode = SourceTimingModes::realtime;
        bool m_source_valid = true;
        DefaultSourceModes m_source_mode = DefaultSourceModes::white_noise;
        


    public:
        /*CONSTRUCTORS, DESTRUCTORS, ASSIGNMENT OPERATORS
        *
        *
        * 
        * 
        */

        /*normal constructor*/
        InputStream(std::string source_url, AVInputFormat* format, const AVCodecContext& output_codec_ctx, AVDictionary* options, 
                        SourceTimingModes timing_mode, DefaultSourceModes source_mode);

        /*alternative constructor from ffmpeg prompt*/
        InputStream(FFMPEGString &prompt_string, const AVCodecContext& output_codec_ctx, 
                    SourceTimingModes timing_mode, DefaultSourceModes source_mode);

        /*alternative 'no source' constructor*/
        InputStream(const AVCodecContext& output_codec_ctx, DefaultSourceModes source_mode);

        /*default constructor*/
        InputStream();

        /*destructor*/
        ~InputStream();

        /*copy constructor*/
        InputStream(const InputStream& input_stream);

        /*copy assignment operator*/
        InputStream& operator= (const InputStream& input_stream);

        /*move constructor*/
        InputStream(InputStream&& input_stream) noexcept;

        /*move assignment operator*/
        InputStream& operator= (InputStream&& input_stream) noexcept;

        /*METHODS INTENDED FOR API USERS
        *
        *
        * 
        * 
        * 
        */

        /*buffer up exactly one frame that matches all the requirements of the output stream*/
        bool get_one_output_frame();

        /*crossfades two sources, stores one output sized frame, 
        * call multiple times to complete the whole crossfade*/
        bool crossfade_frame(AVFrame* new_input_frame, int& fade_time_remaining, int fade_time);

        /*return a pointer to the output frame*/
        AVFrame* get_frame() const {return m_frame;}

        /*configure resamplers for crossfading*/
        void init_crossfade();

        /*return resamplers to normal streaming settings*/
        void end_crossfade();

        int get_frame_length_milliseconds();

        /*sleeps the thread for the correct amount of time to ensure real time operation*/
        void sleep(std::chrono::_V2::steady_clock::time_point &end_time) const;

        /*METHODS NOT REALLY INTENDED FOR API USERS
        *
        *
        * 
        * 
        */

        /*convert the sample format, sample rate and channel layout of the input frame to 
        those which the output encoder requires. NOTE the resulting frame will most likely 
        not contain the correct number of samples for the output encoder*/
        int resample_one_input_frame();

        /*resample with a specific resampling context*/
        int resample_one_input_frame(SwrContext* swr_ctx);

        /*UNPLEASANT FFMPEG BOILERPLATE ZONE
        *
        *
        * 
        * 
        * 
        */

        /*handles boilerplate of the decoder*/
        int open_codec_context(enum AVMediaType type);

        /*handles boilerplate to do with allocating a frame*/
        AVFrame* alloc_frame(AVCodecContext* codec_context);

        /*handles boilerplate to do with allocating the resampling context 
        * to ensure the input stream data is resampled to match the output stream*/
        struct SwrContext* alloc_resampler (AVCodecContext* input_codec_ctx, AVCodecContext* output_codec_ctx);

        /*allocate a resampling context with default input options for crossfading*/
        struct SwrContext* alloc_resampler(AVCodecContext* output_codec_ctx);

        /*set the resampling context options*/
        void set_resampler_options(SwrContext* swr_ctx, AVCodecContext* input_codec_ctx, AVCodecContext* output_codec_ctx);

        /*set the resampling context options with default output settings*/
        void set_resampler_options(SwrContext* swr_ctx);

        /*set the resampling context output options*/
        void set_resampler_options(SwrContext* swr_ctx, AVCodecContext* output_codec_ctx);

        /*boilerplate for copying resampling contexts*/
        void deepcopy_swr_context(struct SwrContext** dst, struct SwrContext* src);
        
        /*boilerplate for copying audio samples queue*/
        void deepcopy_audio_fifo(AVAudioFifo* src);

        /*boilerplate for copying frames*/
        void deepcopy_frame(AVFrame* &dst, AVFrame* src);

    
};

/*class for parsing ffmpeg prompts*/
class FFMPEGString 
{
    private:
    std::string m_string;
    std::string m_destination_url {""};
    std::string m_source_url {""};
    AVDictionary* m_options = NULL;
    AVInputFormat* m_input_format = NULL;
    int m_sample_rate{DEFAULT_SAMPLE_RATE};
    int m_bit_rate{DEFAULT_BIT_RATE};

    public:

    FFMPEGString(std::string string);
    std::string url(){return (m_source_url!="" ? m_source_url:m_destination_url);}
    int sample_rate(){return m_sample_rate;}
    int bit_rate(){return m_bit_rate;}
    AVDictionary* options(){return m_options;}
    AVInputFormat* input_format(){return m_input_format;}
};

#endif
