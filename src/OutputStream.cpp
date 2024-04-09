#include "Fonduempeg.h"



/*normal constructor*/
OutputStream::OutputStream(const char* destination_url, AVDictionary* output_options, 
                            int sample_rate, int bit_rate):

    m_destination_url {destination_url},
    m_output_options {output_options},
    m_sample_rate {sample_rate},
    m_bit_rate {bit_rate}
{
    /*some annoying code to avoid dereferencing a null pointer, surely there is a better way*/
    AVDictionaryEntry* format = av_dict_get(m_output_options, "f", NULL, 0);
    AVDictionaryEntry* content_type = av_dict_get(m_output_options, "content_type", NULL, 0);
    /*if the pointer is NULL, do not dereference, set string to NULL instead*/
    const char* format_s = format ? format->value : NULL;
    const char* content_type_s = content_type ? content_type->value : NULL;
    /*work out the output format from the details passed in: short name, url and mime type*/
    m_output_format = av_guess_format(format_s, 
                                        m_destination_url, content_type_s);

    /* allocate the output media context , guesses format based on filename*/
    avformat_alloc_output_context2(&m_output_format_context, m_output_format, NULL, m_destination_url);

    if (!m_output_format_context)
    {
        cleanup();
        exit(1);
    }


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
     
        m_output_codec_context = avcodec_alloc_context3(m_output_codec);
        if (!m_output_codec_context) 
        {
            fprintf(stderr, "Could not alloc an encoding context\n");
            cleanup();
            exit(1);
        }

   
        int i;

        m_output_codec_context->sample_fmt  =(m_output_codec)->sample_fmts ?
            (m_output_codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        m_output_codec_context->bit_rate    = m_bit_rate;
        m_output_codec_context->sample_rate = DEFAULT_SAMPLE_RATE;
        if ((m_output_codec)->supported_samplerates) 
        {
            for (i = 0; (m_output_codec)->supported_samplerates[i]; i++) 
            {
                if ((m_output_codec)->supported_samplerates[i] == m_sample_rate)
                    m_output_codec_context->sample_rate = m_sample_rate;
            }
        }
        AVChannelLayout default_channel_layout = AV_CHANNEL_LAYOUT_STEREO;
        av_channel_layout_copy(&m_output_codec_context->ch_layout, &default_channel_layout);
        m_audio_stream->time_base = (AVRational){ 1, m_output_codec_context->sample_rate };
 
        /* Some formats want stream headers to be separate. */
        if (m_output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
        {
            m_output_codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
           

    }

    /* Now that all the parameters are set, we can open the audio 
      *  codecs and allocate the necessary encode buffers. Also open the thing that deals with resampling
        * the input to suit the output*/

    /* copy the options into a temp dictionary*/
    AVDictionary* opt = NULL;
    //int ret = 0;
    
    av_dict_copy(&opt, m_output_options, 0);
    /* open the codec with any required options*/
    m_ret = avcodec_open2(m_output_codec_context, m_output_codec, &opt);
    /* delete the temp dictionary*/
    av_dict_free(&opt);
    if (m_ret < 0) 
    {
        fprintf(stderr, "Could not open audio codec\n");
        cleanup();
        exit(1);
    }

    /*ensures a frame with > 0 samples is created even if the codec context has a frame size of zero (implies variable frame size?)*/
    if (m_output_codec_context->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        m_nb_samples = DEFAULT_FRAME_SIZE;
    else
        m_nb_samples = m_output_codec_context->frame_size;


    /* allocate an empty frame*/
    m_frame = av_frame_alloc();
    if (!m_frame) 
    {
        fprintf(stderr, "Error allocating an audio frame\n");
        cleanup();
        exit(1);
    }

    /*copy the frame parameters from the codec context / set them if they need setting*/
    m_frame->format = m_output_codec_context->sample_fmt;
    av_channel_layout_copy(&m_frame->ch_layout, &m_output_codec_context->ch_layout);
    m_frame->sample_rate = DEFAULT_SAMPLE_RATE;
    m_frame->nb_samples = m_nb_samples;

    /*allocate the audio buffers in the frame*/
    /*if (m_nb_samples) {
        if (av_frame_get_buffer(m_frame, 0) < 0) {
            fprintf(stderr, "Error allocating an audio buffer\n");
            cleanup();
            exit(1);
        }
    }*/

    m_ret = avcodec_parameters_from_context(m_audio_stream->codecpar, m_output_codec_context);
    if (m_ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        cleanup();
        exit(1);
    }

    av_dump_format(m_output_format_context, 0, m_destination_url, 1);

        /* open the output file, if needed */
    if (!(m_output_format->flags & AVFMT_NOFILE)) {
        m_ret = avio_open(&m_output_format_context->pb, m_destination_url, AVIO_FLAG_WRITE);
        if (m_ret < 0) 
        {
            fprintf(stderr, "Could not open '%s'\n", m_destination_url);
            cleanup();
            exit(1);
        }
    }

        /* Write the stream header, if any. */
    m_ret = avformat_write_header(m_output_format_context, &m_output_options);
    if (m_ret < 0) 
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


int OutputStream::write_frame()
{
    //av_assert0(m_frame->nb_samples == m_nb_samples);
    m_frame->pts = av_rescale_q(m_samples_count, (AVRational){1, m_output_codec_context->sample_rate},
                                m_output_codec_context->time_base);
    m_samples_count += m_frame -> nb_samples;

    m_ret = avcodec_send_frame(m_output_codec_context, m_frame);

    if (m_ret < 0) 
    {
        fprintf(stderr, "Error sending a frame to the encoder: %s\n",
                av_error_to_string(m_ret));
        cleanup();        
        exit(1);
    }

    while (m_ret >= 0) 
    {
        m_ret = avcodec_receive_packet(m_output_codec_context, m_pkt);
        if (m_ret == AVERROR(EAGAIN) || m_ret == AVERROR_EOF)
            break;
        else if (m_ret < 0) {
            fprintf(stderr, "Error encoding a frame: %s\n", av_error_to_string(m_ret));
            cleanup();
            exit(1);
        }
 
        /* rescale output packet timestamp values from codec to stream timebase */
        av_packet_rescale_ts(m_pkt, m_output_codec_context->time_base, m_audio_stream->time_base);
        m_pkt->stream_index = m_audio_stream->index;
 
        /* Write the compressed frame to the media file. */
        m_ret = av_interleaved_write_frame(m_output_format_context, m_pkt);
        /* pkt is now blank (av_interleaved_write_frame() takes ownership of
         * its contents and resets pkt), so that no unreferencing is necessary.
         * This would be different if one used av_write_frame(). */
        if (m_ret < 0) {
            fprintf(stderr, "Error while writing output packet: %s\n", av_error_to_string(m_ret));
            exit(1);
        }
    }
 
    return m_ret == AVERROR_EOF ? 1 : 0;
}


void OutputStream::finish_streaming()
{
    cleanup();
    if (!(m_output_format->flags & AVFMT_NOFILE))
        /* Close the output file. */
        avio_closep(&m_output_format_context->pb);
 
    /* free the stream */
    avformat_free_context(m_output_format_context);

}

int OutputStream::get_frame_length_milliseconds()
{
    int rate = m_frame->sample_rate;
    int number = m_frame->nb_samples;
    return number/(rate/1000);
}
