#include"Fonduempeg.h"




char* av_error_to_string(int error_code)
{
    char a[AV_ERROR_MAX_STRING_SIZE] = { 0 };
    return av_make_error_string(a, AV_ERROR_MAX_STRING_SIZE, error_code);
}

struct timespec get_timespec_from_ticks(int ticks)
{
    float time = static_cast<float>(ticks) / CLOCKS_PER_SEC;

    int time_seconds = std::floor(time);

    int time_nanoseconds = std::floor((time - time_seconds) * 1e9);

    struct timespec output {time_seconds, time_nanoseconds};

    return output;
}

/*calculates sleep time required and sleeps the thread accordingly, put this function at the end of the loop:
* assumes the iteration start time is equal to the end time of the previous iteration*/
void fondue_sleep(std::chrono::_V2::steady_clock::time_point &end_time, const std::chrono::duration<double> &loop_duration, SourceTimingModes timing_mode)
{
    
    if (timing_mode == SourceTimingModes::realtime)
    {
        std::chrono::duration<double> sleep_time = loop_duration - (std::chrono::steady_clock::now() - end_time);
        std::this_thread::sleep_for(sleep_time);
    }
    
    end_time = std::chrono::steady_clock::now();
}
