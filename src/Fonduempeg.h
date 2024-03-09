#ifndef FONDUEMPEG_H
#define FONDUEMPEG_H

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
} 
#include<cstdlib>

int open_codec_context(AVCodecContext** dec_ctx, AVFormatContext* fmt_ctx, enum AVMediaType type, const char* src_filename);
int decode_packet();

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
       
        AVPacket* m_pkt = NULL;
        bool m_got_frame = false;
        int m_ret{};

    public:
        /*normal constructor*/
        InputStream(const char* source_url, AVCodecContext* output_codec_ctx);
        
        /*destructor*/
        ~InputStream();
        

        void cleanup();

        bool decode_one_frame();

        void unref_frame();

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
        int64_t next_pts {0};
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

        //bool encode_one_frame();

        //void unref_frame();



    
};

#endif
