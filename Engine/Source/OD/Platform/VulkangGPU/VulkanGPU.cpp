#include "VulkanGPU.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Core/Application.h"
#include "OD/Platform/Platform.h"
#include "OD/Platform/BaseGpu/ResourcePool.h"

#include <vulkan/vulkan.h>
#include <vk-bootstrap/VkBootstrap.h>
#include "vk_initializers.h"

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>

#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
#include <unordered_map>

//#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

namespace OD{
namespace Gfx{   

//#define TestDrawInverted 
#define DONT_DEFERRED_RESOURCE_CREATION

GraphicsDeviceInfo vkInfo;
GraphicsStats vkGraphicsStats;
GPUMemoryStats vkGPUMemoryStats;
GraphicsDebug vkGraphicsDebug;

GraphicsStats& VulkanGPUDevice::GetStats() { return vkGraphicsStats; }
GPUMemoryStats& VulkanGPUDevice::GetMemoryStats() { return vkGPUMemoryStats; }
GraphicsDebug& VulkanGPUDevice::GetGraphicsDebug() { return vkGraphicsDebug; }
GraphicsDeviceInfo VulkanGPUDevice::GetInfo() { return vkInfo; }

/*struct BufferData {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    GPUBufferUsage usage;
    GPUBufferMemory memory;
    VkDeviceSize size = 0;
};
ResourcePool<BufferData> bufferPool;

struct PipelineData {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    GPUPipelineInfo info;
};
ResourcePool<PipelineData> pipelinePool;*/

bool _isInitialized{ false };
int _frameNumber {0};

VkExtent2D _windowExtent{ 800, 600 };
VkInstance _instance;
VkDebugUtilsMessengerEXT _debug_messenger;
VkPhysicalDevice _chosenGPU;
VkDevice _device;
VkSurfaceKHR _surface;

VmaAllocator _allocator;

VkSwapchainKHR _swapchain;
VkFormat _swapchainImageFormat;
std::vector<VkImage> _swapchainImages;
std::vector<VkImageView> _swapchainImageViews;

VkQueue _graphicsQueue;
uint32_t _graphicsQueueFamily;

VkCommandPool _commandPool;
VkCommandBuffer _mainCommandBuffer;

FrameBufferLayout _windowFrameBufferLayout;
VkRenderPass _renderPass;
std::vector<VkFramebuffer> _framebuffers;

std::vector<VkSemaphore> _renderSemaphores;

VkDescriptorPool _descriptorPool;
VkDescriptorPool frameDescriptorPools;

struct FrameData {
	VkSemaphore _presentSemaphore;
	VkFence _renderFence;	
	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;
};
constexpr unsigned int FRAME_OVERLAP = 1;
FrameData _frames[FRAME_OVERLAP];

struct UploadContext {
	VkFence _uploadFence;
	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
};
UploadContext _uploadContext;

struct AllocatedBuffer {
	VkBuffer _buffer;
	VmaAllocation _allocation;
};

AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage){
	//allocate vertex buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;
	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;

	//let the VMA library know that this data should be writeable by CPU, but also readable by GPU
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage;

	AllocatedBuffer newBuffer;
	VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &newBuffer._buffer, &newBuffer._allocation, nullptr));
	return newBuffer;
}

void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function){
    VkCommandBuffer cmd = _uploadContext._commandBuffer;

	//begin the command buffer recording. We will use this command buffer exactly once before resetting, so we tell vulkan that
	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	//execute the function
	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = vkinit::submit_info(&cmd);

	//submit command buffer to the queue and execute it.
	// _uploadFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, _uploadContext._uploadFence));

	vkWaitForFences(_device, 1, &_uploadContext._uploadFence, true, 9999999999);
	vkResetFences(_device, 1, &_uploadContext._uploadFence);

	// reset the command buffers inside the command pool
	vkResetCommandPool(_device, _uploadContext._commandPool, 0);
}

struct DeletionQueue{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function){
		deletors.push_back(function);
	}

	void flush(){
		// reverse iterate the deletion queue to execute all the functions
		for(auto it = deletors.rbegin(); it != deletors.rend(); it++){
			(*it)(); //call the function
		}
		deletors.clear();
	}
};

DeletionQueue mainDeletionQueue;

//getter for the frame we are rendering to right now.
FrameData& get_current_frame(){
    return _frames[_frameNumber % FRAME_OVERLAP];
}

#pragma region Pipeline

EShLanguage ToGlslangStage(VkShaderStageFlagBits stage){
    switch(stage){
        case VK_SHADER_STAGE_VERTEX_BIT: return EShLangVertex;
        case VK_SHADER_STAGE_FRAGMENT_BIT: return EShLangFragment;
        case VK_SHADER_STAGE_COMPUTE_BIT: return EShLangCompute;
        case VK_SHADER_STAGE_GEOMETRY_BIT: return EShLangGeometry;
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return EShLangTessControl;
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return EShLangTessEvaluation;
        default: throw std::runtime_error("Unsupported Vulkan shader stage");
    }
}

std::vector<uint32_t> CompileGLSL(const std::string& source, VkShaderStageFlagBits stage){
    static bool initialized = false;
    if(!initialized){
        glslang::InitializeProcess();
        initialized = true;
    }

    const EShLanguage shaderStage = ToGlslangStage(stage);
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
        LogError("Error: {}", error);
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
    options.generateDebugInfo = false;
    options.disableOptimizer = false;
    options.optimizeSize = false;

    glslang::GlslangToSpv(*program.getIntermediate(shaderStage), spirv, &logger, &options);
    return spirv;
}

bool load_shader_module(const char* source, VkShaderStageFlagBits stage, VkShaderModule* outShaderModule){
    std::vector<uint32_t> spirv = CompileGLSL(source, stage);

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirv.size() * sizeof(uint32_t);
    createInfo.pCode = spirv.data();

    return vkCreateShaderModule(_device, &createInfo, nullptr, outShaderModule) == VK_SUCCESS;
}

/*static uint32_t GetVertexLocation(VertexSemantic semantic){
    switch(semantic){
        case VertexSemantic::Position:   return 0;
        case VertexSemantic::UV0:        return 1;
        case VertexSemantic::Normal:     return 2;
        case VertexSemantic::Tangent:    return 4;
        case VertexSemantic::UV1:        return 3;
        case VertexSemantic::UV2:        return 5;
        case VertexSemantic::UV3:        return 6;
        case VertexSemantic::Color0:     return 7;
        case VertexSemantic::Color1:     return 8;
        case VertexSemantic::Weights:    return 9;
        case VertexSemantic::Influences: return 10;
        case VertexSemantic::Custom0:    return 11;
        case VertexSemantic::Custom1:    return 12;
        case VertexSemantic::Custom2:    return 13;
        case VertexSemantic::Custom3:    return 14;
    }
    return 0;
}*/

VkFormat GetVulkanFormat(VertexFormat format){
    switch(format){
        case VertexFormat::Float:   return VK_FORMAT_R32_SFLOAT;
        case VertexFormat::Float2:  return VK_FORMAT_R32G32_SFLOAT;
        case VertexFormat::Float3:  return VK_FORMAT_R32G32B32_SFLOAT;
        case VertexFormat::Float4:  return VK_FORMAT_R32G32B32A32_SFLOAT;
        case VertexFormat::Int:     return VK_FORMAT_R32_SINT;
        case VertexFormat::Int2:    return VK_FORMAT_R32G32_SINT;
        case VertexFormat::Int3:    return VK_FORMAT_R32G32B32_SINT;
        case VertexFormat::Int4:    return VK_FORMAT_R32G32B32A32_SINT;
        case VertexFormat::UInt:    return VK_FORMAT_R32_UINT;
        case VertexFormat::UInt2:   return VK_FORMAT_R32G32_UINT;
        case VertexFormat::UInt3:   return VK_FORMAT_R32G32B32_UINT;
        case VertexFormat::UInt4:   return VK_FORMAT_R32G32B32A32_UINT;
        default: return VK_FORMAT_R32G32B32A32_SFLOAT;
    }
}

