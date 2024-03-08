extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
} 
#include<cstdlib>
#include<string>

int open_codec_context(AVCodecContext** dec_ctx, AVFormatContext* fmt_ctx, enum AVMediaType type, const char* src_filename)
{
    int ret;
    AVStream *st{};
    const AVCodec* dec{};
 
    
        st = fmt_ctx->streams[0];
 
        /* find decoder for the stream */
        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec) {
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }
 
        /* Allocate a codec context for the decoder */
        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx) {
            fprintf(stderr, "Failed to allocate the %s codec context\n",
                    av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }
 
        /* Copy codec parameters from input stream to codec context */
        if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                    av_get_media_type_string(type));
            return ret;
        }
 
        /* Init the decoders */
        if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
       
    
 
    return 0;

}


class InputStream 
{
    private:
        char* m_source_url = NULL;
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
        InputStream(char* source_url, AVCodecContext* output_codec_ctx, AVFrame* common_frame)
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

char* input_filename = "hello";
AVCodecContext* output_codec_context = NULL;
AVFrame* shared_frame = NULL;
InputStream test_input{input_filename, output_codec_context, shared_frame};