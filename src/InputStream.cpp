
#include "Fonduempeg.h"



InputStream::InputStream(const char* source_url, AVInputFormat* format, AVCodecContext* output_codec_ctx, AVDictionary* options, 
                            SourceTimingModes timing_mode, DefaultSourceModes source_mode):
    m_source_url {source_url},
    m_output_codec_ctx {output_codec_ctx},
    m_options {options},
    m_timing_mode {timing_mode},
    m_source_mode {source_mode}
{
    
    m_default_frame_size = DEFAULT_FRAME_SIZE;
    m_frame = alloc_frame(m_output_codec_ctx);
    m_output_frame_size = m_frame->nb_samples;   
    m_swr_ctx_xfade = alloc_resampler(m_output_codec_ctx);

    

    /*open input file and deduce the right format context from the file*/
    if (avformat_open_input(&m_format_ctx, m_source_url, format, &m_options) < 0)
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

    m_pkt = av_packet_alloc();
    
    if (!m_pkt) 
    {
        cleanup();
        throw "Input: could not allocate packet";
    }

    
   

    m_temp_frame = alloc_frame(m_input_codec_ctx);

        /* create resampling contexts */
    m_swr_ctx = alloc_resampler(m_input_codec_ctx, m_output_codec_ctx);

        /* Create the FIFO buffer based on the specified output sample format. */
    if (!(m_queue = av_audio_fifo_alloc(m_output_codec_ctx->sample_fmt,
                                    m_output_codec_ctx->ch_layout.nb_channels, 1))) 
    {
        cleanup();
        throw "Input: failed to allocate audio samples queue";
    }

       
    std::chrono::duration<double> sample_duration (1.0 / m_output_codec_ctx->sample_rate);
    m_loop_duration = (m_output_frame_size-1) * sample_duration;



}

InputStream::InputStream(FFMPEGString &prompt_string, AVCodecContext* output_codec_ctx, 
                        SourceTimingModes timing_mode, DefaultSourceModes source_mode):
                        
                        InputStream(prompt_string.url(), prompt_string.input_format(), output_codec_ctx, prompt_string.options(), timing_mode, source_mode)
{
    /*uses constructor delegation*/
}

   
InputStream::InputStream(AVCodecContext* output_codec_ctx, DefaultSourceModes source_mode):
    m_output_codec_ctx {output_codec_ctx},
    m_source_mode {source_mode}
{
    
    m_timing_mode = SourceTimingModes::realtime;
    m_frame = alloc_frame(m_output_codec_ctx);
    m_output_frame_size = m_frame->nb_samples;
    m_swr_ctx_xfade = alloc_resampler(m_output_codec_ctx);
    m_source_valid = false; 
    AVChannelLayout default_channel_layout = AV_CHANNEL_LAYOUT_STEREO;

    /*allocate the null input frame*/
    m_temp_frame = av_frame_alloc();
    m_temp_frame->format = AV_SAMPLE_FMT_S16;
    av_channel_layout_copy(&m_temp_frame->ch_layout, &default_channel_layout);
    m_temp_frame->sample_rate = m_output_codec_ctx->sample_rate;
    m_temp_frame->nb_samples = m_output_frame_size;

    if (m_output_frame_size)
    {
        if (av_frame_get_buffer(m_temp_frame,0) < 0)
        {
            cleanup();
            throw "Default input: error allocating a temporary audio buffer";
        }
    }

    /*allocate the resampling context*/
    m_swr_ctx = swr_alloc();
    if (!m_swr_ctx) {
        cleanup();
        throw "Default input: could not allocate a resampler context"; 
    }

    /*set the resampling options*/
    av_opt_set_chlayout  (m_swr_ctx, "in_chlayout",       &default_channel_layout,      0);
    av_opt_set_int       (m_swr_ctx, "in_sample_rate",     m_output_codec_ctx->sample_rate,    0);
    av_opt_set_sample_fmt(m_swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16,     0);
    set_resampler_options(m_swr_ctx, m_output_codec_ctx);

    /* initialize the resampling context */
    if ((swr_init(m_swr_ctx)) < 0) 
    {
        cleanup();
        throw "Default input: failed to initialise the resampler context";
    }

    std::chrono::duration<double> sample_duration (1.0 / m_output_codec_ctx->sample_rate);
    m_loop_duration = (m_output_frame_size-1) * sample_duration;

}

