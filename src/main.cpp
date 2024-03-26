

#include "Fonduempeg.h"







/*int main()
{
    
    
    OutputStream test_output{"test.mp3", NULL, DEFAULT_SAMPLE_RATE, DEFAULT_BIT_RATE};
    InputStream test_input{"/home/tb1516/cppdev/fondue/audio_sources/main_theme.mp3",
                            test_output.get_output_codec_context(), NULL}; 
    
   

    while(test_input.get_one_output_frame(SourceTimingModes::realtime))
    {
        test_output.set_frame(test_input.get_frame());
        test_output.write_frame();
    }
        
    test_output.finish_streaming();  

    return 0;

}*/


/*int main()
{
    OutputStream test_output("test.mp3", NULL, DEFAULT_SAMPLE_RATE, DEFAULT_BIT_RATE);
    InputStream test_input("/home/tb1516/cppdev/fondue/audio_sources/main_theme.mp3", test_output.get_output_codec_context(), NULL);
    DefaultInputStream default_input(test_output.get_output_codec_context());

    for (int frame_count = 0; frame_count < 1000; frame_count ++)
    {
        default_input.get_one_output_frame(DefaultSourceModes::white_noise, SourceTimingModes::realtime);
        test_output.set_frame(default_input.get_frame());
        test_output.write_frame();
    }

    test_output.finish_streaming();
    return 0;
}*/

int main ()
{
    OutputStream test_output("test.mp3", NULL, DEFAULT_SAMPLE_RATE, DEFAULT_BIT_RATE);
    InputStream test_input("/home/tb1516/cppdev/fondue/audio_sources/main_theme.mp3", test_output.get_output_codec_context(), NULL);
    DefaultInputStream default_input(test_output.get_output_codec_context());
    
    InputStream* source = &test_input;
    OutputStream* sink = &test_output;
    SourceTimingModes timing_mode = SourceTimingModes::realtime;


    while (true)
    {
        try
        {
            source->get_one_output_frame(timing_mode);    
        }

        catch (const char* exception)
        {
            std::cout<<exception<<": changing to default source\n";
            DefaultInputStream* source = &default_input;
            source->get_one_output_frame(timing_mode);
        
        }

    } 

    return 0;
}