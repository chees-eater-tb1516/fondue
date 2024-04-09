/*
* Written by Tom Briggs in 2024
*
* Fonduempeg is a C++ API for audio encoding calling upon the C API of FFmpeg. 
* 
* It aims to provide an easier user experience by handling much of the boilerplate 
* code in the C API within the constructor functions of the classes defined here.  
* It also uses frames as its smallest unit of audio rather the individual samples.
* The frame size is dictated by the codec used but is typically of the order of 1000 samples.
*  
*
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

#define DEFAULT_BIT_RATE 192000
#define DEFAULT_SAMPLE_RATE 44100
#define DEFAULT_FRAME_SIZE 10000
#define DEFAULT_TIMING_OFFSET 1000
#define RADIO_URL "icecast://source:mArc0n1@icr-emmental.media.su.ic.ac.uk:8888/radio"
#define DEFAULT_FADE_MS 2000

enum class DefaultSourceModes {silence, white_noise};

/*overload the unary + operator to cast the enum class DefaultSourceModes to int for e.g. switch statements*/
constexpr auto operator+(DefaultSourceModes m) noexcept
{
    return static_cast<std::underlying_type_t<DefaultSourceModes>>(m);
}

enum class SourceTimingModes {realtime, freetime};

/*overload the unary + operator to cast the enum class SourceTimingModes to int for e.g. switch statements*/
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

void fondue_sleep(std::chrono::_V2::steady_clock::time_point &end_time, const std::chrono::duration<double> &loop_duration, const SourceTimingModes& timing_mode);

class InputStream;

class OutputStream
{
    private:
        const char* m_destination_url = NULL;
        AVFormatContext* m_output_format_context = NULL;
        const AVOutputFormat* m_output_format = NULL; 
        AVCodecContext* m_output_codec_context = NULL;
        const AVCodec* m_output_codec = NULL;
        AVStream* m_audio_stream = NULL;
        int m_samples_count {0};
        int m_ret {0};
        int m_nb_samples{0};
        AVFrame* m_frame;
        AVPacket* m_pkt;
        AVDictionary* m_output_options = NULL;
        int m_sample_rate, m_bit_rate;


    public: 
        /*normal constructor*/
        OutputStream(const char* destination_url, AVDictionary* output_options, 
            int sample_rate, int bit_rate);

        /*destructor*/
        ~OutputStream();

        void cleanup ();

        void set_frame(AVFrame* frame){m_frame = frame;}

        AVCodecContext* get_output_codec_context() const {return m_output_codec_context;}

        int write_frame (InputStream* source);

        void finish_streaming ();

        int get_frame_length_milliseconds();



    
};

class InputStream
{
    private:
        const char* m_source_url = NULL;
        AVFormatContext* m_format_ctx = NULL;
        AVDictionary* m_options;
        AVCodecContext* m_input_codec_ctx = NULL;
        /*needs reference to the output codec context in order to be 
        able to prepare output frames in the correct format*/
        AVCodecContext* m_output_codec_ctx = NULL;
        AVFrame* m_frame = NULL;
        AVFrame* m_temp_frame = NULL;       
        AVPacket* m_pkt = NULL;
        bool m_got_frame = false;
        int m_ret{};
        struct SwrContext* m_swr_ctx;
        struct SwrContext* m_swr_ctx_xfade;
        int m_dst_nb_samples;
        int m_default_frame_size;
        int m_output_frame_size;
        int m_actual_nb_samples;
        AVAudioFifo* m_queue = NULL;
        int m_number_buffered_samples = 0;
        std::chrono::duration<double> m_loop_duration {};
        SourceTimingModes m_timing_mode = SourceTimingModes::realtime;
        bool m_source_valid = true;
        DefaultSourceModes m_source_mode = DefaultSourceModes::white_noise;
        


    public:
        /*normal constructor*/
        InputStream(const char* source_url, AVInputFormat* format, AVCodecContext* output_codec_ctx, AVDictionary* options, 
                        SourceTimingModes timing_mode, DefaultSourceModes source_mode);

        /*alternative constructor*/
        InputStream(std::string prompt_string, const OutputStream &output_stream, 
                    SourceTimingModes timing_mode, DefaultSourceModes source_mode);
        
        /*destructor*/
        ~InputStream();
        

        void cleanup();

        /*get one frame of raw audio from the input resource*/
        int decode_one_input_frame_recursive();

        /*convert the sample format, sample rate and channel layout of the input frame to 
        those which the output encoder requires. NOTE the resulting frame will most likely 
        not contain the correct number of samples for the output encoder*/
        int resample_one_input_frame();

        int resample_one_input_frame(SwrContext* swr_ctx);

        bool get_one_output_frame();

        void unref_frame();

        AVFrame* get_frame() const {return m_frame;}

        int open_codec_context(enum AVMediaType type);

        AVFrame* alloc_frame(AVCodecContext* codec_context);

        struct SwrContext* alloc_resampler (AVCodecContext* input_codec_ctx, AVCodecContext* output_codec_ctx);

        struct SwrContext* alloc_resampler(AVCodecContext* input_codec_ctx);

        void set_resampler_options(SwrContext* swr_ctx, AVCodecContext* input_codec_ctx, AVCodecContext* output_codec_ctx);

        void set_resampler_options(SwrContext* swr_ctx);

        void set_resampler_options(SwrContext* swr_ctx, AVCodecContext* output_codec_ctx);

        /*configure resamplers for crossfading*/
        void init_crossfade();

        /*return resamplers to normal streaming settings*/
        void end_crossfade();

        void init_default_source();

        bool get_source_valid() const {return m_source_valid;}

        bool crossfade_frame(AVFrame* new_input_frame, int& fade_time_remaining, int fade_time);

        int get_frame_length_milliseconds();

        void sleep(std::chrono::_V2::steady_clock::time_point &end_time) const;

    
};

#endif
