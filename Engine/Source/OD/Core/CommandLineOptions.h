#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace OD{

// The application registers the options provided by its enabled modules.
class CommandLineOptions{
public:
    void Add(const std::string& option, std::string& value){
        options.emplace(option, &value);
    }

    bool HasOptions(const std::vector<std::string>& args) const{
        for(size_t i = 1; i < args.size(); ++i){
            if(options.find(args[i]) != options.end()) return true;
        }
        return false;
    }

    bool Parse(const std::vector<std::string>& args, std::string& error) const{
        for(size_t i = 1; i < args.size(); ++i){
            const auto it = options.find(args[i]);
            if(it == options.end()){
                error = "Unknown option: " + args[i];
                return false;
            }
            if(i + 1 == args.size() || args[i + 1].empty() || args[i + 1].compare(0, 2, "--") == 0){
                error = "Missing value for " + args[i];
                return false;
            }
            if(!it->second->empty()){
                error = "Duplicate option: " + args[i];
                return false;
            }
            *it->second = args[++i];
        }
        return true;
    }

private:
    std::unordered_map<std::string, std::string*> options;
};

}
