#pragma once
#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"
#include "Texture.h"
#include "Cubemap.h"
#include "Framebuffer.h"
#include "UniformBuffer.h"
#include "RendererTypes.h"

namespace OD {

struct OD_API ShaderPipeline{
    CullFace cullFace = CullFace::BACK;
    DepthTest depthTest = DepthTest::LESS;
    bool depthMask = true;
    Vector4 colorMask = {1, 1, 1, 1};
    bool blend = false;
    BlendMode srcBlend;
    BlendMode dstBlend;
    bool supportInstancing;
};

struct OD_API ShaderPassData{
    std::string name;
    ShaderPipeline pipeline;
    std::vector<std::vector<std::string>> properties;
    void UpdateProperties();
};

struct OD_API ShaderSourceData{
    std::vector<std::vector<std::string>> properties;
    std::vector<std::vector<std::string>> pragmas;
    std::vector<ShaderPassData> passes;
    std::string baseSource;
};

bool OD_API ShaderLoadFile(const std::string& path, ShaderSourceData& out);

struct OD_API SubShader{
    ShaderPipeline pipeline;
    std::vector<std::string> enabledKeyworlds;
    std::vector<std::vector<std::string>> pragmas;
    //std::unordered_map<std::string, int> uniforms;
    //std::vector<std::string> _uniforms;
    SubShaderDataGL;

    inline bool SupportInstancing(){ return pipeline.supportInstancing; }
    inline CullFace GetCullFace(){ return pipeline.cullFace; }
    inline DepthTest GetDepthTest(){ return pipeline.depthTest; }
    inline bool IsDepthMask(){ return pipeline.depthMask; }
    inline bool IsBlend(){ return pipeline.blend; }
    inline BlendMode GetSrcBlend(){ return pipeline.srcBlend; }
    inline BlendMode GetDstBlend(){ return pipeline.dstBlend; }

    //inline bool ContainUniformName(const std::string& name){ return std::find(_uniforms.begin(), _uniforms.end(), name) != _uniforms.end(); }
    //inline std::vector<std::vector<std::string>>& Pragmas(){ return pragmas; }
};

}