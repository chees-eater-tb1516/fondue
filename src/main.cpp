#include "Fonduempeg.h"



/* takes data from one source and sends it to the output url. in case of a loss of input data, output data is synthesised to maintain the output stream*/
void continue_streaming (InputStream* source, OutputStream& sink, DefaultInputStream& default_input, ControlFlags* flags, SourceTimingModes& timing_mode)
{
    bool input_valid = true;
    while (flags->normal_streaming && !flags->stop)
    {
        if (input_valid)
        {
            try
            {
                source->get_one_output_frame(timing_mode);
                int x=1;    
            }

            catch (const char* exception)
            {
                std::cout<<exception<<": changing to default source\n";
                default_input.get_one_output_frame(timing_mode);
                sink.set_frame(default_input.get_frame());
                sink.write_frame();
                input_valid = false;
                continue;
            
            }
            sink.set_frame(source->get_frame());
            sink.write_frame();
            continue;

        }

        else
            default_input.get_one_output_frame(timing_mode);
            sink.set_frame(default_input.get_frame());
            sink.write_frame();
        

    } 

   
}
/*takes data from the current and incoming sources, crossfades them and sends data to the output URL. 
* returns a pointer to the incoming stream if the crossfade completes successfully or a pointer to the outgoing stream (immeadiately) in case of any errors*/
InputStream* crossfade (InputStream* source, InputStream* new_input, 
                        OutputStream& sink, DefaultInputStream& default_input, ControlFlags* flags, SourceTimingModes& timing_mode)
{
    int fade_time_remaining = DEFAULT_FADE_MS;

    while (fade_time_remaining > 0 && !flags->stop)
    {
        /*attempt to decode both input and new input frames*/
        try
        {
            new_input->decode_one_input_frame(SourceTimingModes::freetime);
            source->crossfade_frame (new_input->get_frame(), timing_mode);
        }

        /*if either fail*/
        catch (const char* exception)
        {
            std::cout<<exception<<": crossfading failed\n";
            return source;
        }
        
        sink.set_frame(source->get_frame());
        sink.write_frame();
        fade_time_remaining -= sink.get_frame_length_milliseconds();

    }
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
    
    OutputStream sink("test.mp3", output_options, DEFAULT_SAMPLE_RATE, DEFAULT_BIT_RATE);
    InputStream test_input("/home/tb1516/fondue/audio_sources/main_theme.mp3", sink.get_output_codec_context(), input_options);
    InputStream test_input_2("/home/tb1516/fondue/audio_sources/main_theme.mp3", sink.get_output_codec_context(), input_options);
    DefaultInputStream default_input(sink.get_output_codec_context());
    ControlFlags flags;
    InputStream* source = &test_input;
    InputStream* new_source = &test_input_2;
    SourceTimingModes timing_mode = SourceTimingModes::realtime;
    
    
    while (!flags.stop)
    {
        if (flags.normal_streaming)
            continue_streaming(source, sink, default_input, &flags, timing_mode);
        else   
            source = crossfade(source, new_source, sink, default_input, &flags, timing_mode);
    }    
    sink.finish_streaming();

    
    return 0;
}