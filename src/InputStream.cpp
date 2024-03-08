extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
} 
#include<cstdlib>

int open_codec_context(AVCodecContext** dec_ctx, AVFormatContext* fmt_ctx, enum AVMediaType type, const char* src_filename);



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
        int m_audio_stream_idx {-1};
        AVFrame* m_tmp_frame = NULL;
        /*needs reference to a frame common with the output codec*/
        AVFrame* m_frame = NULL;
        AVPacket* m_pkt = NULL;
        int m_ret{}; 

    public:
        /*normal constructor*/
        InputStream(const char* source_url, AVCodecContext* output_codec_ctx, AVFrame* common_frame)
        {
            m_source_url = source_url;
            m_frame = common_frame;
            /*open input file and deduce the right format context from the file*/
            
            if (avformat_open_input(&m_format_ctx, m_source_url, NULL, NULL) < 0)
            {
                fprintf(stderr, "Couldn't open source file %s\n", m_source_url);
                std::exit(1);
            }

            /*retrieve stream information from the format context*/
            if (avformat_find_stream_info(m_format_ctx, NULL) < 0)
            {
                fprintf(stderr, "Could not find stream information\n");
                std::exit(1);
            }


            /*initialise the codec etc*/
            if (open_codec_context(&m_input_codec_ctx, m_format_ctx, AVMEDIA_TYPE_AUDIO, m_source_url) >= 0) 
            {
                m_audio_stream = m_format_ctx->streams[0];
            } 

            av_dump_format(m_format_ctx, 0, m_source_url, 0);

            if (!m_audio_stream) 
            {
                fprintf(stderr, "Could not find audio stream in the input, aborting\n");
                cleanup();
                std::exit(1);
            }    

            m_tmp_frame = av_frame_alloc();
            if (!m_tmp_frame) 
            {
                fprintf(stderr, "Could not allocate frame\n");
                cleanup();
                std::exit(1);
                
            }

            m_pkt = av_packet_alloc();
            if (!m_pkt) 
            {
                fprintf(stderr, "Could not allocate packet\n");
                cleanup();
                std::exit(1);
            }

             if (m_audio_stream)
                printf("Demuxing audio from URL '%s'\n", m_source_url); 
        }
        /*destructor*/
        ~InputStream()
        {
            cleanup();
        }

        void cleanup()
        {
            avcodec_free_context(&m_input_codec_ctx);
            avformat_close_input(&m_format_ctx);
            av_packet_free(&m_pkt);   
            av_frame_free(&m_tmp_frame);
        }

    
};