VkBufferUsageFlags GetVulkanBufferUsage(BufferUsage usage){
    switch(usage){
        case BufferUsage::Vertex:  return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        case BufferUsage::Index:   return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        case BufferUsage::Uniform: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        case BufferUsage::Storage: return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
}

VmaMemoryUsage GetVulkanMemoryUsage(BufferMemory memory){
    switch(memory){
        case BufferMemory::GPUOnly:  return VMA_MEMORY_USAGE_GPU_ONLY;
        case BufferMemory::CPUToGPU: return VMA_MEMORY_USAGE_CPU_TO_GPU;
        case BufferMemory::GPUToCPU: return VMA_MEMORY_USAGE_GPU_TO_CPU;
        case BufferMemory::CPUOnly:  return VMA_MEMORY_USAGE_CPU_ONLY;
    }
    return VMA_MEMORY_USAGE_AUTO;
}

VkCullModeFlags GetVulkanCullMode(CullFace cull){
    return VK_CULL_MODE_NONE;
    
    switch(cull){
        case CullFace::NONE:           return VK_CULL_MODE_NONE;
        #ifdef TestDrawInverted
        case CullFace::BACK:           return VK_CULL_MODE_FRONT_BIT; //return VK_CULL_MODE_BACK_BIT; //Inverted, becose projection[1][1] *= -1.0f;
        case CullFace::FRONT:          return VK_CULL_MODE_BACK_BIT; //return VK_CULL_MODE_FRONT_BIT; //Inverted, becose projection[1][1] *= -1.0f;
        #else
        case CullFace::BACK:           return VK_CULL_MODE_BACK_BIT; 
        case CullFace::FRONT:          return VK_CULL_MODE_FRONT_BIT; 
        #endif
    
        case CullFace::FRONT_AND_BACK: return VK_CULL_MODE_FRONT_AND_BACK;
    }
    return VK_CULL_MODE_BACK_BIT;
}

VkCompareOp GetVulkanCompareOp(DepthTest depthTest){
    switch(depthTest){
        case DepthTest::DISABLE:       return VK_COMPARE_OP_ALWAYS;
        case DepthTest::LESS:          return VK_COMPARE_OP_LESS;
        case DepthTest::LESS_EQUAL:    return VK_COMPARE_OP_LESS_OR_EQUAL;
        case DepthTest::EQUAL:         return VK_COMPARE_OP_EQUAL;
        case DepthTest::GREATER:       return VK_COMPARE_OP_GREATER;
        case DepthTest::GREATER_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case DepthTest::DIFFERENT:     return VK_COMPARE_OP_NOT_EQUAL;
        case DepthTest::NEVER:         return VK_COMPARE_OP_NEVER;
        case DepthTest::ALWAYS:        return VK_COMPARE_OP_ALWAYS;
    }
    return VK_COMPARE_OP_LESS;
}

VkBlendFactor GetVulkanBlendFactor(BlendMode mode){
    switch(mode){
        case BlendMode::ZERO:                     return VK_BLEND_FACTOR_ZERO;
        case BlendMode::ONE:                      return VK_BLEND_FACTOR_ONE;
        case BlendMode::SRC_COLOR:                return VK_BLEND_FACTOR_SRC_COLOR;
        case BlendMode::ONE_MINUS_SRC_COLOR:      return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case BlendMode::DST_COLOR:                return VK_BLEND_FACTOR_DST_COLOR;
        case BlendMode::ONE_MINUS_DST_COLOR:      return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case BlendMode::SRC_ALPHA:                return VK_BLEND_FACTOR_SRC_ALPHA;
        case BlendMode::ONE_MINUS_SRC_ALPHA:      return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case BlendMode::DST_ALPHA:                return VK_BLEND_FACTOR_DST_ALPHA;
        case BlendMode::ONE_MINUS_DST_ALPHA:      return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case BlendMode::CONSTANT_COLOR:           return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case BlendMode::ONE_MINUS_CONSTANT_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case BlendMode::CONSTANT_ALPHA:           return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case BlendMode::ONE_MINUS_CONSTANT_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
    }
    return VK_BLEND_FACTOR_ONE;
}

VkBlendOp GetVulkanBlendOp(BlendOp op){
    switch(op){
        case BlendOp::FUNC_ADD:              return VK_BLEND_OP_ADD;
        case BlendOp::FUNC_SUBTRACT:         return VK_BLEND_OP_SUBTRACT;
        case BlendOp::FUNC_REVERSE_SUBTRACT: return VK_BLEND_OP_REVERSE_SUBTRACT;
        case BlendOp::MIN:                   return VK_BLEND_OP_MIN;
        case BlendOp::MAX:                   return VK_BLEND_OP_MAX;
    }
    return VK_BLEND_OP_ADD;
}

VkDescriptorType GetVulkanDescriptorType(BindingType type){
    switch(type){
        case BindingType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case BindingType::Texture2D: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    }
    Assert(false);
    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
}

bool VulkanGPUDevice::_CreatePipeline(PipelineData& data, const char* source, const PipelineInfo& info){
    std::string srcStr = source;
    std::string vertexSource = "#version 450\n#define Vulkan_API\n#define VERTEX\n" + srcStr;
    std::string fragmentSource = "#version 450\n#define Vulkan_API\n#define FRAGMENT\n" + srcStr;

    VkShaderModule vertModule = VK_NULL_HANDLE;
    VkShaderModule fragModule = VK_NULL_HANDLE;
    if(!load_shader_module(vertexSource.c_str(), VK_SHADER_STAGE_VERTEX_BIT, &vertModule) || !load_shader_module(fragmentSource.c_str(), VK_SHADER_STAGE_FRAGMENT_BIT, &fragModule)) {
        LogError("Failed to build pipeline shaders");
        return false;
    }

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertModule),
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragModule)
    };

    std::vector<VkVertexInputBindingDescription> bindings;
    for(uint32_t i = 0; i < info.vertexLayout.bufferCount; ++i){
        VkVertexInputBindingDescription binding{};
        binding.binding = i;
        binding.stride = static_cast<uint32_t>(info.vertexLayout.buffers[i].stride);
        binding.inputRate = (info.vertexLayout.buffers[i].inputRate == VertexInputRate::Instance) ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
        bindings.push_back(binding);
    }

    std::vector<VkVertexInputAttributeDescription> attributes;
    for(uint32_t i = 0; i < info.vertexLayout.attributeCount; ++i){
        const auto& attr = info.vertexLayout.attributes[i];
        VkVertexInputAttributeDescription attrib{};
        attrib.location = VertexSemanticToSlot(attr.semantic);// GetVertexLocation(attr.semantic);
        attrib.binding = attr.bufferSlot;
        attrib.format = GetVulkanFormat(attr.format);
        attrib.offset = static_cast<uint32_t>(attr.offset);
        attributes.push_back(attrib);
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    vertexInputInfo.pVertexBindingDescriptions = bindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    VkPipelineRasterizationStateCreateInfo rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    rasterizer.cullMode = GetVulkanCullMode(info.cullFace);
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; //VK_FRONT_FACE_COUNTER_CLOCKWISE; //VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling = vkinit::multisampling_state_create_info();

    VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::color_blend_attachment_state();
    colorBlendAttachment.blendEnable = info.blend ? VK_TRUE : VK_FALSE;
    if(info.blend){
        colorBlendAttachment.srcColorBlendFactor = GetVulkanBlendFactor(info.srcBlend);
        colorBlendAttachment.dstColorBlendFactor = GetVulkanBlendFactor(info.dstBlend);
        colorBlendAttachment.colorBlendOp = GetVulkanBlendOp(info.opBlend);
        colorBlendAttachment.srcAlphaBlendFactor = GetVulkanBlendFactor(info.srcAlphaBlend);
        colorBlendAttachment.dstAlphaBlendFactor = GetVulkanBlendFactor(info.dstAlphaBlend);
        colorBlendAttachment.alphaBlendOp = GetVulkanBlendOp(info.opBlend);
    }
    colorBlendAttachment.colorWriteMask = (info.colorMask.x > 0 ? VK_COLOR_COMPONENT_R_BIT : 0) |
                                           (info.colorMask.y > 0 ? VK_COLOR_COMPONENT_G_BIT : 0) |
                                           (info.colorMask.z > 0 ? VK_COLOR_COMPONENT_B_BIT : 0) |
                                           (info.colorMask.w > 0 ? VK_COLOR_COMPONENT_A_BIT : 0);

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = (info.depthTest != DepthTest::DISABLE) ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = info.depthMask ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = GetVulkanCompareOp(info.depthTest);

    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    std::vector<VkDescriptorSetLayout> layouts;
    for(int i = 0; i < info.bindGroupLayoutCount; i++){
        auto index = info.bindGroupLayouts[i];
        VkDescriptorSetLayout layout = bindGroupLayoutPool.Get(index).layout;
        layouts.push_back(layout);
    }

    VkPipelineLayoutCreateInfo layoutInfo = vkinit::pipeline_layout_create_info();
    layoutInfo.setLayoutCount = layouts.size();
    layoutInfo.pSetLayouts = layouts.data();

    VkPipelineLayout layout;
    VK_CHECK(vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &layout));

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;

    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr;
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr;

    VkRenderPass renderPass = GetOrCreate(info.framebufferLayout);
    Assert(renderPass != VK_NULL_HANDLE);

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending; //vkinit::pipeline_color_blend_state_create_info(1, &colorBlendAttachment);
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = renderPass; //_renderPass;

    VkPipeline pipeline;
    VK_CHECK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

    vkDestroyShaderModule(_device, vertModule, nullptr);
    vkDestroyShaderModule(_device, fragModule, nullptr);

    data.pipeline = pipeline;
    data.layout = layout;
    data.info = info;

    return true;
}

void VulkanGPUDevice::_DestroyPipeline(PipelineData& data){
    Assert(data.pipeline != VK_NULL_HANDLE);
    Assert(data.layout != VK_NULL_HANDLE);

    vkDestroyPipeline(_device, data.pipeline, nullptr);
    vkDestroyPipelineLayout(_device, data.layout, nullptr);
    data.pipeline = VK_NULL_HANDLE;
    data.layout = VK_NULL_HANDLE;
}

#pragma endregion

#pragma region Buffer

