#pragma once
#include "OD/Defines.h"
#include "SubShader.h"
#include <set>
#include <vector>
#include <unordered_map>
#include "OD/Platform/OpenGL/GL.h"

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
    std::vector<std::string> GetFileAssociations() override;

    inline bool IsComplete(){ return isComplete; }
    inline std::vector<std::vector<std::string>>& Properties(){ return shaderSourceData.properties; }

private:
    struct KeyworldSpace{
        std::vector<std::string> keyworlds;
        int enabledKey = -1;
    };
    
    struct Pass{
        std::string name;
        std::unordered_map<std::string, Ref<SubShader>> shaders;
    };

    ShaderSourceData shaderSourceData;
    std::vector<Pass> passes;
    std::vector<std::string> errors;
    std::string sourcePath;
    std::vector<KeyworldSpace> keyworldSpaces;
    int curPass = 0;
    Ref<SubShader> currentShader;

    bool isComplete = false;
    ShaderDataGL;

    bool Create(std::string path);
    void Destroy();
    bool InitPass(int pass);
    void AddShaderVaring(std::string key, const std::set<std::string>& keywords, int pass);
};

}