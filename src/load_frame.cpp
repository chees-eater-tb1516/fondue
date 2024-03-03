extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
} 
#include<inttypes.h>


bool load_frame(const char* filename, unsigned char** data)
{
    //dynamically allocate an array of pointers to store the frame(s)
    *data = new unsigned char [2 * 3];
    auto ptr = *data;
    for (int x = 0; x < 2; ++x)
    {
        *ptr++ = 'A';
    }
    return true;
}