bool VulkanGPUDevice::_CreateBuffer(BufferData& data, size_t size, BufferUsage usage, BufferMemory memory){
    // --------------------------------------------------
    // Create Vulkan buffer
    // --------------------------------------------------

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = GetVulkanBufferUsage(usage);
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // GPU-only buffers are updated through staging copies.
    if(memory == BufferMemory::GPUOnly){
        bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    // --------------------------------------------------
    // Allocate memory
    // --------------------------------------------------

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = GetVulkanMemoryUsage(memory);

    // Keep CPU-visible buffers persistently mapped.
    if(memory != BufferMemory::GPUOnly){
        allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

    VK_CHECK(vmaCreateBuffer(
        _allocator,
        &bufferInfo,
        &allocInfo,
        &buffer,
        &allocation,
        nullptr
    ));

    // --------------------------------------------------
    // Store resource
    // --------------------------------------------------

    data.buffer = buffer;
    data.allocation = allocation;
    data.usage = usage;
    data.memory = memory;
    data.size = size;

    return true;
}

void VulkanGPUDevice::_UpdatedBuffer(BufferData& bufferData, const void* data, size_t size){
    if(!data || size == 0){
        return;
    }

    /*if(cmd.updateBuffer.offset + cmd.updateBuffer.size > bufferData.size) {
        LogError("UpdateBuffer exceeds buffer size");
        break;
    }*/

    // CPU visible buffer
    if (bufferData.memory != BufferMemory::GPUOnly) {

        void* mappedData = nullptr;
        bool needUnmap = false;

        VmaAllocationInfo allocInfo{};
        vmaGetAllocationInfo(
            _allocator,
            bufferData.allocation,
            &allocInfo
        );

        mappedData = allocInfo.pMappedData;

        if (!mappedData) {
            VK_CHECK(vmaMapMemory(
                _allocator,
                bufferData.allocation,
                &mappedData
            ));

            needUnmap = true;
        }

        std::memcpy(
            static_cast<char*>(mappedData), // + cmd.updateBuffer.offset,
            data,
            size
        );

        vmaFlushAllocation(
            _allocator,
            bufferData.allocation,
            0, //cmd.updateBuffer.offset,
            size
        );

        if (needUnmap) {
            vmaUnmapMemory(
                _allocator,
                bufferData.allocation
            );
        }
    }
    // GPU-only buffer
    else {

        VkBufferCreateInfo stagingInfo{};
        stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingInfo.size = size;
        stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo stagingAllocInfo{};
        stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation;
        VmaAllocationInfo stagingResultInfo{};

        VK_CHECK(vmaCreateBuffer(
            _allocator,
            &stagingInfo,
            &stagingAllocInfo,
            &stagingBuffer,
            &stagingAllocation,
            &stagingResultInfo
        ));

        // CPU -> staging
        std::memcpy(
            stagingResultInfo.pMappedData,
            data,
            size
        );

        vmaFlushAllocation(
            _allocator,
            stagingAllocation,
            0,
            size
        );

        // staging -> GPU buffer
        immediate_submit([&](VkCommandBuffer transferCmd) {

            VkBufferCopy copy{};
            copy.srcOffset = 0;
            copy.dstOffset = 0; //cmd.updateBuffer.offset;
            copy.size = size;

            vkCmdCopyBuffer(
                transferCmd,
                stagingBuffer,
                bufferData.buffer,
                1,
                &copy
            );
        });

        vmaDestroyBuffer(
            _allocator,
            stagingBuffer,
            stagingAllocation
        );
    }

}

void VulkanGPUDevice::_DestroyBuffer(BufferData& data){
    vmaDestroyBuffer(_allocator, data.buffer, data.allocation);
    data.buffer = VK_NULL_HANDLE;
    data.allocation = VK_NULL_HANDLE;
}

#pragma endregion

#pragma region RenderPass

VkFormat ToVkColorFormat(FramebufferTextureFormat format){
    switch (format){
        case FramebufferTextureFormat::None:
            return VK_FORMAT_UNDEFINED;

        case FramebufferTextureFormat::RGB:
            return VK_FORMAT_R8G8B8_UNORM;

        case FramebufferTextureFormat::RGBA8:
            return VK_FORMAT_R8G8B8A8_UNORM;

        case FramebufferTextureFormat::RGB11B10F:
            return VK_FORMAT_B10G11R11_UFLOAT_PACK32;

        case FramebufferTextureFormat::RGB16F:
            return VK_FORMAT_R16G16B16_SFLOAT;

        case FramebufferTextureFormat::RGBA16F:
            return VK_FORMAT_R16G16B16A16_SFLOAT;

        case FramebufferTextureFormat::RGB32F:
            return VK_FORMAT_R32G32B32_SFLOAT;

        case FramebufferTextureFormat::RGBA32F:
            return VK_FORMAT_R32G32B32A32_SFLOAT;

        case FramebufferTextureFormat::RED_INTEGER:
            return VK_FORMAT_R32_SINT;

        default:
            return VK_FORMAT_UNDEFINED;
    }
}

VkFormat ToVkDepthFormat(FramebufferDepthTextureFormat format){
    switch (format){
        case FramebufferDepthTextureFormat::None:
            return VK_FORMAT_UNDEFINED;

        case FramebufferDepthTextureFormat::DEPTH24_STENCIL8:
            return VK_FORMAT_D24_UNORM_S8_UINT;

        case FramebufferDepthTextureFormat::DEPTH32F_STENCIL8:
            return VK_FORMAT_D32_SFLOAT_S8_UINT;

        case FramebufferDepthTextureFormat::DEPTH_COMPONENT16:
            return VK_FORMAT_D16_UNORM;

        //case FramebufferDepthTextureFormat::DEPTH_COMPONENT24:
        //    return VK_FORMAT_D24_UNORM;

        case FramebufferDepthTextureFormat::DEPTH_COMPONENT32:
            return VK_FORMAT_D32_SFLOAT;

        case FramebufferDepthTextureFormat::DEPTH_COMPONENT32F:
            return VK_FORMAT_D32_SFLOAT;

        default:
            return VK_FORMAT_UNDEFINED;
    }
}

VkSampleCountFlagBits ToVkSampleCount(uint8_t samples){
    switch (samples){
        case 1:  return VK_SAMPLE_COUNT_1_BIT;
        case 2:  return VK_SAMPLE_COUNT_2_BIT;
        case 4:  return VK_SAMPLE_COUNT_4_BIT;
        case 8:  return VK_SAMPLE_COUNT_8_BIT;
        case 16: return VK_SAMPLE_COUNT_16_BIT;
        case 32: return VK_SAMPLE_COUNT_32_BIT;
        case 64: return VK_SAMPLE_COUNT_64_BIT;
        default: return VK_SAMPLE_COUNT_1_BIT;
    }
}

VkRenderPass CreateRenderPass(VkDevice device, const FrameBufferLayout& layout, VkFormat swapchainFormat = VK_FORMAT_UNDEFINED){
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorReferences;

    VkAttachmentReference depthReference{};
    bool hasDepth = false;

    VkSampleCountFlagBits samples =
        ToVkSampleCount(layout.samples);

    // ------------------------------------------------------------
    // Color attachments
    // ------------------------------------------------------------
    for(uint32_t i = 0; i < layout.colorAttachmentsCount; ++i){
        const auto& color = layout.colorAttachments[i];

        VkFormat format;

        if(layout.swapChainTarget && i == 0){
            Assert(swapchainFormat != VK_FORMAT_UNDEFINED);
            format = swapchainFormat;
        } else {
            format = ToVkColorFormat(color.format);
        }

        VkAttachmentDescription attachment{};
        attachment.format = format;
        attachment.samples = samples;
        // We don't care about the previous contents.
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // Keep the rendered result.
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if(layout.swapChainTarget && i == 0){
            attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        } else {
            attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        attachments.push_back(attachment);

        VkAttachmentReference reference{};
        reference.attachment = i;
        reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        colorReferences.push_back(reference);
    }

    // ------------------------------------------------------------
    // Depth attachment
    // ------------------------------------------------------------
    if(layout.depthAttachment.format != FramebufferDepthTextureFormat::None){
        hasDepth = true;

        VkAttachmentDescription attachment{};
        attachment.format = ToVkDepthFormat(layout.depthAttachment.format);
        attachment.samples = samples;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthReference.attachment = static_cast<uint32_t>(attachments.size());
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back(attachment);
    }

    // ------------------------------------------------------------
    // Subpass
    // ------------------------------------------------------------
    VkSubpassDescription subpass{};

    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
    subpass.pColorAttachments = colorReferences.empty() ? nullptr : colorReferences.data();

    if(hasDepth){
        subpass.pDepthStencilAttachment = &depthReference;
    }

    // ------------------------------------------------------------
    // Render pass
    // ------------------------------------------------------------
    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VK_CHECK(vkCreateRenderPass(device, &createInfo, nullptr, &renderPass));

    return renderPass;
}

#pragma endregion

#pragma region Texture2D

VkFormat GetImageFormat(ImageFormat f){
    switch(f){
        case ImageFormat::R8_UNORM: return VK_FORMAT_R8_UNORM;

        case ImageFormat::R8G8B8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM; //VK_FORMAT_R8G8B8_UNORM;
        case ImageFormat::R8G8B8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB; //VK_FORMAT_R8G8B8_SRGB;

        case ImageFormat::R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
        case ImageFormat::R8G8B8A8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
    }

    return VK_FORMAT_R8G8B8A8_UNORM;
}

std::vector<uint8_t> ConvertRGBToRGBA(const uint8_t* rgb, uint32_t width, uint32_t height){
    const size_t pixelCount = static_cast<size_t>(width) * height;
    std::vector<uint8_t> rgba(pixelCount * 4);

    for(size_t i = 0; i < pixelCount; i++){
        rgba[i * 4 + 0] = rgb[i * 3 + 0];
        rgba[i * 4 + 1] = rgb[i * 3 + 1];
        rgba[i * 4 + 2] = rgb[i * 3 + 2];
        rgba[i * 4 + 3] = 255;
    }

    return rgba;
}

bool VulkanGPUDevice::_CreateTexture2D(Texture2DData& texData, const Texture2DInfo& info){
    texData.info = info;

    //the format R8G8B8A8 matches exactly with the pixels loaded from stb_image lib
    VkFormat image_format = GetImageFormat(info.format);

    texData.width = info.width;
    texData.height = info.height;

    VkExtent3D imageExtent;
    imageExtent.width = info.width;
    imageExtent.height = info.height;
    imageExtent.depth = 1;

    VkImageCreateInfo dimg_info = vkinit::image_create_info(image_format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);

    VmaAllocationCreateInfo dimg_allocinfo = {};
    dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    //allocate and create the image
    VK_CHECK(vmaCreateImage(_allocator, &dimg_info, &dimg_allocinfo, &texData.image, &texData.allocation, nullptr));

    VkImageViewCreateInfo imageinfo = vkinit::imageview_create_info(image_format, texData.image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(_device, &imageinfo, nullptr, &texData.imageView));

    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR);// VK_FILTER_NEAREST);
    VK_CHECK(vkCreateSampler(_device, &samplerInfo, nullptr, &texData.sampler));

    return true;
} 

void VulkanGPUDevice::_UploadTexture2D(Texture2DData& texData, const void* data, size_t size){
    AllocatedBuffer staging = create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    void* mapped = nullptr;
    vmaMapMemory(_allocator, staging._allocation, &mapped);
    memcpy(mapped, data, size);
    vmaUnmapMemory(_allocator, staging._allocation);
    VkExtent3D extent{texData.width, texData.height, 1};
    immediate_submit([&](VkCommandBuffer command){
        VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image = texData.image; barrier.subresourceRange = range;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        VkBufferImageCopy copy{};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}; copy.imageExtent = extent;
        vkCmdCopyBufferToImage(command, staging._buffer, texData.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    });
    vmaDestroyBuffer(_allocator, staging._buffer, staging._allocation);
}

void VulkanGPUDevice::_DestroyTexture2D(Texture2DData& data){
    Assert(data.sampler != VK_NULL_HANDLE);
    Assert(data.imageView != VK_NULL_HANDLE);
    Assert(data.image != VK_NULL_HANDLE);

    vkDestroySampler(_device, data.sampler, nullptr);
    vkDestroyImageView(_device, data.imageView, nullptr);
    vmaDestroyImage(_allocator, data.image, data.allocation);
    data.sampler = VK_NULL_HANDLE;
    data.imageView = VK_NULL_HANDLE;
    data.image = VK_NULL_HANDLE;
} 

#pragma endregion

#pragma region BindGroupLayout
bool VulkanGPUDevice::_CreateBindGroupLayout(BindGroupLayoutData& data, BindGroupLayoutInfo& info){
    data.info = info;

    auto Convert = [](BindLayoutEntry& e) -> VkDescriptorSetLayoutBinding{
        VkDescriptorSetLayoutBinding entry = {};
        entry.binding = e.binding;
        entry.descriptorCount = 1;
        entry.descriptorType = GetVulkanDescriptorType(e.type);// VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        entry.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        return entry;
    };

    std::vector<VkDescriptorSetLayoutBinding> entries;
    for(int i = 0; i < info.entriesCount; i++){
        auto& e = info.entries[i];
        
        //LogInfo("binding={} type={}", e.binding, static_cast<int>(e.type));
        entries.push_back(Convert(e));
    }

    VkDescriptorSetLayoutCreateInfo setinfo = {};
    setinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setinfo.pNext = nullptr;
    setinfo.flags = 0; //no flags
    setinfo.pBindings = entries.data();
    setinfo.bindingCount = entries.size();

    VK_CHECK(vkCreateDescriptorSetLayout(_device, &setinfo, nullptr, &data.layout));

    return true;
}

void VulkanGPUDevice::_DestroyBindGroupLayout(BindGroupLayoutData& data){
    vkDestroyDescriptorSetLayout(_device, data.layout, nullptr);
    data.layout = VK_NULL_HANDLE;
}
#pragma endregion

#pragma region BindGroup

bool VulkanGPUDevice::_CreateBindGroup(BindGroupData& data, BindGroupInfo& info, VkDescriptorPool pool){
    data.info = info;
    BindGroupLayoutData& layoutData = bindGroupLayoutPool.Get(info.layout);
    
    VkDescriptorSetAllocateInfo allocInfo ={};
    allocInfo.pNext = nullptr;
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layoutData.layout;
    VK_CHECK(vkAllocateDescriptorSets(_device, &allocInfo, &data.descriptorSet));

    std::vector<VkDescriptorBufferInfo> bInfos;
    std::vector<VkDescriptorImageInfo> imageInfos;
    std::vector<VkWriteDescriptorSet> writes;
    bInfos.resize(info.entriesCount);
    imageInfos.resize(info.entriesCount);
    writes.resize(info.entriesCount);

    for(int i = 0; i < info.entriesCount; i++){
        if(layoutData.info.entries[i].type == BindingType::UniformBuffer){
            BufferData& bufferData = bufferPool.Get(info.entries[i].buffer);

            VkDescriptorBufferInfo& binfo = bInfos[i];
            binfo = {};
            binfo.buffer = bufferData.buffer;// _frames[i].cameraBuffer._buffer;
            binfo.offset = info.entries[i].offset;
            binfo.range = info.entries[i].size;// ssizeof(GPUCameraData);

            VkWriteDescriptorSet& setWrite = writes[i];
            setWrite = {};
            setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            setWrite.pNext = nullptr;
            setWrite.dstBinding = info.entries[i].binding;
            setWrite.dstSet = data.descriptorSet;
            setWrite.descriptorCount = 1;
            setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            setWrite.pBufferInfo = &binfo;
        } else if(layoutData.info.entries[i].type == BindingType::Texture2D){
            VkImageView imageView = VK_NULL_HANDLE;
            VkSampler sampler = VK_NULL_HANDLE; 

            if(info.entries[i].texture != InvalidID){
                Texture2DData& texData = texture2DPool.Get(info.entries[i].texture);
                imageView = texData.imageView;
                sampler = texData.sampler;
            }

            if(info.entries[i].framebuffer != InvalidID){
                FramebufferData& framebufferData = framebufferPool.Get(info.entries[i].framebuffer);
                int attacment = info.entries[i].framebufferAttacement;
                imageView = attacment < 0 ? framebufferData.depthAttachment.imageView : framebufferData.colorAttachments[attacment].imageView;
                sampler = attacment < 0 ? framebufferData.depthAttachment.sampler : framebufferData.colorAttachments[attacment].sampler;
            }

            VkDescriptorImageInfo& imageBufferInfo = imageInfos[i];
            imageBufferInfo.sampler = sampler;
            imageBufferInfo.imageView = imageView;
            imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet& setWrite = writes[i];
            setWrite = vkinit::write_descriptor_image(
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
                data.descriptorSet, 
                &imageBufferInfo, 
                info.entries[i].binding
            );
        } else {
            Assert(false);
        }
    }

    vkUpdateDescriptorSets(_device, writes.size(), writes.data(), 0, nullptr);

    return true;
}

#pragma endregion

#pragma region Framebuffer

uint64_t HashCombineByte(uint64_t hash, uint8_t value){
    hash ^= value;
    hash *= 1099511628211ull; // FNV-1a prime
    return hash;
}

uint64_t GetFramebufferLayoutHash(const FrameBufferLayout& layout){
    uint64_t hash = 14695981039346656037ull; // FNV-1a offset basis

    // Number of color attachments
    hash = HashCombineByte(hash, layout.colorAttachmentsCount);

    // Color attachments
    for (uint32_t i = 0; i < layout.colorAttachmentsCount; ++i)
    {
        const FramebufferAttachment& attachment =
            layout.colorAttachments[i];

        hash = HashCombineByte(
            hash,
            static_cast<uint8_t>(attachment.format));

        hash = HashCombineByte(
            hash,
            attachment.mipLevels);
    }

    // Depth attachment
    hash = HashCombineByte(
        hash,
        static_cast<uint8_t>(layout.depthAttachment.format));

    hash = HashCombineByte(
        hash,
        layout.depthAttachment.mipLevels);

    hash = HashCombineByte(
        hash,
        layout.samples);

    return hash;
}

VkRenderPass VulkanGPUDevice::GetOrCreate(const FrameBufferLayout& layout){
    uint64_t hash = GetFramebufferLayoutHash(layout);
    for(int i = 0; i < renderPasses.size(); i++){
        if(renderPasses[i].hash == hash) return renderPasses[i].renderPass;
    }

    uint32_t newId = renderPasses.size();
    RenderPasses pass = {};
    pass.hash = hash;
    pass.id = newId;
    pass.renderPass = CreateRenderPass(_device, layout);
    renderPasses.push_back(pass);
    return pass.renderPass;
}

bool VulkanGPUDevice::_CreateFramebuffer(FramebufferData& data, const FrameBufferCreateInfo& info){
    //VulkanFramebuffer result{};

    data.width = info.width;
    data.height = info.height;
    data.layout = info.layout;

    const FrameBufferLayout& layout = info.layout;

    VkRenderPass pass = VK_NULL_HANDLE;

    // ------------------------------------------------------------
    // Render pass
    // ------------------------------------------------------------
    data.renderPass = GetOrCreate(layout);

    // ------------------------------------------------------------
    // Color attachments
    // ------------------------------------------------------------
    data.colorAttachments.resize(layout.colorAttachmentsCount);

    for(uint32_t i = 0; i < layout.colorAttachmentsCount; ++i){
        const FramebufferAttachment& attachment = layout.colorAttachments[i];

        auto& vkAttachment = data.colorAttachments[i];

        VkFormat format = ToVkColorFormat(attachment.format);

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = format;
        imageInfo.extent.width = info.width;
        imageInfo.extent.height = info.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = attachment.mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = ToVkSampleCount(layout.samples);
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        //VK_CHECK(vkCreateImage(_device, &imageInfo, nullptr, &vkAttachment.image));

        // Allocate memory here...
        // vkGetImageMemoryRequirements()
        // vkAllocateMemory()
        // vkBindImageMemory()
        VmaAllocationCreateInfo dimg_allocinfo = {};
        dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        VK_CHECK(vmaCreateImage(_allocator, &imageInfo, &dimg_allocinfo, &vkAttachment.image, &vkAttachment.allocation, nullptr));

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = vkAttachment.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = attachment.mipLevels;

        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView(_device, &viewInfo, nullptr, &vkAttachment.imageView));

        VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR);// VK_FILTER_NEAREST);
        VK_CHECK(vkCreateSampler(_device, &samplerInfo, nullptr, &vkAttachment.sampler));
    }

    // ------------------------------------------------------------
    // Depth attachment
    // ------------------------------------------------------------

    if (layout.depthAttachment.format != FramebufferDepthTextureFormat::None){
        const auto& attachment = layout.depthAttachment;

        VkFormat format = ToVkDepthFormat(attachment.format);

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = format;
        imageInfo.extent.width = info.width;
        imageInfo.extent.height = info.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = attachment.mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = ToVkSampleCount(layout.samples);
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        //VK_CHECK(vkCreateImage(_device, &imageInfo, nullptr, &data.depthAttachment.image));

        // Allocate/bind memory here...
        VmaAllocationCreateInfo dimg_allocinfo = {};
        dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        VK_CHECK(vmaCreateImage(_allocator, &imageInfo, &dimg_allocinfo, &data.depthAttachment.image, &data.depthAttachment.allocation, nullptr));

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = data.depthAttachment.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = attachment.mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView(_device, &viewInfo, nullptr, &data.depthAttachment.imageView));

        VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR);// VK_FILTER_NEAREST);
        VK_CHECK(vkCreateSampler(_device, &samplerInfo, nullptr, &data.depthAttachment.sampler));
    }

    // ------------------------------------------------------------
    // VkFramebuffer
    // ------------------------------------------------------------

    std::vector<VkImageView> views;

    for(auto& attachment: data.colorAttachments){
        views.push_back(attachment.imageView);
    }

    if(data.depthAttachment.imageView != VK_NULL_HANDLE){
        views.push_back(data.depthAttachment.imageView);
    }

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = data.renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(views.size());
    framebufferInfo.pAttachments = views.data();
    framebufferInfo.width = info.width;
    framebufferInfo.height = info.height;
    framebufferInfo.layers = 1;
    VK_CHECK(vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &data.framebuffer));

    return true;
}

