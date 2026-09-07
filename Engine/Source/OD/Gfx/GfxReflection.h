#pragma once
#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include "Gfx.h"

namespace OD{
namespace Gfx{   

struct OD_API ShaderVariable {
    std::string name;

    uint32_t offset = 0;
    uint32_t size = 0;

    uint32_t arrayStride = 0;
    uint32_t matrixStride = 0;
    uint32_t arraySize = 1;

    std::vector<ShaderVariable> members;
};

struct OD_API ShaderVertexAttribute {
    std::string name;
    uint32_t location = 0;
    VertexFormat format = VertexFormat::Float;
};

struct OD_API ShaderBindingInfo {
    std::string name;
    std::string blockName;

    uint32_t set = 0;
    uint32_t binding = 0;

    BindingType type = BindingType::UniformBuffer;

    uint32_t size = 0;

    std::vector<ShaderVariable> variables;
};

struct OD_API ShaderReflection {
    std::vector<ShaderVertexAttribute> vertexAttributes;
    std::vector<ShaderBindingInfo> bindings;
};

bool OD_API Reflect(const char* shaderSource, ShaderReflection& reflection);
void OD_API ShaderReflectionToPipelineInfo(const ShaderReflection& reflection, PipelineInfo& pipelineOut, std::vector<BindGroupLayoutInfo>& layoutsOut);

}}