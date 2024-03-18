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
#include<cstdlib>
#include<queue>
#include<iostream>
#include<ctime>
#include"unistd.h"

#define DEFAULT_BIT_RATE 192000
#define DEFAULT_SAMPLE_RATE 44100
#define DEFAULT_FRAME_SIZE 10000


char* av_error_to_string(int error_code);

class InputStream 
{
    private:
        const char* m_source_url = NULL;
        AVFormatContext* m_format_ctx = NULL;
        AVCodecContext* m_input_codec_ctx = NULL;
        /*needs reference to the output codec context in order to be 
        able to output frames in the correct format*/
        AVCodecContext* m_output_codec_ctx = NULL;
        AVStream* m_audio_stream =  NULL;
        int m_audio_stream_idx {0};
        AVFrame* m_frame = NULL;
        AVFrame* m_temp_frame = NULL;       
        AVPacket* m_pkt = NULL;
        bool m_got_frame = false;
        int m_ret{};
        int m_output_data_size;
        struct SwrContext* m_swr_ctx;
        int m_dst_nb_samples;
        int m_default_frame_size;
        int m_output_frame_size;
        int m_actual_nb_samples;
        AVAudioFifo* m_queue = NULL;
        std::queue<uint8_t> m_raw_sample_queue;
        int m_number_buffered_samples = 0;
        time_t m_next_frame_time = std::clock();

    public:
        /*normal constructor*/
        InputStream(const char* source_url, AVCodecContext* output_codec_ctx);
        
        /*destructor*/
        ~InputStream();
        

        void cleanup();

        /*get one frame of raw audio from the input resource*/
        int decode_one_input_frame();

        int decode_one_input_frame_realtime();

        /*convert the sample format, sample rate and channel layout of the input frame to 
        those which the output encoder requires. NOTE the resulting frame will most likely 
        not contain the correct number of samples for the output encoder*/
        int resample_one_input_frame();

        bool get_one_output_frame();

        void unref_frame();

        AVFrame* get_frame() const {return m_frame;}

        int open_codec_context(enum AVMediaType type);

        AVFrame* alloc_frame(AVCodecContext* codec_context);
    
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