void VulkanGPUDevice::_DestroyFramebuffer(FramebufferData& data){
    vkDestroyFramebuffer(_device, data.framebuffer, nullptr);
    data.framebuffer = VK_NULL_HANDLE;
}

#pragma endregion

#pragma region Device
VulkanGPUDevice::VulkanGPUDevice() {
    vkInfo.apiName = "Vulkan";
    vkInfo.version = 1;
    vkInfo.supportUniformBuffer = true;

    _windowFrameBufferLayout = {};
    _windowFrameBufferLayout.colorAttachments[0].format = FramebufferTextureFormat::RGBA8;
    _windowFrameBufferLayout.colorAttachmentsCount = 1;
    _windowFrameBufferLayout.depthAttachment.format = FramebufferDepthTextureFormat::None;
}

void create_allocator(){
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = _chosenGPU;
    allocatorInfo.device = _device;
    allocatorInfo.instance = _instance;
    vmaCreateAllocator(&allocatorInfo, &_allocator);
}

void init_vulkan(){
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("OD Engine Vulkan")
        .require_api_version(1, 1, 0)
        .request_validation_layers(true)
        .use_default_debug_messenger()
        .build();

    vkb::Instance vkb_inst = inst_ret.value();
    _instance = vkb_inst.instance;
    _debug_messenger = vkb_inst.debug_messenger;

    Platform::CreateVulkanSurface(_instance, &_surface);

    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1, 1).set_surface(_surface).select().value();

    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();

    _device = vkbDevice.device;
    _chosenGPU = physicalDevice.physical_device;

    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    create_allocator();
}

