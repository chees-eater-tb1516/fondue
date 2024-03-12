

#include "Fonduempeg.h"







int main()
{
    int frame_count {};
    
    OutputStream test_output{"test.mp3", NULL, DEFAULT_SAMPLE_RATE, DEFAULT_BIT_RATE};
    InputStream test_input{"/home/tb1516/cppdev/fondue/audio_sources/main_theme.mp3",
                            test_output.get_output_codec_context()}; 
    
   

    while(test_input.get_one_output_frame())
    {
        test_output.set_frame(test_input.get_frame());
        test_output.write_frame();
        frame_count ++;
    }
        
    test_output.finish_streaming();  

    
    
    return 0;

}
