#include "Fonduempeg.h"

int main ()
{
    /*options pertaining to the input, generally corresponds to the parts of the ffmpeg input prompt*/
    AVDictionary* input_options = NULL;
    /*options pertaining to the output, generally corresponds to the parts of the ffmpeg output prompt*/
    AVDictionary* output_options = NULL;
    /*hard coded icecast options, uncomment if rqd*/
    //av_dict_set(&output_options, "c:a", "libmp3lame", 0);
    //av_dict_set(&output_options, "f", "mp3", 0);
    //av_dict_set(&output_options, "content_type", "audio/mpeg", 0);
    
    OutputStream test_output("test.mp3", output_options, DEFAULT_SAMPLE_RATE, DEFAULT_BIT_RATE);
    InputStream test_input("/home/tb1516/cppdev/fondue/audio_sources/main_theme.mp3", test_output.get_output_codec_context(), input_options);
    DefaultInputStream default_input(test_output.get_output_codec_context());
    
    InputStream* source = &test_input;
    OutputStream* sink = &test_output;
    SourceTimingModes timing_mode = SourceTimingModes::realtime;
    bool input_valid = true;
    int frame_count = 0;

    



    while (true)
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