#pragma once
#include "OD/Core/Asset.h"
#include "OD/Platform/OpenGL/GL.h"
#include "SubShader.h"
#include <set>
#include <vector>
#include <unordered_map>

namespace OD{

// Handler Mult Shader Variants
class OD_API Shader: public Asset{
    friend class Material;
    friend class OpenGLGraphicsDevice;
    friend class WebGPUGraphicsDevice;
public:
    Shader(std::string sourcePath);
    Shader(){}

    static Ref<Shader> CreateFromFile(const std::string& filepath);

    bool LoadFromFile(const std::string& path) override;
    bool LoadFromPackage(const std::string& path, Package& package) override;
    std::vector<std::string> GetFileAssociations() override;

    bool Save(const std::string& outPath, SaveType type) override;

    inline bool IsComplete(){ return isComplete; }
    inline std::vector<std::vector<std::string>>& Properties(){ return shaderSourceData.properties; }

    enum class DrawType{
        DefaultDraw = 0,
        SkinnedDraw = 1,
        InstancingDraw = 2,
        InstancingDraw43 = 3
    };

    struct SubShaderTarget{
        Ref<SubShader> drawTypes[4] = {nullptr, nullptr, nullptr, nullptr};
    };

private:
    struct KeyworldSpace{
        std::vector<std::string> keyworlds;
        int enabledKey = -1;
    };
    
    struct Pass{
        std::string name;
        std::unordered_map<std::string, SubShaderTarget> shaders;
    };

    ShaderSourceData shaderSourceData;
    std::vector<Pass> passes;
    std::vector<std::string> errors;
    std::string sourcePath;
    std::vector<KeyworldSpace> keyworldSpaces;
    int curPass = 0;
    //Ref<SubShader> currentShader;
    SubShaderTarget currentShader;

    bool isComplete = false;
    ShaderDataGL;

    bool Create(std::string path);
    void Destroy();
    bool InitPass(int pass);
    void AddShaderVaring(std::string key, const std::set<std::string>& keywords, int pass, const std::set<DrawType>& drawTypes);
};

}