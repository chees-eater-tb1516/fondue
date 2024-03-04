extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
} 


int open_codec_context(int* audio_stream_idx, AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type, const char* src_filename);
int decode_packet(AVCodecContext* dec, const AVPacket* pkt, AVFrame* frame, int audio_frame_count, FILE* audio_dst_file);

int demux_decode(const char* src_filename, const char* audio_dst_filename)
{
    AVFormatContext* fmt_ctx = NULL;
    AVCodecContext* audio_dec_ctx = NULL;
    AVStream* audio_stream = NULL;
    FILE* audio_dst_file = NULL;
    int audio_stream_idx {-1};
    AVFrame* frame = NULL;
    AVPacket* pkt = NULL;
    int audio_frame_count = 0;
    int ret = 0;
    int x;

    /* open input file and allocate format context, since fmt_ctx is a pointer to NULL, format context 
    is allocated automatically*/

    if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0)
    {
        fprintf(stderr, "Couldn't open source file %s\n", src_filename);
        exit(1);
    }

    /*retrieve stream information*/

    if (avformat_find_stream_info(fmt_ctx, NULL) < 0)
    {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

    if (open_codec_context( &audio_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO, src_filename) >= 0) 
    {
        audio_stream = fmt_ctx->streams[audio_stream_idx];
        audio_dst_file = fopen(audio_dst_filename, "wb");
        if (!audio_dst_file) {
            fprintf(stderr, "Could not open destination file %s\n", audio_dst_filename);
            ret = 1;
            goto end;
        }
    } 

        /* dump input information to stderr */
    av_dump_format(fmt_ctx, 0, src_filename, 0);
 
    if (!audio_stream) 
    {
        fprintf(stderr, "Could not find audio stream in the input, aborting\n");
        ret = 1;
        goto end;
    }    
    
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate frame\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if (audio_stream)
        printf("Demuxing audio from file '%s' into '%s'\n", src_filename, audio_dst_filename);

        /* read frames from the file */
    while (av_read_frame(fmt_ctx, pkt) >= 0) 
    {
        // check if the packet belongs to a stream we are interested in, otherwise
        // skip it
        
        if (pkt->stream_index == audio_stream_idx)
            ret = decode_packet(audio_dec_ctx, pkt, frame, audio_frame_count, audio_dst_file);
        av_packet_unref(pkt);
        if (ret < 0)
            break;
    }



    x=1;

    end:

    return 1;

}