extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

int output_audio_frame(AVFrame *frame, int audio_frame_count, FILE* audio_dst_file);

int decode_packet(AVCodecContext* dec, const AVPacket* pkt, AVFrame* frame, int audio_frame_count, FILE* audio_dst_file)
{
    int ret = 0;
 
    // submit the packet to the decoder
    ret = avcodec_send_packet(dec, pkt);
    if (ret < 0) {
        printf("Error submitting a packet for decoding\n");
        return ret;
    }
 
    // get all the available frames from the decoder
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec, frame);
        if (ret < 0) {
            // those two return values are special and mean there is no output
            // frame available, but there were no errors during decoding
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                return 0;
 
            printf("Error during decoding\n");
            return ret;
        }
 
        // write the frame data to output file
        
        ret = output_audio_frame(frame, audio_frame_count, audio_dst_file);
 
        av_frame_unref(frame);
        if (ret < 0)
            return ret;
    }
 
    return 0;
}
