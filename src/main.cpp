

#include "Fonduempeg.h"
#define DEFAULT_BIT_RATE 128000
#define DEFAULT_SAMPLE_RATE 44100







int main()
{


   
    AVCodecContext* output_codec_context = NULL;
    AVFrame* shared_frame = NULL; 
    int frame_count {};
    
    InputStream test_input{"/home/tb1516/fondue/audio_sources/main_theme.mp3", output_codec_context}; 
    OutputStream test_output{"test.wav", NULL, DEFAULT_SAMPLE_RATE, DEFAULT_BIT_RATE};
   
     
        while(test_input.decode_one_frame())
        {
            
            shared_frame=test_input.get_frame();
            frame_count ++;
            test_input.unref_frame();
        }
      

    
    
    return 0;

}
