#include "ShaderHandler.h"
#include <algorithm>
#include <numeric>

namespace OD{

void _Combine(std::vector<std::vector<std::string>> terms, std::string accum, std::vector<std::string>& combinations){
    bool last = (terms.size() == 1);
    int n = terms[0].size();
    for(int i = 0; i < n; i++){
        std::string item = accum + "_" + terms[0][i];
        if(last){
            combinations.push_back(item);
        } else{
            auto newTerms = terms;
            newTerms.erase(newTerms.begin());
            _Combine(newTerms, item, combinations);
        }
    }
}

std::set<std::string> Split(std::string s, std::string delimiter){
    size_t pos_start = 0, pos_end, delim_len = delimiter.size();
    std::string token;
    std::set<std::string> res;

    while((pos_end = s.find(delimiter, pos_start)) != std::string::npos){
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        if(token.empty() == false) res.insert(token);
    }
    
    std::string _s = s.substr(pos_start);
    if(_s.empty() == false) res.insert(_s);
    return res;
}

std::set<std::string> Split2(std::string s, std::string delimiter){
    using namespace std;

    set<string> res;
    string token = "";
    for (int i = 0; i < s.size(); i++) {
        bool flag = true;
        for (int j = 0; j < delimiter.size(); j++) {
            if (s[i + j] != delimiter[j]) flag = false;
        }
        if (flag) {
            if (token.size() > 0) {
                res.insert(token);
                token = "";
                i += delimiter.size() - 1;
            }
        } else {
            token += s[i];
        }
    }
    res.insert(token);
    return res;
}

std::string GetKey(const std::set<std::string>& keyworlds){
    if(keyworlds.size() == 0) return "";
    return std::accumulate(keyworlds.begin(), keyworlds.end(), std::string(""));
}

ShaderHandler::ShaderHandler(std::string source){
    Ref<Shader> shader = Shader::CreateFromFile(sourcePath);
    Init(shader, source);
}

ShaderHandler::ShaderHandler(Ref<Shader> shader){
    Assert(shader != nullptr);
    Assert(shader->Path() != "Memory");

    Init(shader, shader->Path());
}

void ShaderHandler::Init(Ref<Shader> baseShader, std::string path){
    shaders[""] = baseShader;
    currentShader = baseShader;
    sourcePath = path;

    std::vector<std::vector<std::string>> multCompile;
    std::vector<std::string> combinations;

    for(auto i: baseShader->Pragmas()){
        if(i.size() < 2) continue;

        //LogInfo("!!!=>>> %s", i[0].c_str()); 
        //continue;
        
        if(i[0] != "MultiCompile") continue;

        multCompile.push_back(std::vector<std::string>());
        keyworldSpaces.push_back(KeyworldSpace());

        for(int j = 1; j < i.size(); j++){
            multCompile[multCompile.size()-1].push_back(i[j]);
            keyworldSpaces[keyworldSpaces.size()-1].keyworlds.push_back(i[j]);
        }
    }
    for(auto& i: multCompile){
        bool c = false;
        for(auto j: i){
            if(j == "_") c = true;
        }
        if(c == false) i.push_back("_");
    }
    
    if(multCompile.size() < 1) return;

    _Combine(multCompile, std::string(""), combinations);
    //LogInfo("-----------All Shader Varing-------------");
    for(std::string s: combinations){
        std::set<std::string> keywords = Split(s, "_");
        std::string key = GetKey(keywords);

        /*for(auto j: keywords){
            LogInfo("SplitValue: %s", j.c_str());
        }*/
        //LogInfo("Shader Varing Key: \"%s\" Original: \"%s\" KeywordsCount: %zd", key.c_str(), s.c_str(), keywords.size());
        if(shaders.count(key) == false) AddShaderVaring(key, keywords);

        /*std::string shaderVaring = baseShader->Path();
        std::string suffix = ".glsl";
        shaderVaring = shaderVaring.substr(0, shaderVaring.length() - suffix.length());
        shaderVaring += key + ".shader";
        LogInfo("%s", shaderVaring.c_str());*/
    }
}

void ShaderHandler::DisableKeyword(std::string keyword){
    //enabledKeywords.erase(keyword);return;

    for(auto& i: keyworldSpaces){
        for(auto j: i.keyworlds){
            if(j == keyword){
                i.enabledKey = -1;
                break;
            }
        }
    }
}

void ShaderHandler::EnableKeyword(std::string keyword){
    //enabledKeywords.insert(keyword);return;

    for(auto& i: keyworldSpaces){
        int index = 0;
        for(auto j: i.keyworlds){
            if(j == keyword){
                i.enabledKey = index;
                break;
            }
            index += 1;
        }
    }
}

std::set<std::string> ShaderHandler::GetEnabledKeywords(){
    //return enabledKeywords;

    std::set<std::string> out;
    for(auto i: keyworldSpaces){
        if(i.enabledKey < 0){
            out.insert("");
        } else {
            out.insert(i.keyworlds[i.enabledKey]);
        }
    }
    return out;
}

void ShaderHandler::SetCurrentShader(){
    UpdateCurrentShader();
}

Ref<Shader> ShaderHandler::GetCurrentShader(){
    return currentShader;
}

void ShaderHandler::UpdateCurrentShader(){
    std::string key = GetKey(GetEnabledKeywords());
    //LogInfo("Key: %s", key.c_str());

    if(shaders.count(key)){
        currentShader = shaders[key];
    } else {
        Assert(false);
        AddShaderVaring(key, GetEnabledKeywords());
    }
}

void ShaderHandler::AddShaderVaring(std::string key, const std::set<std::string>& keywords){
    std::vector<std::string> _enabledKeywords(keywords.begin(), keywords.end());
    Ref<Shader> shader = Shader::CreateFromFile(sourcePath, _enabledKeywords);
    shaders[key] = shader;
    currentShader = shader;
}

}