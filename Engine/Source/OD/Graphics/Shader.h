#pragma once
#include "OD/Defines.h"
#include "SubShader.h"
#include <set>
#include <vector>
#include <unordered_map>

namespace OD{

// Handler Mult Shader Variants
class OD_API Shader{
public:
    Shader(std::string sourcePath);

    //Need Call Disable First
    void DisableKeyword(std::string keyword);
    void EnableKeyword(std::string keyword);
    void SetPass(int pass);

    void SetCurrentShader();
    Ref<SubShader> GetCurrentShader();

private:
    struct KeyworldSpace{
        std::vector<std::string> keyworlds;
        int enabledKey = -1;
    };

    struct Pass{
        std::string name;
        std::unordered_map<std::string, Ref<SubShader>> shaders;
    };

    std::vector<KeyworldSpace> keyworldSpaces;
    std::set<std::string> enabledKeywords;

    ShaderSourceData shaderSourceData;
    std::vector<Pass> passes;
    std::string sourcePath;
    int curPass = 0;
    Ref<SubShader> currentShader;

    void InitPass(std::string path, int pass);
    void UpdateCurrentShader();
    void AddShaderVaring(std::string key, const std::set<std::string>& keywords, int pass);
    std::set<std::string> GetEnabledKeywords();
};

}