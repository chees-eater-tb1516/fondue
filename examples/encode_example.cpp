extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
}

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


/* check that a given sample format is supported by the encoder */
static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;
 
    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt)
            return 1;
        p++;
    }
    return 0;
}

/* just pick the highest supported samplerate */
static int select_sample_rate(const AVCodec *codec)
{
    const int *p;
    int best_samplerate = 0;
 
    if (!codec->supported_samplerates)
        return 44100;
 
    p = codec->supported_samplerates;
    while (*p) {
        if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate))
            best_samplerate = *p;
        p++;
    }
    return best_samplerate;
}

/* select layout with the highest channel count */
static int select_channel_layout(const AVCodec* codec, AVChannelLayout* dst)
{
    const AVChannelLayout *p, *best_ch_layout;
    int best_nb_channels   = 0;

   

    AVChannelLayout default_channel_layout = AV_CHANNEL_LAYOUT_STEREO;
    
 
    if (!codec->ch_layouts)
        return av_channel_layout_copy(dst, &default_channel_layout);
 
    p = codec->ch_layouts;
    while (p->nb_channels) {
        int nb_channels = p->nb_channels;
 
        if (nb_channels > best_nb_channels) {
            best_ch_layout   = p;
            best_nb_channels = nb_channels;
        }
        p++;
    }
    return av_channel_layout_copy(dst, best_ch_layout);
}

static void encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt,
                   FILE *output)
{
    int ret;
 
    /* send the frame for encoding */
    ret = avcodec_send_frame(ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending the frame to the encoder\n");
        exit(1);
    }
 
    /* read all the available output packets (in general there may be any
     * number of them */
    while (ret >= 0) {
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error encoding audio frame\n");
            exit(1);
        }
 
        fwrite(pkt->data, 1, pkt->size, output);
        av_packet_unref(pkt);
    }
}


int encode_example(const char* output_filename)
{
    const AVCodec* codec;
    AVCodecContext* codec_ctx =  NULL;
    AVFrame* frame;
    AVPacket* pkt;
    int i, j, k, ret;
    FILE* output_file;
    uint16_t* samples; 
    float t, tincr;

    /*find the encoder, in this case hard coded as the mp2 encoder*/
    codec = avcodec_find_encoder(AV_CODEC_ID_MP2);
    if (!codec)
    {
        fprintf(stderr, "codec not found\n");
        exit(1);
    }

    /*allocate the codec context*/
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx)
    {
        fprintf(stderr, "couldn't allocate audio codec context\n");
        exit(1);
    }

    /* put sample parameters */
    codec_ctx->bit_rate = 64000;
 
    /* check that the encoder supports  the chosen input format (s16 pcm)*/
    codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
    if (!check_sample_fmt(codec, codec_ctx->sample_fmt)) 
    {
        fprintf(stderr, "Encoder does not support sample format %s",
                av_get_sample_fmt_name(codec_ctx->sample_fmt));
        exit(1);
    }

    /* select other audio parameters supported by the encoder */
    codec_ctx->sample_rate    = select_sample_rate(codec);
    ret = select_channel_layout(codec, &codec_ctx->ch_layout);
    if (ret < 0)
        exit(1);

    /* open it */
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    output_file = fopen(output_filename, "wb");
    if (!output_file) {
        fprintf(stderr, "Could not open %s\n", output_filename);
        exit(1);
    }

        /* packet for holding encoded output */
    pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "could not allocate the packet\n");
        exit(1);
    }

        /* frame containing input raw audio */
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate audio frame\n");
        exit(1);
    }

    frame->nb_samples     = codec_ctx->frame_size;
    frame->format         = codec_ctx->sample_fmt;
    ret = av_channel_layout_copy(&frame->ch_layout, &codec_ctx->ch_layout);
    if (ret < 0)
        exit(1);
    
        /* allocate the data buffers */
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate audio data buffers\n");
        exit(1);
    }

        /* encode a single tone sound */
    t = 0;
    tincr = 2 * M_PI * 440.0 / codec_ctx->sample_rate;
    for (i=0; i<200; i++)
    {
        /* make sure the frame is writable -- makes a copy if the encoder
         * kept a reference internally */
        ret = av_frame_make_writable(frame);
        if (ret < 0)
            exit(1);
        samples = (uint16_t*)frame->data[0];
        for (j = 0; j < codec_ctx->frame_size; j++) 
        {
            samples[2*j] = (int)(sin(t) * 10000);
 
            for (k = 1; k < codec_ctx->ch_layout.nb_channels; k++)
                samples[2*j + k] = samples[2*j];
            t += tincr;
        } 
        encode(codec_ctx, frame, pkt, output_file);   

    }

        /* flush the encoder */
    encode(codec_ctx, NULL, pkt, output_file);
 
    fclose(output_file);
 
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&codec_ctx);
 
    return 0;
    
   


}