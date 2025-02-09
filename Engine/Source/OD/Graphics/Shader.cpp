#include "Shader.h"
#include "Graphics.h"
#include <algorithm>
#include <numeric>

namespace OD{

void _Combine_(std::vector<std::vector<std::string>> terms, std::string accum, std::vector<std::string>& combinations){
    bool last = (terms.size() == 1);
    int n = terms[0].size();
    for(int i = 0; i < n; i++){
        std::string item = accum + "_" + terms[0][i];
        if(last){
            combinations.push_back(item);
        } else{
            auto newTerms = terms;
            newTerms.erase(newTerms.begin());
            _Combine_(newTerms, item, combinations);
        }
    }
}

std::set<std::string> Split_(std::string s, std::string delimiter){
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

std::set<std::string> Split2_(std::string s, std::string delimiter){
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

std::string GetKey_(const std::set<std::string>& keyworlds){
    if(keyworlds.size() == 0) return "";
    return std::accumulate(keyworlds.begin(), keyworlds.end(), std::string(""));
}

Shader::Shader(std::string path){
    Graphics::ShaderCreate(*this, path);
}

Ref<Shader> Shader::CreateFromFile(const std::string& filepath){
    Ref<Shader> out = CreateRef<Shader>();
    if(Graphics::ShaderCreate(*out, filepath) == false){
        return nullptr;
    }
    return out;
}

bool Shader::LoadFromFile(const std::string& path){
    return Graphics::ShaderCreate(*this, path);
}

std::vector<std::string> Shader::GetFileAssociations(){ 
    return std::vector<std::string>{
        ".shader",
        ".glsl"
    }; 
}

bool Shader::InitPass(int pass){
    //passes[pass].shaders[""] = baseShader;
    //currentShader = baseShader;

    AddShaderVaring("", std::set<std::string>(), pass);
    if(isComplete == false) return false;

    std::vector<std::vector<std::string>> multCompile;
    std::vector<std::string> combinations;

    for(auto i: shaderSourceData.passes[pass].properties /*baseShader->Pragmas()*/){
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
    
    if(multCompile.size() < 1) return true;

    _Combine_(multCompile, std::string(""), combinations);
    //LogInfo("-----------All Shader Varing-------------");
    for(std::string s: combinations){
        std::set<std::string> keywords = Split_(s, "_");
        std::string key = GetKey_(keywords);

        /*for(auto j: keywords){
            LogInfo("SplitValue: %s", j.c_str());
        }*/
        //LogInfo("Shader Varing Key: \"%s\" Original: \"%s\" KeywordsCount: %zd", key.c_str(), s.c_str(), keywords.size());
        if(passes[pass].shaders.count(key) == false) {
            AddShaderVaring(key, keywords, pass);
            if(isComplete == false) return false;
        }

        /*std::string shaderVaring = baseShader->Path();
        std::string suffix = ".glsl";
        shaderVaring = shaderVaring.substr(0, shaderVaring.length() - suffix.length());
        shaderVaring += key + ".shader";
        LogInfo("%s", shaderVaring.c_str());*/
    }

    return true;
}

/*
void Shader::DisableKeyword(std::string keyword){
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

void Shader::EnableKeyword(std::string keyword){
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

void Shader::SetPass(int pass){
    curPass = pass;
}
*/

/*std::set<std::string> Shader::GetEnabledKeywords(){
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
}*/

/*
void Shader::SetCurrentShader(){
    UpdateCurrentShader();
}

Ref<SubShader> Shader::GetCurrentShader(){
    return currentShader;
}
*/

/*void Shader::UpdateCurrentShader(){
    std::string key = GetKey_(GetEnabledKeywords());
    //LogInfo("Key: %s", key.c_str());

    if(passes[curPass].shaders.count(key)){
        currentShader = passes[curPass].shaders[key];
    } else {
        Assert(false);
        AddShaderVaring(key, GetEnabledKeywords(), curPass);
    }
}*/

void Shader::AddShaderVaring(std::string key, const std::set<std::string>& keywords, int pass){
    std::vector<std::string> _enabledKeywords(keywords.begin(), keywords.end());
    _enabledKeywords.push_back(passes[pass].name);

    int startSize = shaderSourceData.baseSource.size();
    int insirtSize = 0;
    for(auto& i: _enabledKeywords){
        std::string keyworld = "#define " + i + "\n";
        shaderSourceData.baseSource.insert(0, keyworld);
        insirtSize += keyworld.size();
    }

    //LogWarning("%s", shaderSourceData.baseSource.c_str());

    Ref<SubShader> shader = CreateRef<SubShader>();
    Graphics::SubShaderCreateFromBaseSource(
        *shader,
        shaderSourceData.baseSource, 
        _enabledKeywords, 
        shaderSourceData.passes[pass].pipeline,
        errors
    );
    if(shader == nullptr){
        isComplete = false;
    }
    passes[pass].shaders[key] = shader;
    //currentShader = shader;

    shaderSourceData.baseSource.erase(0, insirtSize);
    Assert(shaderSourceData.baseSource.size() ==  startSize);
}

}