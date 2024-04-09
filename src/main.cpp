#include "Fonduempeg.h"
  
ControlFlags g_flags;
std::mutex flags_mtx;
std::mutex new_source_mtx;
/* takes data from one source and sends it to the output url*/
void continue_streaming (InputStream* source, OutputStream& sink, 
                         std::chrono::_V2::steady_clock::time_point& end_time)
{
    std::unique_lock<std::mutex> lock (flags_mtx);
    while (g_flags.normal_streaming && !g_flags.stop)
    {
        lock.unlock();
        try
        {
            source->get_one_output_frame();
            sink.write_frame(source);
            source->sleep(end_time);          
        }

        catch (const char* exception)
        {
            std::cout<<exception<<": changing to default source\n";
            source->init_default_source();
        }

        lock.lock();
    } 

   
}
/*takes data from the current and incoming sources, crossfades them and sends data to the output URL. 
* returns a pointer to the incoming stream if the crossfade completes successfully or a pointer to the outgoing stream (immeadiately) in case of any errors*/
InputStream* crossfade (InputStream* source, InputStream* new_input, 
                        OutputStream& sink, std::chrono::_V2::steady_clock::time_point& end_time)
{
    int fade_time_remaining = DEFAULT_FADE_MS;
    const int fade_time = DEFAULT_FADE_MS;
    source->init_crossfade();
    new_input->init_crossfade();
    std::lock_guard<std::mutex> lock (flags_mtx);
    while (fade_time_remaining > 0 && !g_flags.stop)
    {
        /*attempt to decode both input and new input frames and crossfade together*/
        try
        {
            new_input->get_one_output_frame();
            source->crossfade_frame (new_input->get_frame(), fade_time_remaining, fade_time);
            sink.write_frame(source);
            source->sleep(end_time);
        }

        /*if anything fails*/
        catch (const char* exception)
        {
            std::cout<<exception<<": crossfading failed\n";
            return source;
        }
    }
    new_input->end_crossfade();
    return new_input;
}

void audio_processing (InputStream* source, InputStream* &new_source, 
                        OutputStream &sink)
{
    std::chrono::_V2::steady_clock::time_point end_time = std::chrono::steady_clock::now();
    std::unique_lock<std::mutex> lock (flags_mtx);
    while (!g_flags.stop)
    {
        if (g_flags.normal_streaming)
        {
            lock.unlock();
            continue_streaming(source, sink, end_time);
            lock.lock();
            continue;
        }
            
        else 
        {  
            lock.unlock();
            std::unique_lock<std::mutex> lock2 (new_source_mtx);
            source = crossfade(source, new_source, sink, end_time);
            lock2.unlock();
            lock.lock();
            g_flags.normal_streaming = true;
        }   
    }    
    sink.finish_streaming();
}


void control(InputStream* &new_source, const OutputStream &sink)
{
    std::chrono::duration<int> refresh_interval(1);
    int count {};
    AVDictionary* input_options = NULL;
    const char* input_url = "/home/tb1516/cppdev/fondue/audio_sources/Like_as_the_hart.mp3";
    AVCodecContext* output_codec_ctx = sink.get_output_codec_context();
    SourceTimingModes timing_mode = SourceTimingModes::realtime; 
    DefaultSourceModes source_mode = DefaultSourceModes::white_noise;

    while (!g_flags.stop)
    {
        /*if (count == 20)
        {
        
            InputStream* temp_ptr {new InputStream(input_url, output_codec_ctx, input_options, timing_mode, source_mode)};
            std::unique_lock<std::mutex> lock2 (new_source_mtx);
            new_source = temp_ptr;
            lock2.unlock();
            std::lock_guard<std::mutex> lock (flags_mtx);
            g_flags.normal_streaming = false;
            lock2.lock();
            //delete temp_ptr;
            //temp_ptr = NULL;
         
        }

        if (count == 60)
        {
            std::lock_guard<std::mutex> lock (flags_mtx);
            g_flags.stop = true;
        }
        count ++;*/
        std::this_thread::sleep_for(refresh_interval);
    }
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
    InputStream test_input("/home/tb1516/fondue/audio_sources/main_theme.mp3", NULL, sink.get_output_codec_context(), input_options, 
                            SourceTimingModes::freetime, DefaultSourceModes::white_noise);
    /*try
    {
        InputStream test_input2("-f alsa -i hw:1,0 -ar 44100 -ac 2", sink, 
                            SourceTimingModes::realtime, DefaultSourceModes::white_noise);
    }
    catch (const char* exception)
    {
        std::cout<<exception<<": failed to correctly access input, using default source\n";
        /*implement 'no input' constructor and call it here*/
    /*}*/
    
    
    
    int x=1;
    InputStream* source = &test_input;
    InputStream* new_source = NULL;
    //flags.normal_streaming=false;
    
    std::thread audioThread(audio_processing, source, std::ref(new_source), std::ref(sink));
    std::thread controlThread(control, std::ref(new_source), sink);

    audioThread.join();
    controlThread.join();

    
    return 0;
}