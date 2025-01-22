#pragma once
#include "OD/Defines.h"
#include "SubShader.h"
#include <set>
#include <vector>
#include <unordered_map>

namespace OD{

// Handler Mult Shader Variants
class OD_API MultiCompileShader{
public:
    MultiCompileShader(std::string sourcePath);
    MultiCompileShader(Ref<SubShader> shader);

    //Need Call Disable First
    void DisableKeyword(std::string keyword);
    void EnableKeyword(std::string keyword);

    void SetCurrentShader();
    Ref<SubShader> GetCurrentShader();

private:
    struct KeyworldSpace{
        std::vector<std::string> keyworlds;
        int enabledKey = -1;
    };

    std::vector<KeyworldSpace> keyworldSpaces;
    std::set<std::string> enabledKeywords;

    std::string sourcePath;
    std::unordered_map<std::string, Ref<SubShader>> shaders;
    Ref<SubShader> currentShader;

    void Init(Ref<SubShader> baseShader, std::string path);
    void UpdateCurrentShader();
    void AddShaderVaring(std::string key, const std::set<std::string>& keywords);

    std::set<std::string> GetEnabledKeywords();
};

}