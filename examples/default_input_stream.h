extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
#include <libavutil/avassert.h>
#include<libavutil/audio_fifo.h>
#include<libavfilter/avfilter.h>
#include<libavdevice/avdevice.h>
} 

#include<iostream>
#include<ctime>
#include<chrono>
#include<cmath>
#include<cstdlib>
#include<thread>
#include<functional>
#include<mutex>
#include<cstring>

class DefaultInputStream
{   
    AVCodecContext* m_output_codec_ctx = NULL;
    AVFrame* m_frame = NULL;
    AVFrame* m_temp_frame = NULL;
    struct SwrContext* m_swr_ctx;
    int m_ret{};
    int m_output_data_size;
    std::chrono::duration<double> m_loop_duration {};
    SourceTimingModes m_timing_mode = SourceTimingModes::realtime;

    public:
    /*constructor*/
    DefaultInputStream(AVCodecContext* output_codec_ctx, SourceTimingModes timing_mode);
    /*destructor*/
    ~DefaultInputStream();

    void cleanup();

    bool get_one_output_frame(DefaultSourceModes mode);

    bool get_one_output_frame();

    AVFrame* get_frame() const {return m_frame;}

    void sleep(std::chrono::_V2::steady_clock::time_point &end_time) const;

};
