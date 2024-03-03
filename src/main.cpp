#include<iostream>
#include<fstream>


bool load_frame(const char* filename, unsigned char** data);
bool save_frame(const char *out_filename, unsigned char** data);

//note pointer to pointer in function prototype because we are about to dynamically allocate an array of pointers to store the frame(s)



int main()
{
    std::cout << "hello world \n";
    unsigned char* frame_data;
    if(!load_frame("/home/tb1516/sound/new_main_theme.wav", &frame_data))
    {
        std::cout<<"Couldn't load audio frame \n";
        return 1;
    } 


    return 0;

}
