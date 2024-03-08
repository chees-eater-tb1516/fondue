
#include "Fonduempeg.h"
#include<iostream>

/*normal constructor*/
InputStream::InputStream(const char* source_url, AVCodecContext* output_codec_ctx)
{
    m_source_url = source_url;
    
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
        m_audio_stream = m_format_ctx->streams[m_audio_stream_idx];
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
InputStream::~InputStream()
{
    cleanup();
}

/*for exiting elegantly*/
void InputStream::cleanup()
{
    avcodec_free_context(&m_input_codec_ctx);
    avformat_close_input(&m_format_ctx);
    av_packet_free(&m_pkt);   
    av_frame_free(&m_tmp_frame);
}

int InputStream::decode_one_frame(AVFrame* common_frame)
{
    int ret = 0;
    if (av_read_frame(m_format_ctx, m_pkt) >=0)
    {
        if (m_pkt->stream_index == m_audio_stream_idx)
        {

        }
    }
    /*return one packet from the input*/
    av_read_frame(m_format_ctx, m_pkt);
    /*send packet to the encoder*/
    ret = avcodec_send_packet(m_input_codec_ctx, m_pkt);
    /*receive one frame from the encoder*/
    ret = avcodec_receive_frame(m_input_codec_ctx, m_tmp_frame);
    if (ret < 0) 
        {
            // those two return values are special and mean there is no output
            // frame available, but there were no errors during decoding
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                return 0;
 
            printf("Error during decoding\n");
            return ret;
        }
    common_frame=m_tmp_frame;
    //std::cout<< *m_tmp_frame->data[0] << "\n";
    av_frame_unref(m_tmp_frame);
    
    
    av_packet_unref(m_pkt);

    return 0;

}

    
