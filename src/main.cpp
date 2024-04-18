#include "Fonduempeg.h"
  
ControlFlags g_flags;
std::mutex flags_mtx;
std::mutex new_source_mtx;

/* takes data from one source and sends it to the output url*/
void continue_streaming (InputStream& source, OutputStream& sink, 
                         std::chrono::_V2::steady_clock::time_point& end_time)
{
    std::unique_lock<std::mutex> lock (flags_mtx);
    while (g_flags.normal_streaming && !g_flags.stop)
    {
        lock.unlock();
        try
        {
            source.get_one_output_frame();
            sink.write_frame(source);
            source.sleep(end_time);        
        }

        catch (const char* exception)
        {
            std::cout<<exception<<": changing to default source\n";
            source = InputStream {sink.get_output_codec_context(), DefaultSourceModes::white_noise};
        }   
        lock.lock();
    }    
}

/*takes data from the current and incoming sources, crossfades them and sends data to the output URL. 
* returns a (rvalue) reference to the incoming stream if the crossfade completes successfully 
* or a (rvalue) reference to the outgoing stream (immeadiately) in case of any errors*/
InputStream&& crossfade (InputStream& source, InputStream& new_source, 
                        OutputStream& sink, std::chrono::_V2::steady_clock::time_point& end_time)
{
    int fade_time_remaining = DEFAULT_FADE_MS;
    const int fade_time = DEFAULT_FADE_MS;
    source.init_crossfade();
    new_source.init_crossfade();
    std::lock_guard<std::mutex> lock (flags_mtx);
    while (fade_time_remaining > 0 && !g_flags.stop)
    {
        /*attempt to decode both input and new input frames and crossfade together*/
        try
        {
            new_source.get_one_output_frame();
            source.crossfade_frame (new_source.get_frame(), fade_time_remaining, fade_time);
            sink.write_frame(source);
            source.sleep(end_time);
        }

        /*if anything fails*/
        catch (const char* exception)
        {
            std::cout<<exception<<": crossfading failed\n";
            return std::move(source);
        }
    }
    new_source.end_crossfade();
    return std::move(new_source);
}

void audio_processing (InputStream &source, InputStream &new_source, 
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


void control(InputStream &new_source, const OutputStream &sink)
{
    std::chrono::duration<int> refresh_interval(1);
    int count {};
    AVDictionary* input_options = NULL;
    const char* input_url = "/home/tb1516/cppdev/fondue/audio_sources/Like_as_the_hart.mp3";
    AVCodecContext output_codec_ctx = sink.get_output_codec_context();
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
    FFMPEGString input_prompt{"-i /home/tb1516/cppdev/fondue/audio_sources/main_theme.mp3"};
    //FFMPEGString input_prompt{"-f alsa -i hw:1,0 -ar 44100 -ac 2"};
    FFMPEGString output_prompt{"test.mp3"};
    //FFMPEGString output_prompt{"-c:a libmp3lame -f mp3 -content_type audio/mpeg icecast://source:mArc0n1@icr-emmental.media.su.ic.ac.uk:8888/radio"}

    avdevice_register_all();
    OutputStream sink{output_prompt};
    InputStream source {};
    InputStream new_source{};
    try
    {    
        source = InputStream {input_prompt, sink.get_output_codec_context(),
                                        SourceTimingModes::realtime, DefaultSourceModes::white_noise};
    }
    catch (const char* exception)
    {
        std::cout<<exception<<": failed to correctly access input, switching to default source\n";
        source = InputStream {sink.get_output_codec_context(), DefaultSourceModes::white_noise};
    }
    
    std::thread audioThread(audio_processing, std::ref(source), std::ref(new_source), std::ref(sink));
    std::thread controlThread(control, std::ref(new_source), sink);

    audioThread.join();
    controlThread.join();

    
    return 0;
}