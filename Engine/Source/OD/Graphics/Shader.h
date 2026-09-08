#pragma once
#include "OD/Core/Resource.h"
#include "OD/Platform/OpenGL/GL.h"
#include "OD/Gfx/Gfx.h"
#include "OD/Gfx/GfxReflection.h"
#include "SubShader.h"
#include <set>
#include <vector>
#include <unordered_map>

namespace OD{

// Handler Mult Shader Variants
class OD_API Shader: public Resource{
    friend class Material;
    friend class OpenGLGraphicsDevice;
    friend class WebGPUGraphicsDevice;
    friend class Graphics;
public:
    Shader(std::string sourcePath);
    Shader(){}

    //TODO: Outdata this later
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
        InstancingDraw43 = 3,
        SkinnedDraw2 = 4,
        Count = 5
    };

    struct SubShaderTarget{
        Ref<SubShader> drawTypes[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    };

    inline int PassesCount(){return passes.size(); }
    inline const std::vector<std::string>& TagsString(int pass){ return passes[pass].tagsString; }
    inline const std::vector<uint32_t>& TagsHash(int pass){ return passes[pass].tagsHash; }

private:
    struct KeyworldSpace{
        std::vector<std::string> keyworlds;
        int enabledKey = -1;
    };
    
    struct Pass{
        std::string name;
        std::unordered_map<std::string, SubShaderTarget> shaders;
        std::vector<std::string> tagsString;
        std::vector<uint32_t> tagsHash;
    };

    ShaderSourceData shaderSourceData;
    std::vector<Pass> passes;
    std::vector<std::string> errors;
    std::string sourcePath;
    std::vector<KeyworldSpace> keyworldSpaces;
    int curPass = 0;
    //Ref<SubShader> currentShader;
    SubShaderTarget currentShader;

    Gfx::ShaderReflection reflection;
    Gfx::BindGroupLayout materialBindGroupLayout;
    Gfx::PipelineInfo pipelineInfo;
    std::vector<Gfx::BindGroupLayoutInfo> layoutsOut;

    bool isComplete = false;
    ShaderDataGL;

    bool Create(std::string path);
    void Destroy();
    bool InitPass(int pass);
    void AddShaderVaring(std::string key, const std::set<std::string>& keywords, int pass, const std::set<DrawType>& drawTypes);
};

}