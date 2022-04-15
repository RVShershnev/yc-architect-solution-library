#include <iostream>
#include <fstream>

#include "transcoder-client.h"
#include "transcoder-config.h"
#include "audio-prep-svc/audio_prep_svc.h"
#include "speechkit_asr_svc.h"


using namespace std;


class asr_process_callback : public asr_svc_callback{
public:
    virtual void asr_result(std::string asr_result_json) override{
        std::cout << "ASR results:\n" << asr_result_json << std::endl;
    }

};


class audio_transformer_callback : public  audio_prep_svc_callback{
    public:
    virtual void format_detection_result(std::string detection_result_json)  override
    {
        std::cout << "Media format recognition completed." << detection_result_json << std::endl;
    }

    virtual void preparation_pipeline_compleated(std::string pipeline_result_json)  override
    {
        speechkit_asr_svc asr_svc(options);
        auto callback = std::make_shared<asr_process_callback>();
         asr_svc.start_asr_task(callback, options[CFG_PARAM_AUDIO_SOURCE]);
    }
    private:
};




bool add_option(std::string option_name, std::string option_value){
        options[trim(option_name)] = trim(option_value);
        return !options[option_name].empty();
}

bool parse_config_option(std::string config_option_line){
    std::string config_option_name;

    std::string  s_config_option_line = ltrim(config_option_line);
    
    if (s_config_option_line.length() == 0) {
        std::cout << "Error parsing config line: " << config_option_line << std::endl;
        return false; // False if line is null
    }else if (s_config_option_line.cbegin()[0] == '#') {
        return false; // This is comment line - ignore
    }

    std::istringstream is_option(config_option_line);

    if (std::getline(is_option, config_option_name, '=')) {
        std::string config_option_value;
        if (std::getline(is_option, config_option_value)) {
            return add_option(config_option_name, config_option_value);
        }
    }
    return false;
}

bool parse_config(){
    const char * config_file = options[CFG_PARAM_CONFIG].c_str();
    if (!config_file){
        std::cout << "Config file option required: config=<path_to_cfg_file> " << std::endl;
        return false;
    }
    std::ifstream cfg_file_in(config_file);

    if (cfg_file_in){
        for (std::string line; std::getline(cfg_file_in, line); ) {
            parse_config_option(line);
        }
        return true;
    }else{
        std::cout << "Config file " << config_file << " not found." << std::endl;
        return false;
    }
}

int main(int argc, char** argv)
{
    bool error = false;
	if (argc < 3) {
        error = true;
	}else {
        // Parse command line options
        for (int i = 1; i < argc; i++){
                if (!parse_config_option(argv[i])) {
                    error = true;
                    break;
                }
        }
        if (options.size() < 2 ){
            error = true;
        }else{
            if (!parse_config()) {
                error = true;
            }else{
                auto prepare_audio_callback = std::make_shared<audio_transformer_callback>();
                audio_preparation_svc prepare_svc(options,prepare_audio_callback);
                prepare_svc.discover_audio_format(options[CFG_PARAM_AUDIO_SOURCE]);
                prepare_svc.start_preparation_pipeline(options[CFG_PARAM_AUDIO_SOURCE]);
            }
        }
    }
    if (error) {
        std::cout << "Usage asr-client config=<path_to_cfg_file> audio-source=<uri_to_audio>" << std::endl;
        return -1;
    }else{
        std::cout << "Completed." << std::endl;

        return 0;
    }

}