void init_swapchain(){
    vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU, _device, _surface };
    vkb::Swapchain vkbSwapchain = swapchainBuilder
        //.use_default_format_selection()
        .set_desired_format({ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(_windowExtent.width, _windowExtent.height)
        .build()
        .value();

    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();
    _swapchainImageFormat = vkbSwapchain.image_format;
}

void init_commands(){
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_commandPool));

    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_commandPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_mainCommandBuffer));

    //create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers
	//VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for(int i = 0; i < FRAME_OVERLAP; i++){
		VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));
		//allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);
		VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
	}

    VkCommandPoolCreateInfo uploadCommandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily);
	//create pool for upload context
	VK_CHECK(vkCreateCommandPool(_device, &uploadCommandPoolInfo, nullptr, &_uploadContext._commandPool));
	mainDeletionQueue.push_function([=](){ vkDestroyCommandPool(_device, _uploadContext._commandPool, nullptr); });

	//allocate the default command buffer that we will use for the instant commands
	VkCommandBufferAllocateInfo cmdAllocInfo2 = vkinit::command_buffer_allocate_info(_uploadContext._commandPool, 1);
	VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo2, &_uploadContext._commandBuffer));
}

void VulkanGPUDevice::InitDefaultRenderpass(){
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = _swapchainImageFormat;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_renderPass));

    Assert(renderPasses.size() == 0);
    RenderPasses pass = {};
    pass.hash = GetFramebufferLayoutHash(_windowFrameBufferLayout);
    pass.id = renderPasses.size();
    pass.renderPass = _renderPass;
    renderPasses.push_back(pass);
}

void VulkanGPUDevice::InitFramebuffers(){
    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.renderPass = _renderPass;
    fb_info.attachmentCount = 1;
    fb_info.width = _windowExtent.width;
    fb_info.height = _windowExtent.height;
    fb_info.layers = 1;

    const uint32_t swapchain_imagecount = _swapchainImages.size();
    _framebuffers.resize(swapchain_imagecount);

    for(size_t i = 0; i < swapchain_imagecount; i++){
        fb_info.pAttachments = &_swapchainImageViews[i];
        VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr, &_framebuffers[i]));
    }
}

void init_sync_structures(){
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    uint32_t imageCount = _swapchainImages.size();
    _renderSemaphores.resize(imageCount);

    for(size_t i = 0; i < imageCount; i++){
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_renderSemaphores[i]));
    }

	for(int i = 0; i < FRAME_OVERLAP; i++){     
        VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._presentSemaphore));
	}

    VkFenceCreateInfo uploadFenceCreateInfo = vkinit::fence_create_info();
	VK_CHECK(vkCreateFence(_device, &uploadFenceCreateInfo, nullptr, &_uploadContext._uploadFence));
    mainDeletionQueue.push_function([=](){ vkDestroyFence(_device, _uploadContext._uploadFence, nullptr); });
}

void VulkanGPUDevice::InitDescriptors(){
    //create a descriptor pool that will hold 10 uniform buffers
	std::vector<VkDescriptorPoolSize> sizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
		//add combined-image-sampler descriptor types to the pool
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = 0;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)sizes.size();
	pool_info.pPoolSizes = sizes.data();
	vkCreateDescriptorPool(_device, &pool_info, nullptr, &_descriptorPool);
    vkCreateDescriptorPool(_device, &pool_info, nullptr, &frameDescriptorPools);
}

