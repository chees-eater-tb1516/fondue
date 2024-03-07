#include<iostream>


bool load_frame(const char* filename, unsigned char** data);
int demux_decode(const char* src_filename, const char* audio_dst_filename);
//note pointer to pointer in function prototype because we are about to dynamically allocate an array of pointers to store the frame(s)
bool save_frame (unsigned char** data);
int encode_example(const char* output_filename);
int mux_example(const char* filename);





int main()
{
    //std::cout << "hello world \n";
    //unsigned char* frame_data;
    /*if(!load_frame("/home/tb1516/cppdev/fondue/audio_sources/NOLA.wav", &frame_data))
    {
        std::cout<<"Couldn't load audio frame \n";
        return 1;
    } 

    if (!save_frame(&frame_data))
    {
        return 1;
    } */
    /*if (demux_decode("/home/tb1516/cppdev/fondue/audio_sources/NOLA.wav", "raw_audio") >0)
    {
        printf("Couldn't demux and decode audio file\n");
        return 1;
    }*/

    //encode_example("out_file.mp2");
    mux_example("noise.mp3");
    


    return 0;

}
