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
        AVFrame* m_tmp_frame = NULL;
       
        AVPacket* m_pkt = NULL;
        int m_ret{}; 

    public:
        /*normal constructor*/
        InputStream(const char* source_url, AVCodecContext* output_codec_ctx);
        
        /*destructor*/
        ~InputStream();
        

        void cleanup();

        int decode_one_frame(AVFrame* common_frame);

        int write_one_frame(AVFrame* common_frame);
      

    
};

#endif
