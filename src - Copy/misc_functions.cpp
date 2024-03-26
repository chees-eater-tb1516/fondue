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