




class InputStream 
{
    private:
        const char* m_source_url = NULL;
        AVFormatContext* m_format_ctx = NULL;
        AVCodecContext* m_input_codec_ctx = NULL;
        /*needs reference to the output codec context in order to be 
        able to output frames in the correct format*/
        AVCodecContext* m_output_codec_ctx = NULL;
        AVStream* m_audio_stream =  NULL;
        int m_audio_stream_idx {-1};
        AVFrame* m_tmp_frame = NULL;
        /*needs reference to a frame common with the output codec*/
        AVFrame* m_frame = NULL;
        AVPacket* m_pkt = NULL;
        int m_ret{}; 

    public:
        /*normal constructor*/
        InputStream(const char* source_url, AVCodecContext* output_codec_ctx, AVFrame* common_frame);
        
        /*destructor*/
        ~InputStream();
        

        void cleanup();
      

    
};

