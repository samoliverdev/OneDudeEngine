#pragma once
#include "OD/Defines.h"
#include "Shader.h"

namespace OD{

// Handler Mult Shader Variants
class OD_API MultiCompileShader{
public:
    MultiCompileShader(std::string sourcePath);
    MultiCompileShader(Ref<Shader> shader);

    //Need Call Disable First
    void DisableKeyword(std::string keyword);
    void EnableKeyword(std::string keyword);

    void SetCurrentShader();
    Ref<Shader> GetCurrentShader();

private:
    struct KeyworldSpace{
        std::vector<std::string> keyworlds;
        int enabledKey = -1;
    };

    std::vector<KeyworldSpace> keyworldSpaces;
    std::set<std::string> enabledKeywords;

    std::string sourcePath;
    std::unordered_map<std::string, Ref<Shader>> shaders;
    Ref<Shader> currentShader;

    void Init(Ref<Shader> baseShader, std::string path);
    void UpdateCurrentShader();
    void AddShaderVaring(std::string key, const std::set<std::string>& keywords);

    std::set<std::string> GetEnabledKeywords();
};

}