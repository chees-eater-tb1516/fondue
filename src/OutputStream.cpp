#include "Fonduempeg.h"



/*normal constructor*/
OutputStream::OutputStream(const char* destination_url, AVDictionary* output_options, 
    int sample_rate, int bit_rate)
{
    
    m_destination_url = destination_url;
    m_output_options = output_options;

    /* allocate the output media context , guesses format based on filename*/
    avformat_alloc_output_context2(&m_output_format_context, NULL, NULL, m_destination_url);
    if (!m_output_format_context) 
    {
        /* defaults to mpeg*/
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&m_output_format_context, NULL, "mpeg", m_destination_url);
    }
    if (!m_output_format_context)
    {
        cleanup();
        exit(1);
    }
        

    /*assign things to the output format from the output format context*/
    m_output_format = m_output_format_context->oformat;

    /* Add the audio stream using the default format codecs 
        and initialize the codecs. */
    if (m_output_format->audio_codec != AV_CODEC_ID_NONE)     
    {
        m_output_codec = avcodec_find_encoder(m_output_format->audio_codec);
        if (!(m_output_codec)) 
        {
            fprintf(stderr, "Could not find encoder for '%s'\n",
                    avcodec_get_name(m_output_format->audio_codec));
            cleanup();
            exit(1);
        }

        m_pkt = av_packet_alloc();
        if (!m_pkt) 
        {
            fprintf(stderr, "Could not allocate AVPacket\n");
            cleanup();
            exit(1);
        }

        m_audio_stream = avformat_new_stream(m_output_format_context, NULL);
        if (!m_audio_stream) 
        {
            fprintf(stderr, "Could not allocate stream\n");
            cleanup();
            exit(1);
        }

        m_audio_stream->id = m_output_format_context->nb_streams-1;
        AVCodecContext* c;
        c = avcodec_alloc_context3(m_output_codec);
        if (!c) 
        {
            fprintf(stderr, "Could not alloc an encoding context\n");
            cleanup();
            exit(1);
        }

        m_output_codec_context = c;
        int i;

        m_output_codec_context->sample_fmt  = (m_output_codec)->sample_fmts ?
            (m_output_codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        m_output_codec_context->bit_rate    = bit_rate;
        m_output_codec_context->sample_rate = sample_rate;
        if ((m_output_codec)->supported_samplerates) 
        {
            m_output_codec_context->sample_rate = (m_output_codec)->supported_samplerates[0];
            for (i = 0; (m_output_codec)->supported_samplerates[i]; i++) 
            {
                if ((m_output_codec)->supported_samplerates[i] == sample_rate)
                    m_output_codec_context->sample_rate = sample_rate;
            }
        }
        AVChannelLayout default_channel_layout = AV_CHANNEL_LAYOUT_STEREO;
        av_channel_layout_copy(&c->ch_layout, &default_channel_layout);
        m_audio_stream->time_base = (AVRational){ 1, c->sample_rate };
 
        /* Some formats want stream headers to be separate. */
        if (m_output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
        {
            c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
           

    }

    /* Now that all the parameters are set, we can open the audio 
        codecs and allocate the necessary encode buffers. Also open the thing that deals with resampling
        the input to suit the output*/

    /* copy the options into a temp dictionary*/
    AVDictionary* opt = NULL;
    int ret = 0;
    int nb_samples;
    av_dict_copy(&opt, m_output_options, 0);
    /* open the codec with any required options*/
    ret = avcodec_open2(m_output_codec_context, m_output_codec, &opt);
    /* delete the temp dictionary*/
    av_dict_free(&opt);
    if (ret < 0) 
    {
        fprintf(stderr, "Could not open audio codec\n");
        cleanup();
        exit(1);
    }

    if (m_output_codec_context->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
    /*can anyone explain to me what the bitwise AND operator is doing here?*/
        nb_samples = 10000;
    else
        nb_samples = m_output_codec_context->frame_size;

    /* allocate an empty frame*/
    m_frame = av_frame_alloc();
    if (!m_frame) 
    {
        fprintf(stderr, "Error allocating an audio frame\n");
        cleanup();
        exit(1);
    }

    m_frame->format = m_output_codec_context->sample_fmt;
    av_channel_layout_copy(&m_frame->ch_layout, &m_output_codec_context->ch_layout);
    m_frame->sample_rate = sample_rate;
    m_frame->nb_samples = nb_samples;

    if (nb_samples) {
        if (av_frame_get_buffer(m_frame, 0) < 0) {
            fprintf(stderr, "Error allocating an audio buffer\n");
            cleanup();
            exit(1);
        }
    }

    ret = avcodec_parameters_from_context(m_audio_stream->codecpar, m_output_codec_context);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        cleanup();
        exit(1);
    }

    av_dump_format(m_output_format_context, 0, m_destination_url, 1);

        /* open the output file, if needed */
    if (!(m_output_format->flags & AVFMT_NOFILE)) {
        ret = avio_open(&m_output_format_context->pb, m_destination_url, AVIO_FLAG_WRITE);
        if (ret < 0) 
        {
            fprintf(stderr, "Could not open '%s'\n", m_destination_url);
            cleanup();
            exit(1);
        }
    }

        /* Write the stream header, if any. */
    ret = avformat_write_header(m_output_format_context, &m_output_options);
    if (ret < 0) 
    {
        fprintf(stderr, "Error occurred when opening output file\n");
        cleanup();
        exit(1);
    }
    
}
/*normal destructor*/
OutputStream::~OutputStream()
{
    cleanup();
}




/*for exiting elegantly*/
void OutputStream::cleanup()
{
    avcodec_free_context(&m_output_codec_context);
    av_frame_free(&m_frame);
    av_packet_free(&m_pkt);
}