InputStream::InputStream()
{
    
}


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

int InputStream::resample_one_input_frame(SwrContext* swr_ctx)
{
    /*note CANNOT deal with sample rate changes, should only be used with crossfade*/
    m_ret=swr_convert(swr_ctx, m_frame->data, m_frame->nb_samples, 
                        (const uint8_t **)m_frame->data, m_frame->nb_samples);
    
   
    return m_ret;
}


bool InputStream::get_one_output_frame()
{
    if (!m_source_valid)
    {
        int i, j, v, fullscale;
        int16_t *q = (int16_t*)m_temp_frame->data[0];

        for (j = 0; j < m_temp_frame->nb_samples; j ++)
        {
            switch (+m_source_mode)
            {
                case +DefaultSourceModes::silence:
                    v = 0;
                    break;
                case +DefaultSourceModes::white_noise:
                /*fullscale = 100 gives quiet white noise*/
                    fullscale=100;
                    v = (static_cast<float>(std::rand())/RAND_MAX -0.5)*fullscale;
                    break;
                default:
                    v = 0;
                    break;
            }
            
            for (i = 0; i < m_temp_frame->ch_layout.nb_channels; i ++)
            {
                *q++ = v;
            }
        }

        /*resample to achieve the output sample format and channel configuration*/

        m_ret = av_frame_make_writable(m_frame);
        /*since not changing the sample rate the number of samples shouldn't change*/
        m_ret = swr_convert(m_swr_ctx, m_frame->data, m_temp_frame->nb_samples,
                        (const uint8_t **)m_temp_frame->data, m_temp_frame->nb_samples);

        if (m_ret < 0)
        {
            printf("error resampling frame: %s\n", av_error_to_string(m_ret));
            cleanup();
            throw "Default input: error resampling frame";
        }
        

        return true;

    }
    
    
    
    
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
        if (decode_one_input_frame_recursive() < 0) 
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



bool InputStream::crossfade_frame(AVFrame* new_input_frame, int& fade_time_remaining, int fade_time)
{
    /*uses pointer arithmetic to linearly fade between two frames, requires AV_SAMPLE_FMT_FLTP*/
    int i , j; 
    float *q , *v;
    get_one_output_frame();

    /*a value between zero and one representing how far through the fade we are currently*/
    float non_dimensional_fade_time = 1 - static_cast<float>(fade_time_remaining)/fade_time;

    float non_dimensional_fade_time_increment = 1 / (static_cast<float>(new_input_frame->sample_rate/1000)*fade_time);

    for (i = 0; i < m_frame->ch_layout.nb_channels; i++)
    {
        q = (float*)m_frame->data[i];
        v = (float*)new_input_frame->data[i];

        for (j = 0; j < m_frame->nb_samples; j++)
        {
            *q = *q *(1-non_dimensional_fade_time) + *v * non_dimensional_fade_time; 
            q++;
            v++;
        }
    }

    resample_one_input_frame(m_swr_ctx_xfade); 
    fade_time_remaining -= get_frame_length_milliseconds();
    return true;
    
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

    if (codec_context->codec)
    {
        if (codec_context->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
            codec_context->frame_size = DEFAULT_FRAME_SIZE;
    }
    
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


void InputStream::init_crossfade()
{
    set_resampler_options(m_swr_ctx);
    /* initialize the resampling context */
    if ((swr_init(m_swr_ctx)) < 0) 
    {
        cleanup();
        throw "crossfading: failed to initialise the resampler context";
    }
}

void InputStream::end_crossfade()
{
    set_resampler_options(m_swr_ctx, m_output_codec_ctx);
    if ((swr_init(m_swr_ctx)) < 0) 
    {
        cleanup();
        throw "end crossfading: failed to initialise the resampler context";
    }
}

void InputStream::init_default_source()
{
    m_source_valid = false;
    AVChannelLayout default_channel_layout = AV_CHANNEL_LAYOUT_STEREO;
    av_opt_set_chlayout  (m_swr_ctx, "in_chlayout",       &default_channel_layout,      0);
    av_opt_set_int       (m_swr_ctx, "in_sample_rate",     m_output_codec_ctx->sample_rate,    0);
    av_opt_set_sample_fmt(m_swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16,     0);

    av_frame_free(&m_temp_frame);
    m_temp_frame = av_frame_alloc();
    m_temp_frame->format = AV_SAMPLE_FMT_S16;
    av_channel_layout_copy(&m_temp_frame->ch_layout, &default_channel_layout);
    m_temp_frame->sample_rate = m_output_codec_ctx->sample_rate;
    m_temp_frame->nb_samples = m_output_frame_size;


    if (m_output_frame_size)
    {
        if (av_frame_get_buffer(m_temp_frame,0) < 0)
        {
            cleanup();
            throw "Default input: error allocating a temporary audio buffer";
        }
    }
    m_ret = swr_init(m_swr_ctx);
    if (m_ret < 0) 
    {
        cleanup();
        throw "switching to default source: failed to initialise the resampler context";
    }

}

void InputStream::set_resampler_options(SwrContext* swr_ctx, AVCodecContext* input_codec_ctx, AVCodecContext* output_codec_ctx)
{
    av_opt_set_chlayout  (swr_ctx, "in_chlayout",       &input_codec_ctx->ch_layout,      0);
    av_opt_set_int       (swr_ctx, "in_sample_rate",     input_codec_ctx->sample_rate,    0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt",      input_codec_ctx->sample_fmt,     0);
    av_opt_set_chlayout  (swr_ctx, "out_chlayout",      &output_codec_ctx->ch_layout,      0);
    av_opt_set_int       (swr_ctx, "out_sample_rate",    output_codec_ctx->sample_rate,    0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt",     output_codec_ctx->sample_fmt,     0);
}

void InputStream::set_resampler_options(SwrContext* swr_ctx)
{
    AVChannelLayout default_channel_layout = AV_CHANNEL_LAYOUT_STEREO;
    av_opt_set_chlayout  (swr_ctx, "out_chlayout",      &default_channel_layout,      0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt",     AV_SAMPLE_FMT_FLTP,     0);
}

void InputStream::set_resampler_options(SwrContext* swr_ctx, AVCodecContext* output_codec_ctx)
{
    av_opt_set_chlayout  (swr_ctx, "out_chlayout",      &output_codec_ctx->ch_layout,      0);
    av_opt_set_int       (swr_ctx, "out_sample_rate",    output_codec_ctx->sample_rate,    0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt",     output_codec_ctx->sample_fmt,     0);
}


struct SwrContext* InputStream::alloc_resampler (AVCodecContext* input_codec_ctx, AVCodecContext* output_codec_ctx)
{
    /*create the resampling context*/
    struct SwrContext* swr_ctx = swr_alloc();
    if (!swr_ctx) {
        cleanup();
        throw "Input: could not allocate a resampler context";
        
    }

    
    set_resampler_options(swr_ctx, input_codec_ctx, output_codec_ctx);

    /* initialize the resampling context */
    if ((swr_init(swr_ctx)) < 0) 
    {
        cleanup();
        throw "Input: failed to initialise the resampler context";
    }

    return swr_ctx;

}

struct SwrContext* InputStream::alloc_resampler (AVCodecContext* output_codec_ctx)
{
    /*create the resampling context*/
    struct SwrContext* swr_ctx = swr_alloc();
    if (!swr_ctx) {
        cleanup();
        throw "Input: could not allocate a resampler context";
        
    }

    AVChannelLayout default_channel_layout = AV_CHANNEL_LAYOUT_STEREO;
    av_opt_set_chlayout  (swr_ctx, "in_chlayout",       &default_channel_layout,      0);
    av_opt_set_int       (swr_ctx, "in_sample_rate",     output_codec_ctx->sample_rate,    0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_FLTP,     0);
    
    set_resampler_options(swr_ctx, output_codec_ctx);
    

    /* initialize the resampling context */
    if ((swr_init(swr_ctx)) < 0) 
    {
        cleanup();
        throw "Input: failed to initialise the resampler context";
    }

    return swr_ctx;

}

int InputStream::get_frame_length_milliseconds()
{
    int rate = m_frame->sample_rate;
    int number = m_frame->nb_samples;
    return number/(rate/1000);
}

void InputStream::sleep(std::chrono::_V2::steady_clock::time_point &end_time) const
{
    fondue_sleep(end_time, m_loop_duration, m_timing_mode);
}









    
