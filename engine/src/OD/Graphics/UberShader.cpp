#include "UberShader.h"
#include <algorithm>
#include <numeric>

namespace OD{

UberShader::UberShader(std::string source){
    sourcePath = source;

    Ref<Shader> shader = Shader::CreateFromFile(sourcePath);
    shaders[""] = shader;
    currentShader = shader;
}

UberShader::UberShader(Ref<Shader> shader){
    Assert(shader != nullptr);
    Assert(shader->Path() != "Memory");

    sourcePath = shader->Path();
    shaders[""] = shader;
    currentShader = shader;
}

void UberShader::DisableKeyword(std::string keyword){
    enabledKeywords.erase(keyword);
}

void UberShader::EnableKeyword(std::string keyword){
    enabledKeywords.insert(keyword);
}

void UberShader::SetCurrentShader(){
    UpdateCurrentShader();
}

Ref<Shader> UberShader::GetCurrentShader(){
    return currentShader;
}

void UberShader::UpdateCurrentShader(){

    std::string key = std::accumulate(enabledKeywords.begin(), enabledKeywords.end(), std::string(""));

    if(shaders.count(key)){
        currentShader = shaders[key];
    } else {
        std::vector<std::string> _enabledKeywords(enabledKeywords.begin(), enabledKeywords.end());
        Ref<Shader> shader = Shader::CreateFromFile(sourcePath, _enabledKeywords);
        shaders[key] = shader;
        currentShader = shader;
    }
}

}