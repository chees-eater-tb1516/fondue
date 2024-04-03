#include "Fonduempeg.h"
  

/* takes data from one source and sends it to the output url. in case of a loss of input data, output data is synthesised to maintain the output stream*/
void continue_streaming (InputStream* source, OutputStream& sink, DefaultInputStream& default_input, 
                        ControlFlags* flags, std::chrono::_V2::steady_clock::time_point& end_time)
{
    bool input_valid = true;
    while (flags->normal_streaming && !flags->stop)
    {
        if (input_valid)
        {
            try
            {
                source->get_one_output_frame();
                int x=1;    
            }

            catch (const char* exception)
            {
                std::cout<<exception<<": changing to default source\n";
                default_input.get_one_output_frame();
                sink.set_frame(default_input.get_frame());
                sink.write_frame();
                input_valid = false;
                default_input.sleep(end_time);
                continue;
            
            }
            sink.set_frame(source->get_frame());
            sink.write_frame();
            source->sleep(end_time);
            continue;

        }

        else
            default_input.get_one_output_frame();
            sink.set_frame(default_input.get_frame());
            sink.write_frame();
            default_input.sleep(end_time);
        

    } 

   
}
/*takes data from the current and incoming sources, crossfades them and sends data to the output URL. 
* returns a pointer to the incoming stream if the crossfade completes successfully or a pointer to the outgoing stream (immeadiately) in case of any errors*/
InputStream* crossfade (InputStream* source, InputStream* new_input, 
                        OutputStream& sink, ControlFlags* flags, std::chrono::_V2::steady_clock::time_point& end_time)
{
    int fade_time_remaining = DEFAULT_FADE_MS;
    const int fade_time = DEFAULT_FADE_MS;
    source->init_crossfade();
    new_input->init_crossfade();

    while (fade_time_remaining > 0 && !flags->stop)
    {
        /*attempt to decode both input and new input frames and crossfade together*/
        try
        {
            new_input->get_one_output_frame();
            source->crossfade_frame (new_input->get_frame(), fade_time_remaining, fade_time);
        }

        /*if either fail*/
        catch (const char* exception)
        {
            std::cout<<exception<<": crossfading failed\n";
            return source;
        }
        
        sink.set_frame(source->get_frame());
        sink.write_frame();
        source->sleep(end_time);

    }
    new_input->end_crossfade();
    return new_input;
}



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
    SourceTimingModes timing_mode = SourceTimingModes::realtime;
    OutputStream sink("test.mp3", output_options, DEFAULT_SAMPLE_RATE, DEFAULT_BIT_RATE);
    InputStream test_input("/home/tb1516/cppdev/fondue/audio_sources/Durufle requiem.mp3", sink.get_output_codec_context(), input_options, timing_mode);
    InputStream test_input_2("/home/tb1516/cppdev/fondue/audio_sources/Like_as_the_hart.mp3", sink.get_output_codec_context(), input_options, timing_mode);
    DefaultInputStream default_input(sink.get_output_codec_context(), timing_mode);
    std::chrono::_V2::steady_clock::time_point end_time = std::chrono::steady_clock::now();
    ControlFlags flags;
    InputStream* source = &test_input;
    InputStream* new_source = &test_input_2;
    //flags.normal_streaming=false;
    
    while (!flags.stop)
    {
        if (flags.normal_streaming)
            continue_streaming(source, sink, default_input, &flags, end_time);
        else   
            source = crossfade(source, new_source, sink, &flags, end_time);
            flags.normal_streaming = true;
    }    
    sink.finish_streaming();

    
    return 0;
}