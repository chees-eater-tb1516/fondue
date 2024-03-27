
#include "Fonduempeg.h"


/*normal constructor*/
InputStream::InputStream(const char* source_url, AVCodecContext* output_codec_ctx, AVDictionary* options)
{
    m_source_url = source_url;
    m_output_codec_ctx = output_codec_ctx;
    m_default_frame_size = DEFAULT_FRAME_SIZE;
    
    /*open input file and deduce the right format context from the file*/
    
    if (avformat_open_input(&m_format_ctx, m_source_url, NULL, &options) < 0)
    {
        throw "Input: couldn't open source";
    }

    /*retrieve stream information from the format context*/
    if (avformat_find_stream_info(m_format_ctx, NULL) < 0)
    {
        throw "Input: could not find stream information";
    }


    /*initialise the codec etc*/
    if (open_codec_context(AVMEDIA_TYPE_AUDIO) < 0)
    {
        throw "Input: could not open codec context";
    }

        
    

    av_dump_format(m_format_ctx, 0, m_source_url, 0);


    m_temp_frame = alloc_frame(m_input_codec_ctx);
    m_frame = alloc_frame(m_output_codec_ctx);
    m_output_frame_size = m_frame->nb_samples;   


    m_pkt = av_packet_alloc();
    if (!m_pkt) 
    {
        cleanup();
        throw "Input: could not allocate packet";
    }


    //printf("Demuxing audio from URL '%s'\n", m_source_url); 

        /* create resampler context */
    m_swr_ctx = swr_alloc();
    if (!m_swr_ctx) {
        cleanup();
        throw "Input: could not allocate a resampler context";
        
    }
 
    /* set options */
    av_opt_set_chlayout  (m_swr_ctx, "in_chlayout",       &m_input_codec_ctx->ch_layout,      0);
    av_opt_set_int       (m_swr_ctx, "in_sample_rate",     m_input_codec_ctx->sample_rate,    0);
    av_opt_set_sample_fmt(m_swr_ctx, "in_sample_fmt",      m_input_codec_ctx->sample_fmt,     0);
    av_opt_set_chlayout  (m_swr_ctx, "out_chlayout",      &m_output_codec_ctx->ch_layout,      0);
    av_opt_set_int       (m_swr_ctx, "out_sample_rate",    m_output_codec_ctx->sample_rate,    0);
    av_opt_set_sample_fmt(m_swr_ctx, "out_sample_fmt",     m_output_codec_ctx->sample_fmt,     0);
 
    /* initialize the resampling context */
    if ((swr_init(m_swr_ctx)) < 0) {
        cleanup();
        throw "Input: failed to initialise the resampler contexr";
    }

        /* Create the FIFO buffer based on the specified output sample format. */
    if (!(m_queue = av_audio_fifo_alloc(m_output_codec_ctx->sample_fmt,
                                      m_output_codec_ctx->ch_layout.nb_channels, 1))) 
    {
        cleanup();
        throw "Input: failed to allocate audio samples queue";
    }

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
    av_frame_free(&m_frame);
    av_frame_free(&m_temp_frame);
    swr_free(&m_swr_ctx);
}

int InputStream::decode_one_input_frame_recursive()
{
    if (!m_got_frame)
    {
        m_ret = av_read_frame(m_format_ctx, m_pkt);
        if (m_ret < 0)
        {
            if (m_ret == AVERROR_EOF)
                return m_ret;
            printf("Error reading a packet from file\n");
            return m_ret;
        }
            

        m_ret = avcodec_send_packet(m_input_codec_ctx, m_pkt);
        if (m_ret < 0)
        {
            printf("Error submitting a packet for decoding\n");
            return m_ret;
        }
    }
    
    if (m_ret >= 0)
    {
        m_ret = avcodec_receive_frame(m_input_codec_ctx, m_temp_frame);

        
        
        if (m_ret < 0) 
        {
            // those two return values are special and mean there is no output
            // frame available, but there were no errors during decoding
            if (m_ret == AVERROR_EOF || m_ret == AVERROR(EAGAIN))
            {
                m_ret = 0;
                m_got_frame = false;
                //get the next packet
                av_packet_unref (m_pkt);
                return decode_one_input_frame_recursive();
            }

            printf("Error during decoding\n");
            return m_ret;
        }
        m_got_frame = true;

        
        return m_ret;

    }

   return m_ret;
}

int InputStream::decode_one_input_frame(SourceTimingModes timing)
{
    
    if ((m_ret = decode_one_input_frame_recursive()) < 0) 
    {
        return m_ret;
    }
    switch(+timing)
    {   
        case +SourceTimingModes::realtime:
            /*work out how much audio is in the frame in units of clock ticks and subtracts a little bit to ensure
            * the loop is always slightly faster than required (too slow leads to dropouts)*/
            m_ticks_per_frame = (m_temp_frame->nb_samples * CLOCKS_PER_SEC)/m_temp_frame->sample_rate - DEFAULT_TIMING_OFFSET;
            /*work out how much processor time has passed since the last frame was decoded and set a sleep time accordingly*/   
            m_sleep_time = get_timespec_from_ticks(m_ticks_per_frame-(std::clock()-m_end_time));
            /*store the time just after the frame was decoded (for the benefit of the next iteration)*/
            m_end_time = std::clock();
            /*put the thread to sleep for the calculated amount of time*/
            nanosleep(&m_sleep_time, NULL);
            break;
        case +SourceTimingModes::freetime:
            m_end_time = std::clock();
            break;

        default:
            m_end_time = std::clock();
            break;
    }
    return 0;
}

