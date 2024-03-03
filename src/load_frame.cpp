extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
} 
#include<inttypes.h>
#include<iostream>

//open the audio file
bool load_frame(const char* filename, unsigned char** data_out)
{
    AVFormatContext* av_format_ctx = avformat_alloc_context();
    if (!av_format_ctx)
    {
        std::cerr<<"couldn't allocate format context \n";
        return false;
    }
    if (avformat_open_input(&av_format_ctx, filename, NULL, NULL) != 0)
    {
        std::cerr<<"couldn't open audio file \n";
        return false;
    }
// in this case the file only contains one stream - the audio stream
    
    AVCodecParameters* av_codec_params;
    AVCodec* av_codec;
    av_codec_params = av_format_ctx->streams[0]->codecpar;
    av_codec = avcodec_find_decoder(av_codec_params->codec_id);

  
// lay the foundations to begin decoding the stream

    AVCodecContext* av_codec_ctx= avcodec_alloc_context3(av_codec);
    if(!av_codec_ctx)
    {
        std::cerr<<"couldn't create codec context \n";
        return false;
    }

    if(avcodec_parameters_to_context(av_codec_ctx, av_codec_params) < 0)
    {
        std::cerr << "could't initialise codec context \n";
        return false;
    }

    if (avcodec_open2(av_codec_ctx, av_codec, NULL)< 0)
    {
        std::cout << "couldn't open codec\n";
        return false;
    }

// unpack a single frame
    AVFrame* av_frame = av_frame_alloc();
    if (!av_frame)
    {
        std::cerr << "couldn't allocate frame \n";
        return false;
    } 

    AVPacket* av_packet = av_packet_alloc();
    if (!av_packet)
    {
        std::cerr << "couldn't allocate packet \n";
        return false;
    }
    int response;

    while (av_read_frame(av_format_ctx, av_packet) >=0)
    {
        response = avcodec_send_packet(av_codec_ctx, av_packet);
        if (response < 0)
        {
            std::cerr << "Failed to decode packet \n";
            return false;
        }
        response = avcodec_receive_frame(av_codec_ctx, av_frame);
        if (response == AVERROR(EAGAIN) || AVERROR_EOF)
        {
            continue;
        }

        else if (response < 0)
        {
            std::cerr << "Failed to decode packet \n";
            return false;
        }

        av_packet_unref(av_packet);
        break;


    }

    unsigned char* data = new unsigned char [2 * 3];
    for (int x = 0; x < 10; ++x)
    {
        data[x]=av_frame->data[0][x];
    }

    *data_out = data;


//clean up
    avformat_close_input(&av_format_ctx);
    avformat_free_context(av_format_ctx);
    av_frame_free(&av_frame);
    av_packet_free(&av_packet);
    avcodec_free_context(&av_codec_ctx);


    return true;
} 