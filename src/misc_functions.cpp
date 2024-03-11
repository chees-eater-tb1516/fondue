#include"Fonduempeg.h"




char* av_error_to_string(int error_code)
{
    char a[AV_ERROR_MAX_STRING_SIZE] = { 0 };
    return av_make_error_string(a, AV_ERROR_MAX_STRING_SIZE, error_code);
}