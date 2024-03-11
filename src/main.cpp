

#include "Fonduempeg.h"







int main()
{


   
    AVCodecContext* output_codec_context = NULL;
    AVFrame* shared_frame = NULL; 
    int frame_count {};
    
    OutputStream test_output{"test.wav", NULL, DEFAULT_SAMPLE_RATE, DEFAULT_BIT_RATE};
    InputStream test_input{"/home/tb1516/cppdev/fondue/audio_sources/main_theme.mp3",
                            test_output.get_output_codec_context()}; 
    
   
     
        while(test_input.decode_one_input_frame()>=0)
        {
            
            test_input.resample_one_input_frame();
            test_input.get_one_output_frame();
            shared_frame=test_input.get_frame();
            frame_count ++;
            test_input.unref_frame();
        }
      

    
    
    return 0;

}
