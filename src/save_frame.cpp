#include<fstream>
#include<iostream>
bool save_frame(const char *out_filename, unsigned char** data)
{
    std::ofstream outf{out_filename};

    if (!outf)
    {
        std::cerr<< "couldn't open file for writing \n";
        return false;
    }

    auto ptr = *data;
    for (int x = 0; x < 2; ++x){
        outf << *ptr++ << '\n';
    }

    return true;


}