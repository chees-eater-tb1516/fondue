
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
} 
#include "InputStream.h"
AVCodecContext* output_codec_context = NULL;
AVFrame* shared_frame = NULL; 






int main()
{


   
    InputStream test_input{"/home/tb1516/cppdev/fondue/audio_sources/main_theme.mp3", output_codec_context, shared_frame}; 
    return 0;

}
