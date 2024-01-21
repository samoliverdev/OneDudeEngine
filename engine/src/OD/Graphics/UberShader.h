#pragma once

#include "OD/Defines.h"
#include "Shader.h"
#include <unordered_map>
#include <set>

namespace OD{

class UberShader{
public:
    UberShader(std::string sourcePath);
    UberShader(Ref<Shader> shader);

    void DisableKeyword(std::string keyword);
    void EnableKeyword(std::string keyword);

    void SetCurrentShader();
    
    Ref<Shader> GetCurrentShader();

private:
    std::set<std::string> enabledKeywords;

    std::string sourcePath;
    std::unordered_map<std::string, Ref<Shader>> shaders;
    Ref<Shader> currentShader;

    void UpdateCurrentShader();
};

}