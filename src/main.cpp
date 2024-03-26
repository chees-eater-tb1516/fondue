#include "Fonduempeg.h"

int main ()
{
    OutputStream test_output("test.mp3", NULL, DEFAULT_SAMPLE_RATE, DEFAULT_BIT_RATE);
    InputStream test_input("/home/tb1516/fondue/audio_sources/main_theme.mp3", test_output.get_output_codec_context(), NULL);
    DefaultInputStream default_input(test_output.get_output_codec_context());
    
    InputStream* source = &test_input;
    OutputStream* sink = &test_output;
    SourceTimingModes timing_mode = SourceTimingModes::freetime;
    bool input_valid = true;
    int frame_count = 0;


    while (frame_count < 3000)
    {
        frame_count ++;
        if (input_valid)
        {
            try
            {
                source->get_one_output_frame(timing_mode);    
            }

            catch (const char* exception)
            {
                std::cout<<exception<<": changing to default source\n";
                default_input.get_one_output_frame(timing_mode);
                sink->set_frame(default_input.get_frame());
                sink->write_frame();
                input_valid = false;
                continue;
            
            }
            sink->set_frame(source->get_frame());
            sink->write_frame();
            continue;

        }

        else
            default_input.get_one_output_frame(timing_mode);
            sink->set_frame(default_input.get_frame());
            sink->write_frame();
        

    } 

    sink->finish_streaming();

    return 0;
}