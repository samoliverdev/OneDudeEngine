#include "GfxReflection.h"
#include "OD/Base.h"
#include "OD/Core/Log.h"
#include <spirv_reflect.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>

#include <spirv/unified1/spirv.hpp>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>

namespace OD{
namespace Gfx{   

static VertexFormat ReflectVertexFormat(const SpvReflectInterfaceVariable* var){
    if(!var) return VertexFormat::Float;

    const SpvReflectFormat format = var->format;

    switch(format){
        case SPV_REFLECT_FORMAT_R32_SFLOAT: return VertexFormat::Float;
        case SPV_REFLECT_FORMAT_R32G32_SFLOAT: return VertexFormat::Float2;
        case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT: return VertexFormat::Float3;
        case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return VertexFormat::Float4;
        case SPV_REFLECT_FORMAT_R32_SINT: return VertexFormat::Int;
        case SPV_REFLECT_FORMAT_R32G32_SINT: return VertexFormat::Int2;
        case SPV_REFLECT_FORMAT_R32G32B32_SINT: return VertexFormat::Int3;
        case SPV_REFLECT_FORMAT_R32G32B32A32_SINT: return VertexFormat::Int4;
        case SPV_REFLECT_FORMAT_R32_UINT: return VertexFormat::UInt;
        case SPV_REFLECT_FORMAT_R32G32_UINT: return VertexFormat::UInt2;
        case SPV_REFLECT_FORMAT_R32G32B32_UINT: return VertexFormat::UInt3;
        case SPV_REFLECT_FORMAT_R32G32B32A32_UINT: return VertexFormat::UInt4;
        default: return VertexFormat::Float;
    }

    return VertexFormat::Float;
}

static ShaderVariable ReflectVariable(const SpvReflectBlockVariable& variable){
    ShaderVariable result;

    if(variable.name)
        result.name = variable.name;

    result.offset = variable.offset;
    result.size = variable.size;

    result.arrayStride = variable.array.stride;
    result.matrixStride = 0; //variable.matrix.stride;

    result.arraySize = 1;

    if(variable.array.dims_count > 0){
        result.arraySize = 1;

        for(uint32_t i = 0; i < variable.array.dims_count; ++i){
            result.arraySize *= variable.array.dims[i];
        }
    }

    result.members.reserve(variable.member_count);

    for(uint32_t i = 0; i < variable.member_count; ++i){
        result.members.push_back(ReflectVariable(variable.members[i]));
    }

    return result;
}

static bool ReflectBindingType(const SpvReflectDescriptorBinding& binding, BindingType& result){
    switch(binding.descriptor_type){
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            result = BindingType::UniformBuffer;
            return true;

        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            result = BindingType::StorageBuffer;
            return true;

        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            result = BindingType::Texture2D;
            return true;

        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            result = BindingType::Texture2D;
            return true;

        default:
            return false;
    }

    return false;
}

static std::string GetSPIRVBlockName(const void* spirvData, size_t spirvSize, uint32_t variableId){
    const uint32_t* words = static_cast<const uint32_t*>(spirvData);
    const size_t wordCount = spirvSize / sizeof(uint32_t);

    if(!words || wordCount < 5) return {};

    // SPIR-V header is 5 words.
    std::unordered_map<uint32_t, std::string> names;
    std::unordered_map<uint32_t, uint32_t> pointerTypes;
    std::unordered_map<uint32_t, uint32_t> structTypes;

    for(size_t i = 5; i < wordCount;){
        const uint32_t firstWord = words[i];

        const uint16_t opcode =
            static_cast<uint16_t>(firstWord & 0xFFFF);

        const uint16_t wordLength =
            static_cast<uint16_t>(firstWord >> 16);

        if (wordLength == 0 || i + wordLength > wordCount)
            break;

        switch (opcode)
        {
        case spv::OpName:
        {
            // OpName <id> "name"
            if (wordLength >= 3)
            {
                const uint32_t id = words[i + 1];

                const char* name =
                    reinterpret_cast<const char*>(&words[i + 2]);

                names[id] = name;
            }
            break;
        }

        case spv::OpTypePointer:
        {
            // OpTypePointer <result-id> <storage-class> <type-id>
            if (wordLength >= 4)
            {
                const uint32_t resultId = words[i + 1];
                const uint32_t typeId   = words[i + 3];

                pointerTypes[resultId] = typeId;
            }
            break;
        }

        case spv::OpTypeStruct:
        {
            // OpTypeStruct <result-id> ...
            if (wordLength >= 2)
            {
                const uint32_t resultId = words[i + 1];

                structTypes[resultId] = resultId;
            }
            break;
        }

        default:
            break;
        }

        i += wordLength;
    }

    // Find:
    //
    // OpVariable %pointerType variableId
    //
    // We need to know the pointer type used by the variable.
    uint32_t pointerTypeId = 0;

    for (size_t i = 5; i < wordCount;)
    {
        const uint32_t firstWord = words[i];

        const uint16_t opcode =
            static_cast<uint16_t>(firstWord & 0xFFFF);

        const uint16_t wordLength =
            static_cast<uint16_t>(firstWord >> 16);

        if (wordLength == 0 || i + wordLength > wordCount)
            break;

        if (opcode == spv::OpVariable && wordLength >= 4)
        {
            const uint32_t typeId = words[i + 1];
            const uint32_t resultId = words[i + 2];

            if (resultId == variableId)
            {
                pointerTypeId = typeId;
                break;
            }
        }

        i += wordLength;
    }

    if (!pointerTypeId)
        return {};

    // Pointer -> struct
    auto pointerIt = pointerTypes.find(pointerTypeId);

    if (pointerIt == pointerTypes.end())
        return {};

    const uint32_t structId = pointerIt->second;

    // Struct -> OpName
    auto nameIt = names.find(structId);

    if (nameIt == names.end())
        return {};

    return nameIt->second;
}

bool ReflectSPIRV(const void* spirvData, size_t spirvSize, ShaderReflection& reflection, bool skipVertexAttributes){
    //reflection.vertexAttributes.clear();
    //reflection.bindings.clear();

    SpvReflectShaderModule module{};

    SpvReflectResult result = spvReflectCreateShaderModule(spirvSize, spirvData, &module);

    if(result != SPV_REFLECT_RESULT_SUCCESS)
        return false;


    //////////////////////////////////////////////////
    // Vertex attributes
    //////////////////////////////////////////////////
    uint32_t inputCount = 0;

    result = spvReflectEnumerateInputVariables(&module, &inputCount, nullptr);

    if(result != SPV_REFLECT_RESULT_SUCCESS){
        spvReflectDestroyShaderModule(&module);
        return false;
    }

    std::vector<SpvReflectInterfaceVariable*> inputs(
        inputCount
    );

    result = spvReflectEnumerateInputVariables(&module, &inputCount, inputs.data());

    if(result != SPV_REFLECT_RESULT_SUCCESS){
        spvReflectDestroyShaderModule(&module);
        return false;
    }

    for(auto* input : inputs){
        if(skipVertexAttributes) continue;
        // Built-in variables such as gl_Position
        // aren't vertex attributes.
        if(input->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN){
            continue;
        }

        ShaderVertexAttribute attribute;
        attribute.name = input->name;// ? input->name : "";
        attribute.location = input->location;
        attribute.format = ReflectVertexFormat(input);
        reflection.vertexAttributes.push_back(std::move(attribute));
    }


    //////////////////////////////////////////////////
    // Descriptor bindings
    //////////////////////////////////////////////////

    uint32_t bindingCount = 0;

    result = spvReflectEnumerateDescriptorBindings(&module, &bindingCount, nullptr);

    if(result != SPV_REFLECT_RESULT_SUCCESS){
        spvReflectDestroyShaderModule(&module);
        return false;
    }

    std::vector<SpvReflectDescriptorBinding*> bindings(bindingCount);

    result = spvReflectEnumerateDescriptorBindings(&module, &bindingCount, bindings.data());

    if(result != SPV_REFLECT_RESULT_SUCCESS){
        spvReflectDestroyShaderModule(&module);
        return false;
    }

    auto ContainsBlockName = [&](const std::string& name){
        for(const auto& binding : reflection.bindings){
            if(name.empty()) continue;
            if(name == binding.blockName) return true;
        }
        return false;
    };

    auto ContainsName = [&](const std::string& name){
        for(const auto& binding : reflection.bindings){
            if(name.empty()) continue;
            if(name == binding.name) return true;
        }
        return false;
    };

    for(auto* binding : bindings){
        ShaderBindingInfo info;
        info.name = binding->name;// ? binding->name : "";
        info.set = binding->set;
        info.binding = binding->binding;

        if(!ReflectBindingType(*binding, info.type))
            continue;


        //////////////////////////////////////////////////
        // Uniform buffer
        //////////////////////////////////////////////////

        if(info.type == BindingType::UniformBuffer){
            //if(binding->block){
                info.size = binding->block.size;
                info.blockName = binding->block.name;// ? binding->block.name : "";

                info.blockName = GetSPIRVBlockName(spirvData, spirvSize, binding->spirv_id);

                for(uint32_t i = 0; i < binding->block.member_count; ++i){
                    info.variables.push_back(ReflectVariable(binding->block.members[i]));
                }
            //}
        }

        if(ContainsName(info.name)) continue;
        if(ContainsBlockName(info.blockName)) continue;
        reflection.bindings.push_back(std::move(info));
    }


    spvReflectDestroyShaderModule(&module);

    return true;
}

std::vector<uint32_t> CompileGLSL(const std::string& source, EShLanguage stage){
    static bool initialized = false;
    if(!initialized){
        glslang::InitializeProcess();
        initialized = true;
    }

    const EShLanguage shaderStage = stage; //ToGlslangStage(stage);
    const char* sourceString = source.c_str();

    glslang::TShader shader(shaderStage);
    shader.setStrings(&sourceString, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, shaderStage, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);

    const TBuiltInResource* resources = GetDefaultResources();
    EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    if(!shader.parse(resources, 450, false, messages)){
        std::string error = "GLSL compilation failed:\n" + std::string(shader.getInfoLog()) + "\n" + shader.getInfoDebugLog();
        LogInfo("Error: {}", error);
        throw std::runtime_error(error);
    }

    glslang::TProgram program;
    program.addShader(&shader);

    if(!program.link(messages)){
        std::string error = "GLSL linking failed:\n" + std::string(program.getInfoLog()) + "\n" + program.getInfoDebugLog();
        LogInfo("Error: {}", error);
        throw std::runtime_error(error);
    }

    std::vector<uint32_t> spirv;
    spv::SpvBuildLogger logger;
    glslang::SpvOptions options;
    /*options.stripDebugInfo = true;
    options.emitNonSemanticShaderDebugSource = true;
    options.emitNonSemanticShaderDebugInfo = true;*/
    options.generateDebugInfo = true; //false;
    options.disableOptimizer = true; //false;
    options.optimizeSize = false;

    glslang::GlslangToSpv(*program.getIntermediate(shaderStage), spirv, &logger, &options);
    return spirv;
}

bool Reflect(const char* shaderSource, ShaderReflection& reflection){
    std::string srcStr = shaderSource;
    std::string vertexSource = "#version 450\n#define Vulkan_API\n#define VERTEX\n" + srcStr;
    std::string fragmentSource = "#version 450\n#define Vulkan_API\n#define FRAGMENT\n" + srcStr;

    std::vector<uint32_t> spirvV = CompileGLSL(vertexSource, EShLangVertex);
    std::vector<uint32_t> spirvF = CompileGLSL(fragmentSource, EShLangFragment);

    reflection.vertexAttributes.clear();
    reflection.bindings.clear();

    ReflectSPIRV(spirvV.data(), spirvV.size() * sizeof(uint32_t), reflection, false);
    ReflectSPIRV(spirvF.data(), spirvF.size() * sizeof(uint32_t), reflection, true);

    return false;
}

void ShaderReflectionToPipelineInfo(const ShaderReflection& reflection, PipelineInfo& pipelineOut, std::vector<BindGroupLayoutInfo>& layoutsOut){
    // ------------------------------------------------------------
    // Vertex attributes
    // ------------------------------------------------------------
    for(const ShaderVertexAttribute& attribute : reflection.vertexAttributes){
        VertexAttribute& dst = pipelineOut.vertexLayout.attributes[pipelineOut.vertexLayout.attributeCount++];

        dst.semantic = SlotToVertexSemanticTo(attribute.location); VertexSemantic::Custom0; // map below
        dst.format = attribute.format;
        dst.bufferSlot = 0;
        dst.offset = 0;
    }

    // ------------------------------------------------------------
    // Descriptor bindings
    // ------------------------------------------------------------
    layoutsOut.resize(4); //Max possible Bindgroups/Sets
    for (const ShaderBindingInfo& binding : reflection.bindings){
        // Currently assuming set == bind group index.
        // Make sure your PipelineInfo supports enough groups.
        //if(binding.set >= pipelineOut.bindGroupLayoutCount) pipelineOut.bindGroupLayoutCount = binding.set + 1;

        BindGroupLayoutInfo& layout = layoutsOut[binding.set]; //info.bindGroupLayouts[binding.set];

        Assert(layout.entriesCount < 4);
        if(layout.entriesCount >= 4) continue;

        BindLayoutEntry& entry = layout.entries[layout.entriesCount++];

        entry.binding = binding.binding;
        entry.type = binding.type;

        if(binding.type == BindingType::UniformBuffer){
            entry.minUniformBufferSize = binding.size;
            entry.dynamicOffset = false;
        } else {
            entry.minUniformBufferSize = 0;
            entry.dynamicOffset = false;
        }
    }
}

}}