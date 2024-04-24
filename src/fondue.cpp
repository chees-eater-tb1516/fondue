#include "Fonduempeg.h"
#include <nlohmann/json.hpp>
#include<fstream>
#include<vector>

#define PATH_TO_CONFIG_FILE "/home/icradio/cpp_dev/fondue/config_files/config.json"

using json = nlohmann::json;

std::mutex new_source_mtx;


void continue_streaming (InputStream& source, OutputStream& sink, 
                         std::chrono::_V2::steady_clock::time_point& end_time, ControlFlags& flags);
InputStream&& crossfade (InputStream& source, InputStream& new_source, 
                        OutputStream& sink, std::chrono::_V2::steady_clock::time_point& end_time, ControlFlags& flags);
void audio_processing (InputStream &source, InputStream &new_source, 
                        OutputStream &sink, ControlFlags& flags);
void audio_processing (InputStream &source, InputStream &new_source, 
                        OutputStream &sink, ControlFlags& flags);
void control (InputStream &new_source, const AVCodecContext& 
                        output_codec_ctx, ControlFlags& flags);
bool find_and_remove(std::string& command, const std::string& substring);

std::vector<std::string> split_command_on_whitespace(const std::string& command);

json open_config_file(const std::string& file_path);

void write_config_file(const json& config, const std::string& file_path);


int main ()
{
 
    json config = json::object();
    config = open_config_file(PATH_TO_CONFIG_FILE);
    
    avdevice_register_all();
    std::string default_source_name {config["stream settings"]["default source"]};
    FFMPEGString input_prompt{config["sources"][default_source_name]};
    FFMPEGString output_prompt{config["stream settings"]["test output"]};
    
    
    OutputStream sink{output_prompt};
    InputStream source {};
    InputStream new_source{};
    ControlFlags flags{};
    
    try
    {    
        source = InputStream {input_prompt, sink.get_output_codec_context(),
                                        SourceTimingModes::realtime, DefaultSourceModes::white_noise};
    }
    catch (const char* exception)
    {
        std::cout<<exception<<": failed to correctly access input, switching to default source\n";
        source = InputStream {sink.get_output_codec_context(), DefaultSourceModes::white_noise};
    }
    
    std::thread audioThread(audio_processing, std::ref(source), std::ref(new_source), std::ref(sink), std::ref(flags));
    std::thread controlThread(control, std::ref(new_source), std::ref(sink.get_output_codec_context()), std::ref(flags));

    audioThread.join();
    controlThread.join();

    return 0;
}



void audio_processing (InputStream &source, InputStream &new_source, 
                        OutputStream &sink, ControlFlags& flags)
{
    std::chrono::_V2::steady_clock::time_point end_time = std::chrono::steady_clock::now();

    while (!flags.stop)
    {
        if (flags.normal_streaming)
        {
            continue_streaming(source, sink, end_time, flags);
            continue;
        }            
        else 
        {  
            std::lock_guard<std::mutex> lock (new_source_mtx);
            source = crossfade(source, new_source, sink, end_time, flags);
            flags.normal_streaming = true;
        }    
    }    
    sink.finish_streaming();
}

void control(InputStream &new_source, const AVCodecContext& output_codec_ctx, ControlFlags& flags)
{
    
    SourceTimingModes timing_mode = SourceTimingModes::realtime; 
    DefaultSourceModes source_mode = DefaultSourceModes::white_noise;
    std::string command {};

    while (!flags.stop)
    {
        std::getline(std::cin, command);

        //kill
        if (command == "kill")
        {
            flags.stop = true;
        }
        //list-sources
        else if (command == "list-sources")
        {
            json config = json::object(); 
            config = open_config_file(PATH_TO_CONFIG_FILE);
            for (auto item = config["sources"].begin(); item != config["sources"].end(); ++item)
            {
                std::cout << item.key() << " : " << item.value() << '\n';
            }
        }
        // add-source [source name] [source url]
        else if (find_and_remove(command, "add-source "))
        {
            std::vector<std::string> source_vector {split_command_on_whitespace(command)};
            json config = json::object(); 
            config = open_config_file(PATH_TO_CONFIG_FILE);
            config ["sources"][source_vector[0]] = source_vector[1];
            write_config_file(config, PATH_TO_CONFIG_FILE);
        }

        //delete-source [source name]
        else if (find_and_remove(command, "delete-source "))
        {
            json config = json::object();
            config = open_config_file(PATH_TO_CONFIG_FILE);
            if (find_and_remove(command, "\""))
            {
                size_t position = command.find("\"");
                command=command.substr(0, position);
            }
            if (config["sources"].contains(command))
            {
                config["sources"].erase(command);
                write_config_file(config, PATH_TO_CONFIG_FILE);
            }
            else
            {
                std::cout << "requested source doesn't exist, no sources deleted\n";
            }
        }

        //test-source [source name]
        else if (find_and_remove(command, "test-source "))
        {
            json config = json::object();
            config = open_config_file(PATH_TO_CONFIG_FILE);
            if (find_and_remove(command, "\""))
            {
                size_t position = command.find("\"");
                command=command.substr(0, position);
            }
            if (config["sources"].contains(command))
            {
                FFMPEGString prompt {config["sources"][command]};
                InputStream test_input {};
                try
                {
                    test_input = InputStream{prompt, output_codec_ctx, timing_mode, source_mode};
                    std::cout << "source initialised successfully \n";
                    try 
                    {
                        test_input.get_one_output_frame();
                        std::cout << "data accessed successfully \n";
                    }
                    catch (const char* exception)
                    {
                        std::cout << "couldn't access data: " << exception << '\n';
                    }
                }
                catch(const char* exception)
                {
                    std::cout << "source failed: " << exception << '\n';
                }                
            }
            else
            {
                std::cout << "requested source doesn't exist, no sources tested\n";
            }
        }

        //use-source [source name]
        else if (find_and_remove(command, "use-source "))
        {
            json config = json::object();
            config = open_config_file(PATH_TO_CONFIG_FILE);
            if (find_and_remove(command, "\""))
            {
                size_t position = command.find("\"");
                command=command.substr(0, position);
            }
            if (config["sources"].contains(command))
            {
                FFMPEGString prompt {config["sources"][command]};
            
                try
                {
                    new_source = InputStream{prompt, output_codec_ctx, timing_mode, source_mode};
                }
                catch(const char* exception)
                {
                    std::cout << "source failed: " << exception << '\n';
                } 

                std::cout << "source initialised successfully \n";
                flags.normal_streaming = false;
                config["stream settings"]["active source"] = command;
                write_config_file(config, PATH_TO_CONFIG_FILE);             
            }
            else
            {
                std::cout << "requested source doesn't exist\n";
            }
        }
        //active-source
        else if (command == "active-source")
        {
            json config = json::object();
            config = open_config_file(PATH_TO_CONFIG_FILE);
            std::cout << config["stream settings"]["active source"] << '\n';
        }

        else
        {
            std::cout << "Usage: \nkill: end streaming\n";
            std::cout << "list-sources: list saved audio source names and input prompts\n";
            std::cout << "add-source [source name] [input prompt]: save a new source\n";
            std::cout << "delete-source [source name]: delete a source\n";
            std::cout << "test-source [source name]: verify a source can be opened and can provide audio\n";
            std::cout << "use-source [source name]: switch to streaming from this source\n";
            std::cout << "active-source: return the name of the source currently in use";
        }

    }    
}