void VulkanGPUDevice::Cleanup(){
    if(!_isInitialized) return;

    glslang::FinalizeProcess();
    vkDeviceWaitIdle(_device);

    mainDeletionQueue.flush();

    vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
    vkDestroyDescriptorPool(_device, frameDescriptorPools, nullptr);

    bufferPool.ForEach([&](uint32_t id, BufferData& data){
        if(data.buffer != VK_NULL_HANDLE){
            vmaDestroyBuffer(_allocator, data.buffer, data.allocation);
            data.buffer = VK_NULL_HANDLE;
        }
    });

    pipelinePool.ForEach([&](uint32_t id, PipelineData& data){
        _DestroyPipeline(data);
    });

    bindGroupLayoutPool.ForEach([&](uint32_t id, BindGroupLayoutData& data){ 
        if(data.layout != VK_NULL_HANDLE){
            vkDestroyDescriptorSetLayout(_device, data.layout, nullptr);
            data.layout = VK_NULL_HANDLE;
        }
    });

    texture2DPool.ForEach([&](uint32_t id, Texture2DData& data){
        if(data.image == VK_NULL_HANDLE) return;
        _DestroyTexture2D(data);
    });

    vmaDestroyAllocator(_allocator);

    vkDestroyCommandPool(_device, _commandPool, nullptr);

    for(size_t i = 0; i < _swapchainImages.size(); i++){
        vkDestroySemaphore(_device, _renderSemaphores[i], nullptr);
    }

    for(auto& i: _frames){
        vkDestroyFence(_device, i._renderFence, nullptr);
        vkDestroySemaphore(_device, i._presentSemaphore, nullptr);
        vkDestroyCommandPool(_device, i._commandPool, nullptr);
    }

    vkDestroySwapchainKHR(_device, _swapchain, nullptr);
    vkDestroyRenderPass(_device, _renderPass, nullptr);

    for(size_t i = 0; i < _framebuffers.size(); i++){
        vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);
        vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
    }

    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyDevice(_device, nullptr);
    vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
    vkDestroyInstance(_instance, nullptr);
}

void VulkanGPUDevice::_Init(){
    LogInfo("VulkanGPUDevice::Initialize");

    _windowExtent.width = Application::ScreenWidth();
    _windowExtent.height = Application::ScreenHeight();

    init_vulkan();
    init_swapchain();
    init_commands();
    InitDefaultRenderpass();
    InitFramebuffers();
    init_sync_structures();
    InitDescriptors();
    _isInitialized = true;
}

void VulkanGPUDevice::_Shut(){
    LogInfo("VulkanGPUDevice::Shut");
    Cleanup();
}

void VulkanGPUDevice::Init(bool inmultithread){
    multithread = inmultithread;

    multithreadRendererContext.StartupFrames();

    if(multithread){
        multithreadRendererContext.init = [&](){ _Init(); };
        multithreadRendererContext.shut = [&](){ _Shut(); };
        multithreadRendererContext.runRender = [&](RenderFrame& f){  RunRender(f); f.Clear(); Platform::SwapBuffers(); };
        multithreadRendererContext.Init();
    } else {
        _Init();
    }
}

void VulkanGPUDevice::Shut(){
    if(multithread){
        multithreadRendererContext.Shut();
    } else {
        _Shut();
    }
}

void VulkanGPUDevice::StartRender(){
    if(multithread){
        multithreadRendererContext.StartRender();
    } else {
        SyncSingleThreadData();
    }
}

void VulkanGPUDevice::UpdateRender(){
    if(multithread){
        multithreadRendererContext.WaitForRender();
        multithreadRendererContext.SwapRenderFrames();
        SyncSingleThreadData();
    } else {
        RunRender(*multithreadRendererContext.simulationFrame);
        multithreadRendererContext.simulationFrame->Clear();
        Platform::SwapBuffers();
    }
}

CommandBuffer* VulkanGPUDevice::GetCommandBuffer(){
    return &multithreadRendererContext.simulationFrame->renderCommands; 
}

FrameBufferLayout VulkanGPUDevice::GetWindowFrameBufferLayout(){ 
    return _windowFrameBufferLayout; 
}