int InputStream::resample_one_input_frame()
{
    
    
    m_dst_nb_samples = swr_get_out_samples(m_swr_ctx, m_temp_frame->nb_samples);
    m_frame -> nb_samples = m_dst_nb_samples;

    av_assert0(m_dst_nb_samples == m_frame->nb_samples);
    m_ret=av_frame_make_writable(m_frame);
    m_ret=swr_convert(m_swr_ctx, m_frame->data, m_dst_nb_samples, 
                        (const uint8_t **)m_temp_frame->data, m_temp_frame->nb_samples);
    
    if (m_ret < 0)
    {   
        return m_ret;
    }
    m_actual_nb_samples = m_ret;
    m_frame->nb_samples=m_ret;
    av_frame_unref(m_temp_frame);
    return m_ret;
}

/*decodes and resamples enough data from the input to produce exactly one output sized frame*/
bool InputStream::get_one_output_frame(SourceTimingModes timing)
{
    
    /*if (m_output_codec_ctx->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
    {
        if (decode_one_input_frame() < 0) return false;
        if (resample_one_input_frame() < 0) return false;
        return true;
    }*/

    /*ensure the queue is long enough to contain 1 more input frame than required*/
    if(av_audio_fifo_realloc(m_queue, m_output_frame_size+m_frame->nb_samples) < 0)
    {
        fprintf(stderr, "could not reallocate FIFO\n");
        throw "could not reallocate FIFO";
        return false;
    }


    /*buffer up enough samples to create one output sized frame*/
    while(m_number_buffered_samples < m_output_frame_size)
    {
        if (decode_one_input_frame(timing) < 0) 
        {
            throw "could not decode an input frame";
            return false;
        }    
        if (resample_one_input_frame() < 0)
        {
            throw "could not resample the input frame";
            return false;
        } 
        if (av_audio_fifo_write(m_queue, (void**)m_frame->data, m_frame->nb_samples)< m_frame->nb_samples)
        {
            fprintf(stderr, "could not write data to FIFO\n");
            throw "could not write data to FIFO";
            return false;
        }
        m_number_buffered_samples = av_audio_fifo_size(m_queue);
    }

    m_frame->nb_samples = m_output_frame_size;
    m_ret=av_frame_make_writable(m_frame);

    /*insert the correct number of samples from the queue into the output frame*/
    if (av_audio_fifo_read(m_queue, (void **)m_frame->data, m_frame->nb_samples) < m_frame->nb_samples) 
    {
        fprintf(stderr, "Could not read data from FIFO\n");
        av_frame_free(&m_frame);
        throw "Could not read data from FIFO";
        return false;
    }

    m_number_buffered_samples = av_audio_fifo_size(m_queue);
    
    return true;
  
}



void InputStream::unref_frame()
{
    av_frame_unref(m_frame);
}

int InputStream::open_codec_context(enum AVMediaType type)
{
    int ret;
    AVStream *st{};
    const AVCodec* dec{};
 
    
        st = m_format_ctx->streams[0];
 
        /* find decoder for the stream */
        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec) {
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }
 
        /* Allocate a codec context for the decoder */
        m_input_codec_ctx = avcodec_alloc_context3(dec);
        if (!m_input_codec_ctx) {
            fprintf(stderr, "Failed to allocate the %s codec context\n",
                    av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }
 
        /* Copy codec parameters from input stream to codec context */
        if ((ret = avcodec_parameters_to_context(m_input_codec_ctx, st->codecpar)) < 0) {
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                    av_get_media_type_string(type));
            return ret;
        }
 
        /* Init the decoders */
        if ((ret = avcodec_open2(m_input_codec_ctx, dec, NULL)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
       
    
 
    return 0;

}

AVFrame* InputStream::alloc_frame(AVCodecContext* codec_context)
{
    AVFrame* frame = av_frame_alloc();
    int nb_samples;
    if (!frame) 
    {
        cleanup();
        throw "Input: error allocating an audio frame";
    }
    if (codec_context->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = DEFAULT_FRAME_SIZE;
    else
        nb_samples = codec_context->frame_size;
 
    frame->format = codec_context -> sample_fmt;
    av_channel_layout_copy(&frame->ch_layout, &codec_context->ch_layout);
    frame->sample_rate = codec_context -> sample_rate;
    frame->nb_samples = nb_samples;
 
    if (nb_samples) {
        if (av_frame_get_buffer(frame, 0) < 0) 
        {
            cleanup();
            throw "Input: error allocating an audio buffer";
        }
    }
 
    return frame;
}




    
