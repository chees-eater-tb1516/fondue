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
} 

#include<iostream>
#include<ctime>
#include<chrono>
#include<cmath>
#include<cstdlib>

#define DEFAULT_BIT_RATE 192000
#define DEFAULT_SAMPLE_RATE 44100
#define DEFAULT_FRAME_SIZE 10000

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


char* av_error_to_string(int error_code);

struct timespec get_timespec_from_ticks(int ticks);


class InputStream
{
    private:
        const char* m_source_url = NULL;
        AVFormatContext* m_format_ctx = NULL;
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
        int m_dst_nb_samples;
        int m_default_frame_size;
        int m_output_frame_size;
        int m_actual_nb_samples;
        AVAudioFifo* m_queue = NULL;
        int m_number_buffered_samples = 0;
        time_t m_ticks_per_frame {};
        time_t m_end_time {};
        struct timespec m_sleep_time {};

    public:
        /*normal constructor*/
        InputStream(const char* source_url, AVCodecContext* output_codec_ctx, AVDictionary* options);
        
        /*destructor*/
        ~InputStream();
        

        void cleanup();

        /*get one frame of raw audio from the input resource*/
        int decode_one_input_frame_recursive();

        int decode_one_input_frame(SourceTimingModes timing);

        /*convert the sample format, sample rate and channel layout of the input frame to 
        those which the output encoder requires. NOTE the resulting frame will most likely 
        not contain the correct number of samples for the output encoder*/
        int resample_one_input_frame();

        bool get_one_output_frame(SourceTimingModes timing);

        void unref_frame();

        AVFrame* get_frame() const {return m_frame;}

        int open_codec_context(enum AVMediaType type);

        AVFrame* alloc_frame(AVCodecContext* codec_context);
    
};

class DefaultInputStream
{   
    AVCodecContext* m_output_codec_ctx = NULL;
    AVFrame* m_frame = NULL;
    AVFrame* m_temp_frame = NULL;
    struct SwrContext* m_swr_ctx;
    int m_ret{};
    int m_output_data_size;
    time_t m_ticks_per_frame;
    time_t m_end_time;
    struct timespec m_sleep_time {};

    public:
    /*constructor*/
    DefaultInputStream(AVCodecContext* output_codec_ctx);
    /*destructor*/
    ~DefaultInputStream();

    void cleanup();

    bool get_one_output_frame(DefaultSourceModes mode, SourceTimingModes timing);

    bool get_one_output_frame(SourceTimingModes timing);

    AVFrame* get_frame() const {return m_frame;}

};


class OutputStream
{
    private:
        const char* m_destination_url = NULL;
        AVFormatContext* m_output_format_context = NULL;
        const AVOutputFormat* m_output_format; 
        AVCodecContext* m_output_codec_context = NULL;
        const AVCodec* m_output_codec = NULL;
        AVStream* m_audio_stream = NULL;
        int m_samples_count {0};
        int m_ret {0};
        int m_nb_samples{0};
        AVFrame* m_frame;
        AVPacket* m_pkt;
        AVDictionary* m_output_options = NULL;


    public: 
        /*normal constructor*/
        OutputStream(const char* destination_url, AVDictionary* output_options, 
            int sample_rate, int bit_rate);

        /*destructor*/
        ~OutputStream();

        void cleanup ();

        void set_frame(AVFrame* frame){m_frame = frame;}

        AVCodecContext* get_output_codec_context() const {return m_output_codec_context;}

        int write_frame ();

        void finish_streaming ();



    
};



#endif