void VulkanGPUDevice::RunRender(RenderFrame& frame){
    Pipeline currentPipeline = INVALID_ID;

    VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

    for(auto i: frameBindGroups){
        bindGroupPool.AddDestroyedId(i);
    }
    frameBindGroups.clear();
    vkResetDescriptorPool(_device, frameDescriptorPools, 0);
    
    // Process Resource Commands first
    for(const ResourceCommands::Command& cmd : frame.resourceCommands.commands){
        switch(cmd.type){
            case ResourceCommands::Type::CreateBuffer:{
                Assert(bufferPool.IsValid(cmd.createBuffer.id));
                auto& data = bufferPool.Get(cmd.createBuffer.id);
                if(!_CreateBuffer(data, cmd.createBuffer.size, cmd.createBuffer.usage, cmd.createBuffer.memory)){
                    bufferPool.AddDestroyedId(cmd.createBuffer.id);
                }
                break;
            }

            case ResourceCommands::Type::UpdateBuffer: {
                Assert(bufferPool.IsValid(cmd.updateBuffer.id));
                auto& bufferData = bufferPool.Get(cmd.updateBuffer.id);
                _UpdatedBuffer(bufferData, cmd.updateBuffer.data, cmd.updateBuffer.size);
                break;
            }

            case ResourceCommands::Type::DestroyBuffer:{
                if(!bufferPool.IsValid(cmd.destroyBuffer.id)){
                    LogError("DestroyBuffer: Invalid id!");
                    break;
                }

                Assert(bufferPool.IsValid(cmd.destroyBuffer.id));
                auto& data = bufferPool.Get(cmd.destroyBuffer.id);
                _DestroyBuffer(data);
                bufferPool.AddDestroyedId(cmd.destroyBuffer.id);
                break;
            }

            case ResourceCommands::Type::CreateTexture2D:{
                Assert(texture2DPool.IsValid(cmd.createTexture2D.id));
                auto& data = texture2DPool.Get(cmd.createTexture2D.id);
                if(!_CreateTexture2D(data, cmd.createTexture2D.info)){
                    texture2DPool.AddDestroyedId(cmd.createTexture2D.id);
                }
                break;
            }

            case ResourceCommands::Type::UploadTexture2D:{
                Assert(texture2DPool.IsValid(cmd.uploadTexture2D.id));
                auto& data = texture2DPool.Get(cmd.uploadTexture2D.id);

                if(data.info.format != ImageFormat::R8_UNORM){
                    auto rgba = ConvertRGBToRGBA((uint8_t*)cmd.uploadTexture2D.data, data.width, data.height);
                    _UploadTexture2D(data, rgba.data(), rgba.size());
                } else {
                    _UploadTexture2D(data, cmd.uploadTexture2D.data, cmd.uploadTexture2D.size);
                }
                break;
            }

            case ResourceCommands::Type::DestroyTexture2D:{
                Assert(texture2DPool.IsValid(cmd.destroyTexture2D.id));
                auto& data = texture2DPool.Get(cmd.destroyTexture2D.id);
                _DestroyTexture2D(data);
                texture2DPool.AddDestroyedId(cmd.destroyTexture2D.id);
                break;
            }

            case ResourceCommands::Type::CreatePipeline:{
                Assert(pipelinePool.IsValid(cmd.createPipeline.id));
                auto& data = pipelinePool.Get(cmd.createPipeline.id);
                if(!_CreatePipeline(data, cmd.createPipeline.source, cmd.createPipeline.info)){
                    pipelinePool.AddDestroyedId(cmd.createPipeline.id);
                }
                break;
            }

            case ResourceCommands::Type::DestroyPipeline:{
                Assert(pipelinePool.IsValid(cmd.destroyPipeline.id));
                auto& data = pipelinePool.Get(cmd.destroyPipeline.id);
                _DestroyPipeline(data);
                pipelinePool.AddDestroyedId(cmd.destroyPipeline.id);
                break;
            }
            
            case ResourceCommands::Type::CreateBindGroupLayout:{
                Assert(bindGroupLayoutPool.IsValid(cmd.createBindGroupLayout.id));
                BindGroupLayoutData& data = bindGroupLayoutPool.Get(cmd.createBindGroupLayout.id);
                if(!_CreateBindGroupLayout(data, *cmd.createBindGroupLayout.info)){
                    bindGroupLayoutPool.AddDestroyedId(cmd.createBindGroupLayout.id);
                }
                break;
            }

            case ResourceCommands::Type::DestroyBindGroupLayout:{
                Assert(bindGroupLayoutPool.IsValid(cmd.destroyBindGroupLayout.id));
                BindGroupLayoutData& data = bindGroupLayoutPool.Get(cmd.destroyBindGroupLayout.id);
                _DestroyBindGroupLayout(data);
                bindGroupLayoutPool.AddDestroyedId(cmd.destroyBindGroupLayout.id);
                break;
            }

            case ResourceCommands::Type::CreateBindGroup:{
                Assert(bindGroupPool.IsValid(cmd.createBindGroup.id));
                auto& data = bindGroupPool.Get(cmd.createBindGroup.id);
                _CreateBindGroup(data, *cmd.createBindGroup.info, _descriptorPool);
                break;
            }

            case ResourceCommands::Type::CreateFrameBindGroup:{
                Assert(bindGroupPool.IsValid(cmd.createFrameBindGroup.id));
                auto& data = bindGroupPool.Get(cmd.createFrameBindGroup.id);
                _CreateBindGroup(data, *cmd.createFrameBindGroup.info, frameDescriptorPools);
                frameBindGroups.push_back(cmd.createFrameBindGroup.id);
                break;
            }
            
            case ResourceCommands::Type::CreateFramebuffer:{
                Assert(framebufferPool.IsValid(cmd.createFramebuffer.framebuffer));
                auto& data = framebufferPool.Get(cmd.createFramebuffer.framebuffer);
                if(!_CreateFramebuffer(data, cmd.createFramebuffer.info)){
                    framebufferPool.AddDestroyedId(cmd.createFramebuffer.framebuffer);
                }
                break;
            }

            case ResourceCommands::Type::DestroyFramebuffer:{
                Assert(framebufferPool.IsValid(cmd.destroyFramebuffer.id));
                auto& data = framebufferPool.Get(cmd.destroyFramebuffer.id);
                _DestroyFramebuffer(data);
                framebufferPool.AddDestroyedId(cmd.destroyFramebuffer.id);
                break;
            }
        }
    }

    //VK_CHECK(vkWaitForFences(_device, 1, &_renderFence, true, 1000000000));
    //VK_CHECK(vkResetFences(_device, 1, &_renderFence));

    VK_CHECK(vkResetCommandBuffer(get_current_frame()._mainCommandBuffer, 0));
    VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

    uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._presentSemaphore, nullptr, &swapchainImageIndex));

    VkCommandBufferBeginInfo cmdBeginInfo = {};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    /*VkClearValue clearValue{};
    clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

    VkRenderPassBeginInfo rpInfo = {};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = _renderPass;
    rpInfo.renderArea.offset = { 0, 0 };
    rpInfo.renderArea.extent = _windowExtent;
    rpInfo.framebuffer = _framebuffers[swapchainImageIndex];
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_windowExtent.width);
    viewport.height = static_cast<float>(_windowExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = _windowExtent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);*/

    FramebufferData* _currentFramebuffer = nullptr;

    bool hasWindowRenderPass = false;

    for(const CommandBuffer::Command& renderCmd : frame.renderCommands.commands){
        switch(renderCmd.type){
            case CommandBuffer::Type::Clear:{
                VkClearAttachment clearAttachment{};
                clearAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                clearAttachment.colorAttachment = 0;
                clearAttachment.clearValue.color = { {
                    renderCmd.clear.clearValue.color.x,
                    renderCmd.clear.clearValue.color.y,
                    renderCmd.clear.clearValue.color.z,
                    renderCmd.clear.clearValue.color.w
                } };

                VkClearRect clearRect{};
                clearRect.rect.offset = { 0, 0 };
                clearRect.rect.extent = _windowExtent;
                clearRect.baseArrayLayer = 0;
                clearRect.layerCount = 1;

                vkCmdClearAttachments(cmd, 1, &clearAttachment, 1, &clearRect);
                break;
            }

            case CommandBuffer::Type::Viewport:{
                #ifdef TestDrawInverted
                VkViewport customViewport{};
                customViewport.x = static_cast<float>(renderCmd.viewport.x);
                customViewport.y = static_cast<float>(renderCmd.viewport.y) + static_cast<float>(renderCmd.viewport.h);
                customViewport.width = static_cast<float>(renderCmd.viewport.w);
                customViewport.height = -static_cast<float>(renderCmd.viewport.h);
                customViewport.minDepth = 0.0f;
                customViewport.maxDepth = 1.0f;
                vkCmdSetViewport(cmd, 0, 1, &customViewport);

                VkRect2D customScissor{};
                customScissor.offset = { static_cast<int32_t>(renderCmd.viewport.x), static_cast<int32_t>(renderCmd.viewport.y) };
                customScissor.extent = { renderCmd.viewport.w, renderCmd.viewport.h };
                vkCmdSetScissor(cmd, 0, 1, &customScissor);
                #else

                VkViewport customViewport{};
                customViewport.x = static_cast<float>(renderCmd.viewport.x);
                customViewport.y = static_cast<float>(renderCmd.viewport.y);
                customViewport.width = static_cast<float>(renderCmd.viewport.w);
                customViewport.height = static_cast<float>(renderCmd.viewport.h);
                customViewport.minDepth = 0.0f;
                customViewport.maxDepth = 1.0f;
                vkCmdSetViewport(cmd, 0, 1, &customViewport);

                VkRect2D customScissor{};
                customScissor.offset = { static_cast<int32_t>(renderCmd.viewport.x), static_cast<int32_t>(renderCmd.viewport.y) };
                customScissor.extent = { renderCmd.viewport.w, renderCmd.viewport.h };
                vkCmdSetScissor(cmd, 0, 1, &customScissor);
                #endif
                break;
            }

            case CommandBuffer::Type::SetPipeline:{
                const auto& pipeData = pipelinePool.Get(renderCmd.setPipeline.id);
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeData.pipeline);
                currentPipeline = renderCmd.setPipeline.id;
                break;
            }

            case CommandBuffer::Type::SetVertexBuffer:{
                const auto& bufData = bufferPool.Get(renderCmd.setVertexBuffer.buffer);
                Assert(bufData.buffer != VK_NULL_HANDLE);
                Assert(renderCmd.setVertexBuffer.buffer != InvalidID);
                //Assert(renderCmd.setVertexBuffer.buffer < bufferPool.data.size());
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(cmd, renderCmd.setVertexBuffer.slot, 1, &bufData.buffer, offsets);
                break;
            }

            case CommandBuffer::Type::SetIndexBuffer:{
                const auto& bufData = bufferPool.Get(renderCmd.setIndexBuffer.buffer);
                vkCmdBindIndexBuffer(cmd, bufData.buffer, 0, VK_INDEX_TYPE_UINT32);
                break;
            }

            case CommandBuffer::Type::SetBindGroup:{
                const BindGroupData& bindGroupData = bindGroupPool.Get(renderCmd.setBindGroup.group);
                const PipelineData& pipelineData = pipelinePool.Get(currentPipeline);

                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.layout, renderCmd.setBindGroup.slot, 1, &bindGroupData.descriptorSet, 0, nullptr);
                break;
            };

            case CommandBuffer::Type::Draw:{
                vkCmdDraw(cmd, renderCmd.draw.vertexCount, 1, 0, 0);
                break;
            }

            case CommandBuffer::Type::DrawIndexed:{
                vkCmdDrawIndexed(cmd, renderCmd.drawIndexed.indexCount, 1, 0, 0, 0);
                break;
            }

            case CommandBuffer::Type::DrawInstanced:{
                vkCmdDraw(cmd, renderCmd.drawInstanced.vertexCount, renderCmd.drawInstanced.count, 0, 0);
                break;
            }

            case CommandBuffer::Type::DrawIndexedInstanced:{
                vkCmdDrawIndexed(cmd, renderCmd.drawIndexedInstanced.indexCount, renderCmd.drawIndexedInstanced.indexCount, 0, 0, 0);
                break;
            }
        
            case CommandBuffer::Type::BeginFramebuffer:{
                auto& data = framebufferPool.Get(renderCmd.beginFramebuffer.framebuffer); 
                const auto& layout = data.layout;

                _currentFramebuffer = &data;

                // ------------------------------------------------------------
                // Transition framebuffer attachments to attachment layouts
                // ------------------------------------------------------------

                std::vector<VkImageMemoryBarrier> barriers;

                // Color attachments
                for(uint32_t i = 0; i < layout.colorAttachmentsCount; ++i){
                    auto& attachment = data.colorAttachments[i];

                    VkImageMemoryBarrier barrier{};
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    if(attachment.initialized){
                        // Previous usage was shader read.
                        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    } else {
                        // First usage.
                        barrier.srcAccessMask = 0;
                        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    }
                    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = attachment.image;
                    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = layout.colorAttachments[i].mipLevels;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = 1;
                    barriers.push_back(barrier);
                }

                // Depth attachment
                if(layout.depthAttachment.format != FramebufferDepthTextureFormat::None){
                    auto& attachment = data.depthAttachment;

                    VkImageMemoryBarrier barrier{};
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    if(attachment.initialized){
                        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    } else {
                        barrier.srcAccessMask = 0;
                        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    }
                    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = attachment.image;
                    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = layout.depthAttachment.mipLevels;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = 1;
                    barriers.push_back(barrier);
                }
                if(!barriers.empty()){
                    vkCmdPipelineBarrier(
                        cmd,
                        // Source stage
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        // Destination stage
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        static_cast<uint32_t>(barriers.size()),
                        barriers.data()
                    );
                }

                std::vector<VkClearValue> clearValues;

                // Color clears
                for(uint32_t i = 0; i < layout.colorAttachmentsCount; ++i){
                    VkClearValue clear{};
                    clear.color.float32[0] = 0.0f;
                    clear.color.float32[1] = 0.0f;
                    clear.color.float32[2] = 1.0f;
                    clear.color.float32[3] = 1.0f;
                    clearValues.push_back(clear);
                }

                // Depth clear
                if(layout.depthAttachment.format != FramebufferDepthTextureFormat::None){
                    VkClearValue clear{};
                    clear.depthStencil.depth = 1.0f;
                    clear.depthStencil.stencil = 0;
                    clearValues.push_back(clear);
                }

                VkRenderPassBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                beginInfo.renderPass = data.renderPass;
                beginInfo.framebuffer = data.framebuffer;
                beginInfo.renderArea.offset = { 0, 0 };
                beginInfo.renderArea.extent = { data.width, data.height };
                beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
                beginInfo.pClearValues = clearValues.data();
                vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE );
                break;
            };

            case CommandBuffer::Type::BeginWindowFramebuffer:{
                hasWindowRenderPass = true;
                
                VkClearValue clearValue{};
                clearValue.color = { { 1.0f, 0.0f, 0.0f, 1.0f } };

                VkRenderPassBeginInfo rpInfo = {};
                rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                rpInfo.renderPass = _renderPass;
                rpInfo.renderArea.offset = { 0, 0 };
                rpInfo.renderArea.extent = _windowExtent;
                rpInfo.framebuffer = _framebuffers[swapchainImageIndex];
                rpInfo.clearValueCount = 1;
                rpInfo.pClearValues = &clearValue;

                vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

                /*VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.y = static_cast<float>(_windowExtent.height); //0.0f;
                viewport.width = static_cast<float>(_windowExtent.width);
                viewport.height = -static_cast<float>(_windowExtent.height);// static_cast<float>(_windowExtent.height);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(cmd, 0, 1, &viewport);

                VkRect2D scissor{};
                scissor.offset = { 0, 0 };
                scissor.extent = _windowExtent;
                vkCmdSetScissor(cmd, 0, 1, &scissor);*/
                break;
            }
        
            case CommandBuffer::Type::EndFramebuffer:{
                vkCmdEndRenderPass(cmd);

                // Window framebuffer is handled differently.
                // It must end in PRESENT_SRC_KHR.
                if(_currentFramebuffer == nullptr) break;

                FramebufferData& framebuffer = *_currentFramebuffer;

                const auto& layout = framebuffer.layout;

                std::vector<VkImageMemoryBarrier> barriers;
                // ------------------------------------------------------------
                // Color attachments
                //
                // COLOR_ATTACHMENT_OPTIMAL
                //          ↓
                // SHADER_READ_ONLY_OPTIMAL
                // ------------------------------------------------------------

                for(uint32_t i = 0; i < layout.colorAttachmentsCount; ++i) {
                    auto& attachment = framebuffer.colorAttachments[i];

                    VkImageMemoryBarrier barrier{};
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = attachment.image;
                    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = layout.colorAttachments[i].mipLevels;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = 1;
                    barriers.push_back(barrier);
                    attachment.initialized = true;
                }

                // ------------------------------------------------------------
                // Depth attachment
                //
                // DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                //          ↓
                // SHADER_READ_ONLY_OPTIMAL
                // ------------------------------------------------------------
                if(layout.depthAttachment.format != FramebufferDepthTextureFormat::None){
                    auto& attachment = framebuffer.depthAttachment;

                    VkImageMemoryBarrier barrier{};
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = attachment.image;
                    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = layout.depthAttachment.mipLevels;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = 1;
                    barriers.push_back(barrier);
                    attachment.initialized = true;
                }
                if(!barriers.empty()){
                    vkCmdPipelineBarrier(
                        cmd,
                        // Source
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                        // Destination
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        static_cast<uint32_t>(barriers.size()),
                        barriers.data()
                    );
                }

                _currentFramebuffer = nullptr;
                break;
            };
        }
    }

    if(hasWindowRenderPass == false){
        VkClearValue clearValue{};
        clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
        VkRenderPassBeginInfo rpInfo = {};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass = _renderPass;
        rpInfo.renderArea.offset = { 0, 0 };
        rpInfo.renderArea.extent = _windowExtent;
        rpInfo.framebuffer = _framebuffers[swapchainImageIndex];
        rpInfo.clearValueCount = 1;
        rpInfo.pClearValues = &clearValue;
        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

        /*VkClearAttachment clearAttachment{};
        clearAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        clearAttachment.colorAttachment = 0;
        clearAttachment.clearValue.color = { {
            1,
            1,
            1,
            1
        } };

        VkClearRect clearRect{};
        clearRect.rect.offset = { 0, 0 };
        clearRect.rect.extent = _windowExtent;
        clearRect.baseArrayLayer = 0;
        clearRect.layerCount = 1;

        vkCmdClearAttachments(cmd, 1, &clearAttachment, 1, &clearRect);*/

        vkCmdEndRenderPass(cmd);
    }

    //vkCmdEndRenderPass(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit.pWaitDstStageMask = &waitStage;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &get_current_frame()._presentSemaphore;
    submit.signalSemaphoreCount = 1;
    VkSemaphore renderSemaphore =_renderSemaphores[swapchainImageIndex];
    submit.pSignalSemaphores = &renderSemaphore; //&get_current_frame()._renderSemaphore;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &renderSemaphore; //;// &get_current_frame()._renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &swapchainImageIndex;

    VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));
    _frameNumber++;
}

