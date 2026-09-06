#include "GfxReflection.h"
#include "OD/Core/Log.h"
#include <spirv_reflect.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>

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
        attribute.name = input->name ? input->name : "";
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

    for(auto* binding : bindings){
        ShaderBindingInfo info;
        info.name = binding->name ? binding->name : "";
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
                info.blockName = binding->block.name ? binding->block.name : "";

                for(uint32_t i = 0; i < binding->block.member_count; ++i){
                    info.variables.push_back(ReflectVariable(binding->block.members[i]));
                }
            //}
        }

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
    options.generateDebugInfo = true; //false;
    options.disableOptimizer = true; //false;
    options.optimizeSize = false;

    glslang::GlslangToSpv(*program.getIntermediate(shaderStage), spirv, &logger, &options);
    return spirv;
}

bool Reflect(const char* shaderSource, ShaderReflection& reflection){
    std::string srcStr = shaderSource;
    std::string vertexSource = "#version 450\n#define Vulkan\n#define Vertex\n" + srcStr;
    std::string fragmentSource = "#version 450\n#define Vulkan\n#define Fragment\n" + srcStr;

    std::vector<uint32_t> spirvV = CompileGLSL(vertexSource, EShLangVertex);
    std::vector<uint32_t> spirvF = CompileGLSL(fragmentSource, EShLangFragment);

    reflection.vertexAttributes.clear();
    reflection.bindings.clear();

    ReflectSPIRV(spirvV.data(), spirvV.size() * sizeof(uint32_t), reflection, false);
    ReflectSPIRV(spirvF.data(), spirvF.size() * sizeof(uint32_t), reflection, true);

    return false;
}

}}