/* takes data from one source and sends it to the output url*/
void continue_streaming (InputStream& source, OutputStream& sink, 
                         std::chrono::_V2::steady_clock::time_point& end_time, ControlFlags& flags)
{
    while (flags.normal_streaming && !flags.stop)
    {
        try
        {
            source.get_one_output_frame();
            sink.write_frame(source);
            source.sleep(end_time);        
        }

        catch (const char* exception)
        {
            std::cout<<exception<<": changing to default source\n";
            source = InputStream {sink.get_output_codec_context(), DefaultSourceModes::white_noise};
        }   
    }    
}

/*takes data from the current and incoming sources, crossfades them and sends data to the output URL. 
* returns a (rvalue) reference to the incoming stream if the crossfade completes successfully 
* or a (rvalue) reference to the valid stream (immeadiately) in case of any errors*/
InputStream&& crossfade (InputStream& source, InputStream& new_source, 
                        OutputStream& sink, std::chrono::_V2::steady_clock::time_point& end_time, ControlFlags& flags)
{
    int fade_time_remaining = DEFAULT_FADE_MS;
    const int fade_time = DEFAULT_FADE_MS;
    try
    {
        source.init_crossfade();
        new_source.init_crossfade();
    }

    catch (const char* exception)
    {
        std::cout<<exception<<": crossfading failed\n";
        source.end_crossfade();
        return std::move(source);
    }
       
    while (fade_time_remaining > 0 && !flags.stop)
    {
        /*attempt to decode new input frame*/
        try
        {
            new_source.get_one_output_frame();
        }

        /*if incoming source fails, stop crossfading and return the old source*/
        catch(const char* exception)
        {
            std::cout<<"new source: "<<exception<<": crossfading failed \n";
            source.end_crossfade();
            return std::move(source);
        }
        
        /*attempt to decode input frame and crossfade together*/
        try
        {
            source.crossfade_frame (new_source.get_frame(), fade_time_remaining, fade_time);
        }

        /*if outgoing source fails, replace it with a silent source and continue crossfading*/
        catch (const char* exception)
        {
            std::cout<<"outgoing source: "<<exception<<": switching to default source for remaining fade duration\n";
            source = InputStream(sink.get_output_codec_context(), DefaultSourceModes::silence);
            continue;
        }

        sink.write_frame(source);
        source.sleep(end_time);
    }
    new_source.end_crossfade();
    return std::move(new_source);
}

/*returns true if the substring is at the beginning of the command, then removes the substring from the command*/
bool find_and_remove(std::string& command, const std::string& substring)
{
    std::size_t found = command.find(substring);
    if (found == 0)
    {
        size_t substring_length {substring.length()};
        command = command.substr(substring_length, std::string::npos);
        return true;
    }

    return false;
}

/*splits the string at whitespace characters, ignores whitespaces inside pairs of double quotes*/
std::vector<std::string> split_command_on_whitespace(const std::string& command)
{
    std::vector<std::string> res{};
    std::string internal_string{command};
    size_t position{}, temp_position{};
    while (position < internal_string.size())
    {
        /*ignore white spaces enclosed in double quotes*/
        position = internal_string.find("\"");
        /*check for "" at the beginning of the command*/
        if (position == 0)
        {
            temp_position = internal_string.substr(1, std::string::npos).find("\"");
            if (temp_position == std::string::npos)
            {
                std::cout<<"expected a pair of \"\" but read only one\n";
                break;
            }
               
            res.push_back(internal_string.substr(1, temp_position));
            internal_string.erase(0, temp_position + 3);
            continue;
        }

        position = internal_string.find(" ");
        res.push_back(internal_string.substr(0, position));
        internal_string.erase(0, position + 1);
    }
    return res;
}

/*open the config file and parse it as a json object*/
json open_config_file(const std::string& file_path)
{
    std::ifstream file {file_path};
    json config = json::object();
    config = json::parse(file);
    return config;
}



void write_config_file(const json& config, const std::string& file_path)
{
    std::ofstream file {file_path};
    file << config.dump(1, '\t');
}