#include<iostream>
#include<string>
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
} 

#include<InputStream.h>

AVCodecContext* output_codec_context = NULL;
AVFrame* shared_frame = NULL; 






int main()
{


    char* input_filename = "/home/tb1516/cppdev/fondue/audio_sources/main_theme.mp3";
    InputStream test_input{input_filename, output_codec_context, shared_frame}; 
    return 0;

}
