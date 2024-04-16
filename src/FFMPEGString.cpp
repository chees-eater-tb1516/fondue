#include"Fonduempeg.h"

FFMPEGString::FFMPEGString(std::string string):
                m_string{string}
{
    

    char *key, *value;

    key = strtok(const_cast<char*>(m_string.c_str()), " ");
    value = strtok(NULL, " ");

    while (key)
    {
        /*special case for the input URL*/
        if (strcmp(key, "-i") == 0)
        {
            m_source_url.append(value);            
        }
        /*special case for the input format*/
        if (strcmp(key, "-f") == 0)
        {
            m_input_format = const_cast<AVInputFormat*>(av_find_input_format(value));
        }
        /*special case for the output URL since it will be last in the string -> value = NULL*/
        if (!value)
        {
            m_destination_url.append(key);
            break;
        }

        /*special case for the audio sample rate*/
        if (strcmp(key, "-r:a") == 0 || strcmp(key, "-ar") == 0)
        {
            m_sample_rate = std::stoi(value);
        }

        /*special case for the audio bit rate*/
        if (strcmp(key, "-b:a") == 0 || strcmp(key, "-ab") == 0)
        {
            m_bit_rate = std::stoi(value);
        }

        /*add the key-value pair to the options dictionary*/
        av_dict_set(&m_options, key + 1, value, 0);
        key = strtok(NULL, " ");
        value = strtok(NULL, " ");
    }
}
