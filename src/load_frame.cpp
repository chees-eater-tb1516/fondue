bool load_frame(const char* filename, unsigned char** data)
{
    *data = new unsigned char [2 * 3];
    auto ptr = *data;
    for (int x = 0; x < 2; ++x){
        *ptr++ = 5;
    }
    return true;
}