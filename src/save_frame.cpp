#include<fstream>
#include<iostream>
bool save_frame(unsigned char** data)
{
    std::ofstream outf{ "test_frame.txt" };
    


    auto ptr = *data;
    for (int x = 0; x < 2; ++x){
        outf << *ptr++ << '\n';
    }
    /*outf << "this is line 1\n";
    outf << "this is line 2\n"; */

    return true;


}