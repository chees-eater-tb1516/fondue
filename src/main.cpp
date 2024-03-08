

#include "Fonduempeg.h"
AVCodecContext* output_codec_context = NULL;
AVFrame* shared_frame = NULL; 







int main()
{


   
    AVCodecContext* output_codec_context = NULL;
    AVFrame* shared_frame = NULL; 
    InputStream test_input{"/home/tb1516/cppdev/fondue/audio_sources/main_theme.mp3", output_codec_context}; 
    test_input.decode_one_frame(shared_frame);
    return 0;

}