void VulkanGPUDevice::SyncSingleThreadData(){
    bufferPool.SyncSingleThreadData();
    texture2DPool.SyncSingleThreadData();
    pipelinePool.SyncSingleThreadData();
    bindGroupLayoutPool.SyncSingleThreadData();
    bindGroupPool.SyncSingleThreadData();
    framebufferPool.SyncSingleThreadData();
}

Pipeline VulkanGPUDevice::CreatePipeline(const char* source, PipelineInfo info){   
    auto id = pipelinePool.AllocId();
    multithreadRendererContext.simulationFrame->resourceCommands.CreatePipeline(id, source, info);
    return id;
}

void VulkanGPUDevice::DestroyPipeline(Pipeline id){
    multithreadRendererContext.simulationFrame->resourceCommands.DestroyPipeline(id);
}

Buffer VulkanGPUDevice::CreateBuffer(size_t size, BufferUsage usage, BufferMemory memory){
    #ifdef DONT_DEFERRED_RESOURCE_CREATION

    BufferData data;
    if(!_CreateBuffer(data, size, usage, memory)) return InvalidID;
    auto id = bufferPool.AllocId();
    bufferPool.CpuPushResource(id, data);
    return id;

    #else

    auto id = bufferPool.AllocId();
    multithreadRendererContext.simulationFrame->resourceCommands.CreateBuffer(id, size, usage, memory);
    return id;

    #endif
}

void VulkanGPUDevice::UpdatedBuffer(Buffer buffer, const void* data, size_t size){
    multithreadRendererContext.simulationFrame->resourceCommands.UpdatedBuffer(buffer, data, size);
}

void VulkanGPUDevice::DestroyBuffer(Buffer id){
    multithreadRendererContext.simulationFrame->resourceCommands.DestroyBuffer(id);
}

Texture2D VulkanGPUDevice::CreateTexture2D(Texture2DInfo& info){
    #ifdef DONT_DEFERRED_RESOURCE_CREATION

    Texture2DData data;
    if(!_CreateTexture2D(data, info)) return InvalidID;
    auto id = texture2DPool.AllocId();
    texture2DPool.CpuPushResource(id, data);
    return id;

    #else

    auto id = texture2DPool.AllocId();
    multithreadRendererContext.simulationFrame->resourceCommands.CreateTexture2D(id, info);
    return id;

    #endif
}

void VulkanGPUDevice::UploadTexture2D(Texture2D texture, const void* data, size_t size){
    multithreadRendererContext.simulationFrame->resourceCommands.UploadTexture2D(texture, data, size);
}

void VulkanGPUDevice::DestroyTexture2D(Texture2D tex){
    multithreadRendererContext.simulationFrame->resourceCommands.DestroyTexture2D(tex);
}   

BindGroupLayout VulkanGPUDevice::CreateBindGroupLayout(BindGroupLayoutInfo& info){
    #ifdef DONT_DEFERRED_RESOURCE_CREATION

    BindGroupLayoutData data;
    if(!_CreateBindGroupLayout(data, info)) return InvalidID;
    auto id = bindGroupLayoutPool.AllocId();
    bindGroupLayoutPool.CpuPushResource(id, data);
    return id;

    #else

    auto id = bindGroupLayoutPool.AllocId();
    multithreadRendererContext.simulationFrame->resourceCommands.CreateBindGroupLayout(id, info);
    return id;

    #endif
}

void VulkanGPUDevice::DestroyBindGroupLayout(BindGroupLayout layout){
    multithreadRendererContext.simulationFrame->resourceCommands.DestroyBindGroupLayout(layout);
}

BindGroup VulkanGPUDevice::CreateBindGroup(BindGroupInfo& info){
    #ifdef DONT_DEFERRED_RESOURCE_CREATION

    BindGroupData data;
    if(!_CreateBindGroup(data, info, _descriptorPool)) return InvalidID;
    auto id = bindGroupPool.AllocId();
    bindGroupPool.CpuPushResource(id, data);
    return id;

    #else

    auto id = bindGroupPool.AllocId();
    multithreadRendererContext.simulationFrame->resourceCommands.CreateBindGroup(id, info);
    return id;

    #endif
}

BindGroup VulkanGPUDevice::CreateFrameBindGroup(BindGroupInfo& info){
    auto id = bindGroupPool.AllocId();
    multithreadRendererContext.simulationFrame->resourceCommands.CreateFrameBindGroup(id, info);
    return id;
}

Framebuffer VulkanGPUDevice::CreateFramebuffer(FrameBufferCreateInfo& info){ 
    auto id = framebufferPool.AllocId();
    multithreadRendererContext.simulationFrame->resourceCommands.CreateFramebuffer(id, info);
    return id;
}

void VulkanGPUDevice::DestroyFramebuffer(Framebuffer framebuffer){
    multithreadRendererContext.simulationFrame->resourceCommands.DestroyFramebuffer(framebuffer);
}

#pragma endregion

}
} // namespace OD
