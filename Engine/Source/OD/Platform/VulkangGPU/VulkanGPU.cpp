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

namespace OD {

GraphicsDeviceInfo vkInfo;
GraphicsStats vkGraphicsStats;
GPUMemoryStats vkGPUMemoryStats;
GraphicsDebug vkGraphicsDebug;

GraphicsStats& VulkanGPUDevice::GetStats() { return vkGraphicsStats; }
GPUMemoryStats& VulkanGPUDevice::GetMemoryStats() { return vkGPUMemoryStats; }
GraphicsDebug& VulkanGPUDevice::GetGraphicsDebug() { return vkGraphicsDebug; }
GraphicsDeviceInfo VulkanGPUDevice::GetInfo() { return vkInfo; }

VulkanGPUDevice::VulkanGPUDevice() {
    vkInfo.apiName = "Vulkan";
    vkInfo.version = 1;
    vkInfo.supportUniformBuffer = true;
}

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

VkRenderPass _renderPass;
std::vector<VkFramebuffer> _framebuffers;

std::vector<VkSemaphore> _renderSemaphores;

VkDescriptorPool _descriptorPool;

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

static uint32_t GetVertexLocation(GPUVertexSemantic semantic){
    switch(semantic){
        case GPUVertexSemantic::Position:   return 0;
        case GPUVertexSemantic::Normal:     return 1;
        case GPUVertexSemantic::Tangent:    return 2;
        case GPUVertexSemantic::UV0:        return 3;
        case GPUVertexSemantic::UV1:        return 4;
        case GPUVertexSemantic::UV2:        return 5;
        case GPUVertexSemantic::UV3:        return 6;
        case GPUVertexSemantic::Color0:     return 7;
        case GPUVertexSemantic::Color1:     return 8;
        case GPUVertexSemantic::Weights:    return 9;
        case GPUVertexSemantic::Influences: return 10;
        case GPUVertexSemantic::Custom0:    return 11;
        case GPUVertexSemantic::Custom1:    return 12;
        case GPUVertexSemantic::Custom2:    return 13;
        case GPUVertexSemantic::Custom3:    return 14;
    }
    return 0;
}

VkFormat GetVulkanFormat(GPUVertexFormat format){
    switch(format){
        case GPUVertexFormat::Float:   return VK_FORMAT_R32_SFLOAT;
        case GPUVertexFormat::Float2:  return VK_FORMAT_R32G32_SFLOAT;
        case GPUVertexFormat::Float3:  return VK_FORMAT_R32G32B32_SFLOAT;
        case GPUVertexFormat::Float4:  return VK_FORMAT_R32G32B32A32_SFLOAT;
        case GPUVertexFormat::Int:     return VK_FORMAT_R32_SINT;
        case GPUVertexFormat::Int2:    return VK_FORMAT_R32G32_SINT;
        case GPUVertexFormat::Int3:    return VK_FORMAT_R32G32B32_SINT;
        case GPUVertexFormat::Int4:    return VK_FORMAT_R32G32B32A32_SINT;
        case GPUVertexFormat::UInt:    return VK_FORMAT_R32_UINT;
        case GPUVertexFormat::UInt2:   return VK_FORMAT_R32G32_UINT;
        case GPUVertexFormat::UInt3:   return VK_FORMAT_R32G32B32_UINT;
        case GPUVertexFormat::UInt4:   return VK_FORMAT_R32G32B32A32_UINT;
        default: return VK_FORMAT_R32G32B32A32_SFLOAT;
    }
}

VkBufferUsageFlags GetVulkanBufferUsage(GPUBufferUsage usage){
    switch(usage){
        case GPUBufferUsage::Vertex:  return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        case GPUBufferUsage::Index:   return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        case GPUBufferUsage::Uniform: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        case GPUBufferUsage::Storage: return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
}

VmaMemoryUsage GetVulkanMemoryUsage(GPUBufferMemory memory){
    switch(memory){
        case GPUBufferMemory::GPUOnly:  return VMA_MEMORY_USAGE_GPU_ONLY;
        case GPUBufferMemory::CPUToGPU: return VMA_MEMORY_USAGE_CPU_TO_GPU;
        case GPUBufferMemory::GPUToCPU: return VMA_MEMORY_USAGE_GPU_TO_CPU;
        case GPUBufferMemory::CPUOnly:  return VMA_MEMORY_USAGE_CPU_ONLY;
    }
    return VMA_MEMORY_USAGE_AUTO;
}

VkCullModeFlags GetVulkanCullMode(GPUCullFace cull){
    switch(cull){
        case GPUCullFace::NONE:           return VK_CULL_MODE_NONE;
        case GPUCullFace::BACK:           return VK_CULL_MODE_BACK_BIT;
        case GPUCullFace::FRONT:          return VK_CULL_MODE_FRONT_BIT;
        case GPUCullFace::FRONT_AND_BACK: return VK_CULL_MODE_FRONT_AND_BACK;
    }
    return VK_CULL_MODE_BACK_BIT;
}

VkCompareOp GetVulkanCompareOp(GPUDepthTest depthTest){
    switch(depthTest){
        case GPUDepthTest::DISABLE:       return VK_COMPARE_OP_ALWAYS;
        case GPUDepthTest::LESS:          return VK_COMPARE_OP_LESS;
        case GPUDepthTest::LESS_EQUAL:    return VK_COMPARE_OP_LESS_OR_EQUAL;
        case GPUDepthTest::EQUAL:         return VK_COMPARE_OP_EQUAL;
        case GPUDepthTest::GREATER:       return VK_COMPARE_OP_GREATER;
        case GPUDepthTest::GREATER_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case GPUDepthTest::DIFFERENT:     return VK_COMPARE_OP_NOT_EQUAL;
        case GPUDepthTest::NEVER:         return VK_COMPARE_OP_NEVER;
        case GPUDepthTest::ALWAYS:        return VK_COMPARE_OP_ALWAYS;
    }
    return VK_COMPARE_OP_LESS;
}

VkBlendFactor GetVulkanBlendFactor(GPUBlendMode mode){
    switch(mode){
        case GPUBlendMode::ZERO:                     return VK_BLEND_FACTOR_ZERO;
        case GPUBlendMode::ONE:                      return VK_BLEND_FACTOR_ONE;
        case GPUBlendMode::SRC_COLOR:                return VK_BLEND_FACTOR_SRC_COLOR;
        case GPUBlendMode::ONE_MINUS_SRC_COLOR:      return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case GPUBlendMode::DST_COLOR:                return VK_BLEND_FACTOR_DST_COLOR;
        case GPUBlendMode::ONE_MINUS_DST_COLOR:      return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case GPUBlendMode::SRC_ALPHA:                return VK_BLEND_FACTOR_SRC_ALPHA;
        case GPUBlendMode::ONE_MINUS_SRC_ALPHA:      return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case GPUBlendMode::DST_ALPHA:                return VK_BLEND_FACTOR_DST_ALPHA;
        case GPUBlendMode::ONE_MINUS_DST_ALPHA:      return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case GPUBlendMode::CONSTANT_COLOR:           return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case GPUBlendMode::ONE_MINUS_CONSTANT_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case GPUBlendMode::CONSTANT_ALPHA:           return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case GPUBlendMode::ONE_MINUS_CONSTANT_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
    }
    return VK_BLEND_FACTOR_ONE;
}

VkBlendOp GetVulkanBlendOp(GPUBlendOp op){
    switch(op){
        case GPUBlendOp::FUNC_ADD:              return VK_BLEND_OP_ADD;
        case GPUBlendOp::FUNC_SUBTRACT:         return VK_BLEND_OP_SUBTRACT;
        case GPUBlendOp::FUNC_REVERSE_SUBTRACT: return VK_BLEND_OP_REVERSE_SUBTRACT;
        case GPUBlendOp::MIN:                   return VK_BLEND_OP_MIN;
        case GPUBlendOp::MAX:                   return VK_BLEND_OP_MAX;
    }
    return VK_BLEND_OP_ADD;
}

VkDescriptorType GetVulkanDescriptorType(GPUBindingType type){
    switch(type){
        case GPUBindingType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case GPUBindingType::Texture2D: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    }
    Assert(false);
    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
}

void VulkanGPUDevice::CreateVulkanPipeline(PipelineId id, const char* source, const GPUPipelineInfo& info){
    std::string srcStr = source;
    std::string vertexSource = "#version 450\n#define Vulkan\n#define Vertex\n" + srcStr;
    std::string fragmentSource = "#version 450\n#define Vulkan\n#define Fragment\n" + srcStr;

    VkShaderModule vertModule = VK_NULL_HANDLE;
    VkShaderModule fragModule = VK_NULL_HANDLE;
    if(!load_shader_module(vertexSource.c_str(), VK_SHADER_STAGE_VERTEX_BIT, &vertModule) || !load_shader_module(fragmentSource.c_str(), VK_SHADER_STAGE_FRAGMENT_BIT, &fragModule)) {
        LogError("Failed to build pipeline shaders");
        return;
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
        binding.inputRate = (info.vertexLayout.buffers[i].inputRate == GPUVertexInputRate::Instance)
            ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
        bindings.push_back(binding);
    }

    std::vector<VkVertexInputAttributeDescription> attributes;
    for(uint32_t i = 0; i < info.vertexLayout.attributeCount; ++i){
        const auto& attr = info.vertexLayout.attributes[i];
        VkVertexInputAttributeDescription attrib{};
        attrib.location = GetVertexLocation(attr.semantic);
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
    depthStencil.depthTestEnable = (info.depthTest != GPUDepthTest::DISABLE) ? VK_TRUE : VK_FALSE;
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
    pipelineInfo.renderPass = _renderPass;

    VkPipeline pipeline;
    VK_CHECK(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

    vkDestroyShaderModule(_device, vertModule, nullptr);
    vkDestroyShaderModule(_device, fragModule, nullptr);

    pipelinePool.Get(id).pipeline = pipeline;
    pipelinePool.Get(id).layout = layout;
    pipelinePool.Get(id).info = info;
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
        .use_default_format_selection()
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

void init_default_renderpass(){
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
}

void init_framebuffers(){
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

void init_descriptors(){
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
	pool_info.maxSets = 10;
	pool_info.poolSizeCount = (uint32_t)sizes.size();
	pool_info.pPoolSizes = sizes.data();
	vkCreateDescriptorPool(_device, &pool_info, nullptr, &_descriptorPool);
}

void VulkanGPUDevice::Cleanup(){
    if(!_isInitialized) return;

    glslang::FinalizeProcess();
    vkDeviceWaitIdle(_device);

    mainDeletionQueue.flush();

    vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);

    bufferPool.ForEach([&](uint32_t id, BufferData& data){
        if(data.buffer != VK_NULL_HANDLE){
            vmaDestroyBuffer(_allocator, data.buffer, data.allocation);
            data.buffer = VK_NULL_HANDLE;
        }
    });

    pipelinePool.ForEach([&](uint32_t id, PipelineData& data){
        if(data.pipeline != VK_NULL_HANDLE){
            vkDestroyPipeline(_device, data.pipeline, nullptr);
            vkDestroyPipelineLayout(_device, data.layout, nullptr);
            data.pipeline = VK_NULL_HANDLE;
        }
    });

    bindGroupLayoutPool.ForEach([&](uint32_t id, BindGroupLayoutData& data){ 
        if(data.layout != VK_NULL_HANDLE){
            vkDestroyDescriptorSetLayout(_device, data.layout, nullptr);
            data.layout = VK_NULL_HANDLE;
        }
    });

    texture2DDataPool.ForEach([&](uint32_t id, Texture2DData& data){
        vkDestroySampler(_device, data.sampler, nullptr);
        vkDestroyImageView(_device, data.imageView, nullptr);
        //vkDestroyImage(_device, data.image, nullptr);
        vmaDestroyImage(_allocator, data.image, data.allocation);
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
    init_default_renderpass();
    init_framebuffers();
    init_sync_structures();
    init_descriptors();
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
        multithreadRendererContext.runRender = [&](GPURenderFrame& f){  RunRender(f); f.Clear(); Platform::SwapBuffers(); };
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

GPURenderFrame* VulkanGPUDevice::GetRenderFrame(){
    return multithreadRendererContext.simulationFrame; 
}

void VulkanGPUDevice::RunRender(GPURenderFrame& frame){
    PipelineId currentPipeline = INVALID_ID;

    VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

    // Process Resource Commands first
    for(const GPUResourceCommands::Command& cmd : frame.resourceCommands.commands){
        switch(cmd.type){
            case GPUResourceCommands::Type::CreateBuffer:{
                /*Assert(cmd.createBuffer.id < bufferPool.data.size());

                if(bufferPool.data[cmd.createBuffer.id].buffer != VK_NULL_HANDLE){
                    LogError("Trying CreateBuffer on Used id");
                    continue;
                }

                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = cmd.createBuffer.size;
                bufferInfo.usage = GetVulkanBufferUsage(cmd.createBuffer.usage);

                VmaAllocationCreateInfo allocInfo = {};
                allocInfo.usage = GetVulkanMemoryUsage(cmd.createBuffer.memory);
                if (cmd.createBuffer.memory != GPUBufferMemory::GPUOnly) {
                    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
                }

                VkBuffer buffer;
                VmaAllocation allocation;
                VmaAllocationInfo resultAllocInfo;
                VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &buffer, &allocation, &resultAllocInfo));

                if (cmd.createBuffer.data && cmd.createBuffer.size > 0) {
                    if (cmd.createBuffer.memory != GPUBufferMemory::GPUOnly) {
                        void* mappedData = resultAllocInfo.pMappedData;
                        if (!mappedData) {
                            vmaMapMemory(_allocator, allocation, &mappedData);
                            std::memcpy(mappedData, cmd.createBuffer.data, cmd.createBuffer.size);
                            vmaUnmapMemory(_allocator, allocation);
                        } else {
                            std::memcpy(mappedData, cmd.createBuffer.data, cmd.createBuffer.size);
                        }
                    }
                }

                bufferPool.data[cmd.createBuffer.id].buffer = buffer;
                bufferPool.data[cmd.createBuffer.id].allocation = allocation;
                bufferPool.data[cmd.createBuffer.id].usage = cmd.createBuffer.usage;
                bufferPool.data[cmd.createBuffer.id].memory = cmd.createBuffer.memory;
                bufferPool.data[cmd.createBuffer.id].size = cmd.createBuffer.size;
                break;*/

                //Assert(cmd.createBuffer.id < bufferPool.data.size());

                if(bufferPool.Get(cmd.createBuffer.id).buffer != VK_NULL_HANDLE){
                    LogError("Trying CreateBuffer on Used id");
                    continue;
                }

                // 1. Setup usage flags for the main GPU buffer
                VkBufferCreateInfo bufferInfo = {};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = cmd.createBuffer.size;
                bufferInfo.usage = GetVulkanBufferUsage(cmd.createBuffer.usage);

                // If memory is GPUOnly, we MUST allow it to act as a copy destination!
                if(cmd.createBuffer.memory == GPUBufferMemory::GPUOnly){
                    bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                }

                VmaAllocationCreateInfo allocInfo = {};
                allocInfo.usage = GetVulkanMemoryUsage(cmd.createBuffer.memory);
                if(cmd.createBuffer.memory != GPUBufferMemory::GPUOnly){
                    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
                }

                VkBuffer buffer;
                VmaAllocation allocation;
                VmaAllocationInfo resultAllocInfo;
                VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &buffer, &allocation, &resultAllocInfo));

                // 2. Upload Data
                if(cmd.createBuffer.data && cmd.createBuffer.size > 0){
                    if(cmd.createBuffer.memory != GPUBufferMemory::GPUOnly){
                        // Host visible copy (CPUToGPU / CPUOnly)
                        void* mappedData = resultAllocInfo.pMappedData;
                        bool needUnmap = false;
                        if(!mappedData){
                            vmaMapMemory(_allocator, allocation, &mappedData);
                            needUnmap = true;
                        }

                        std::memcpy(mappedData, cmd.createBuffer.data, cmd.createBuffer.size);
                        vmaFlushAllocation(_allocator, allocation, 0, cmd.createBuffer.size);

                        if(needUnmap){
                            vmaUnmapMemory(_allocator, allocation);
                        }
                    } else {
                        // GPUOnly copy via Staging Buffer
                        VkBufferCreateInfo stagingInfo = {};
                        stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                        stagingInfo.size = cmd.createBuffer.size;
                        stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

                        VmaAllocationCreateInfo stagingAllocInfo = {};
                        stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
                        stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

                        VkBuffer stagingBuffer;
                        VmaAllocation stagingAllocation;
                        VmaAllocationInfo stagingResultInfo;
                        VK_CHECK(vmaCreateBuffer(_allocator, &stagingInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, &stagingResultInfo));

                        // Copy CPU data to staging buffer memory
                        std::memcpy(stagingResultInfo.pMappedData, cmd.createBuffer.data, cmd.createBuffer.size);
                        vmaFlushAllocation(_allocator, stagingAllocation, 0, cmd.createBuffer.size);

                        immediate_submit([&](VkCommandBuffer transferCmd){
                            VkBufferCopy copy;
                            copy.dstOffset = 0;
                            copy.srcOffset = 0;
                            copy.size = cmd.createBuffer.size;
                            vkCmdCopyBuffer(transferCmd, stagingBuffer, buffer, 1, &copy);
                        });
                        vmaDestroyBuffer(_allocator, stagingBuffer, stagingAllocation);

                        // Allocate dynamic single-use command buffer for GPU transfer
                        /*VkCommandBufferAllocateInfo allocCmdInfo = vkinit::command_buffer_allocate_info(_commandPool, 1);
                        VkCommandBuffer transferCmd;
                        VK_CHECK(vkAllocateCommandBuffers(_device, &allocCmdInfo, &transferCmd));

                        VkCommandBufferBeginInfo beginInfo = {};
                        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                        VK_CHECK(vkBeginCommandBuffer(transferCmd, &beginInfo));

                        VkBufferCopy copyRegion = {};
                        copyRegion.srcOffset = 0;
                        copyRegion.dstOffset = 0;
                        copyRegion.size = cmd.createBuffer.size;
                        vkCmdCopyBuffer(transferCmd, stagingBuffer, buffer, 1, &copyRegion);

                        VK_CHECK(vkEndCommandBuffer(transferCmd));

                        // Execute copy operation on GPU Queue immediately
                        VkSubmitInfo submitInfo = {};
                        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                        submitInfo.commandBufferCount = 1;
                        submitInfo.pCommandBuffers = &transferCmd;

                        VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
                        VK_CHECK(vkQueueWaitIdle(_graphicsQueue)); // Synchronize transfer completion

                        // Cleanup staging resources
                        vkFreeCommandBuffers(_device, _commandPool, 1, &transferCmd);
                        vmaDestroyBuffer(_allocator, stagingBuffer, stagingAllocation);*/
                    }
                }

                bufferPool.Get(cmd.createBuffer.id).buffer = buffer;
                bufferPool.Get(cmd.createBuffer.id).allocation = allocation;
                bufferPool.Get(cmd.createBuffer.id).usage = cmd.createBuffer.usage;
                bufferPool.Get(cmd.createBuffer.id).memory = cmd.createBuffer.memory;
                bufferPool.Get(cmd.createBuffer.id).size = cmd.createBuffer.size;

                //LogInfo("CreateBuffer");
                break;
            }

            case GPUResourceCommands::Type::DestroyBuffer:{
                auto& bufData = bufferPool.Get(cmd.destroyBuffer.id);
                if(bufData.buffer != VK_NULL_HANDLE){
                    vmaDestroyBuffer(_allocator, bufData.buffer, bufData.allocation);
                    bufData.buffer = VK_NULL_HANDLE;
                    bufData.allocation = VK_NULL_HANDLE;
                }
                bufferPool.idsDestred.push_back(cmd.destroyBuffer.id);
                break;
            }

            case GPUResourceCommands::Type::CreateTexture2D:{
                if(texture2DDataPool.Get(cmd.createTexture2D.id).image != VK_NULL_HANDLE){
                    LogError("Trying CreateTexture2D on Used id");
                    continue;
                }

                Texture2DData& texData = texture2DDataPool.Get(cmd.createTexture2D.id);

                VkDeviceSize imageSize = cmd.createTexture2D.size;// texWidth * texHeight * 4;

                //the format R8G8B8A8 matches exactly with the pixels loaded from stb_image lib
                VkFormat image_format = VK_FORMAT_R8G8B8A8_SRGB;

                //allocate temporary buffer for holding texture data to upload
                AllocatedBuffer stagingBuffer = create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

                //copy data to buffer
                void* data;
                vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
                memcpy(data, cmd.createTexture2D.data, cmd.createTexture2D.size);
                vmaUnmapMemory(_allocator, stagingBuffer._allocation);

                VkExtent3D imageExtent;
                imageExtent.width = cmd.createTexture2D.info.width;
                imageExtent.height = cmd.createTexture2D.info.height;
                imageExtent.depth = 1;

                VkImageCreateInfo dimg_info = vkinit::image_create_info(image_format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);

                VmaAllocationCreateInfo dimg_allocinfo = {};
                dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

                //allocate and create the image
                VK_CHECK(vmaCreateImage(_allocator, &dimg_info, &dimg_allocinfo, &texData.image, &texData.allocation, nullptr));

                immediate_submit([&](VkCommandBuffer cmd) {
                    VkImageSubresourceRange range;
                    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    range.baseMipLevel = 0;
                    range.levelCount = 1;
                    range.baseArrayLayer = 0;
                    range.layerCount = 1;

                    VkImageMemoryBarrier imageBarrier_toTransfer = {};
                    imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    imageBarrier_toTransfer.image = texData.image;
                    imageBarrier_toTransfer.subresourceRange = range;
                    imageBarrier_toTransfer.srcAccessMask = 0;
                    imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    //barrier the image into the transfer-receive layout
                    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

                    VkBufferImageCopy copyRegion = {};
                    copyRegion.bufferOffset = 0;
                    copyRegion.bufferRowLength = 0;
                    copyRegion.bufferImageHeight = 0;
                    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    copyRegion.imageSubresource.mipLevel = 0;
                    copyRegion.imageSubresource.baseArrayLayer = 0;
                    copyRegion.imageSubresource.layerCount = 1;
                    copyRegion.imageExtent = imageExtent;
                    //copy the buffer into the image
                    vkCmdCopyBufferToImage(cmd, stagingBuffer._buffer, texData.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

                    VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
                    imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    //barrier the image into the shader readable layout
                    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);
                });
                vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);

                VkImageViewCreateInfo imageinfo = vkinit::imageview_create_info(VK_FORMAT_R8G8B8A8_SRGB, texData.image, VK_IMAGE_ASPECT_COLOR_BIT);
	            VK_CHECK(vkCreateImageView(_device, &imageinfo, nullptr, &texData.imageView));

                VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_LINEAR);// VK_FILTER_NEAREST);
                VK_CHECK(vkCreateSampler(_device, &samplerInfo, nullptr, &texData.sampler));

                break;
            }

            case GPUResourceCommands::Type::CreatePipeline:{
                CreateVulkanPipeline(cmd.createPipeline.id, cmd.createPipeline.source, cmd.createPipeline.info);
                //LogInfo("CreatePipeline");
                break;
            }

            case GPUResourceCommands::Type::DestroyPipeline:{
                auto& pipeData = pipelinePool.Get(cmd.destroyPipeline.id);
                if(pipeData.pipeline != VK_NULL_HANDLE){
                    vkDestroyPipeline(_device, pipeData.pipeline, nullptr);
                    vkDestroyPipelineLayout(_device, pipeData.layout, nullptr);
                    pipeData.pipeline = VK_NULL_HANDLE;
                    pipeData.layout = VK_NULL_HANDLE;
                }
                pipelinePool.idsDestred.push_back(cmd.destroyPipeline.id);
                break;
            }
            
            case GPUResourceCommands::Type::CreateBindGroupLayout:{
                BindGroupLayoutData& data = bindGroupLayoutPool.Get(cmd.createBindGroupLayout.id);
                std::memcpy(&data.info, cmd.createBindGroupLayout.info, sizeof(GPUBindGroupLayoutInfo));
                //data.info = cmd.createBindGroupLayout.info;

                auto Convert = [](GPUBindLayoutEntry& e) -> VkDescriptorSetLayoutBinding{
                    VkDescriptorSetLayoutBinding entry = {};
                    entry.binding = e.binding;
                    entry.descriptorCount = 1;
                    entry.descriptorType = GetVulkanDescriptorType(e.type);// VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    entry.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                    return entry;
                };

                std::vector<VkDescriptorSetLayoutBinding> entries;
                for(int i = 0; i < cmd.createBindGroupLayout.info->entriesCount; i++){
                    auto& e = cmd.createBindGroupLayout.info->entries[i];
                    
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
                break;
            }

            case GPUResourceCommands::Type::CreateBindGroup:{
                BindGroupLayoutData& layoutData = bindGroupLayoutPool.Get(cmd.createBindGroup.info->layout);
    
                VkDescriptorSetAllocateInfo allocInfo ={};
                allocInfo.pNext = nullptr;
                allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocInfo.descriptorPool = _descriptorPool;
                allocInfo.descriptorSetCount = 1;
                allocInfo.pSetLayouts = &layoutData.layout;
                VK_CHECK(vkAllocateDescriptorSets(_device, &allocInfo, &bindGroupPool.Get(cmd.createBindGroup.id).descriptorSet));

                std::vector<VkDescriptorBufferInfo> bInfos;
                std::vector<VkDescriptorImageInfo> imageInfos;
                std::vector<VkWriteDescriptorSet> writes;
                bInfos.resize(cmd.createBindGroup.info->entriesCount);
                imageInfos.resize(cmd.createBindGroup.info->entriesCount);
                writes.resize(cmd.createBindGroup.info->entriesCount);

                for(int i = 0; i < cmd.createBindGroup.info->entriesCount; i++){
                    if(layoutData.info.entries[i].type == GPUBindingType::UniformBuffer){
                        BufferData& bufferData = bufferPool.Get(cmd.createBindGroup.info->entries[i].buffer);

                        VkDescriptorBufferInfo& binfo = bInfos[i];
                        binfo = {};
                        binfo.buffer = bufferData.buffer;// _frames[i].cameraBuffer._buffer;
                        binfo.offset = cmd.createBindGroup.info->entries[i].offset;
                        binfo.range = cmd.createBindGroup.info->entries[i].size;// ssizeof(GPUCameraData);

                        VkWriteDescriptorSet& setWrite = writes[i];
                        setWrite = {};
                        setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        setWrite.pNext = nullptr;
                        setWrite.dstBinding = cmd.createBindGroup.info->entries[i].binding;
                        setWrite.dstSet = bindGroupPool.Get(cmd.createBindGroup.id).descriptorSet;
                        setWrite.descriptorCount = 1;
                        setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        setWrite.pBufferInfo = &binfo;
                    } else if(layoutData.info.entries[i].type == GPUBindingType::Texture2D){
                        Texture2DData& texData = texture2DDataPool.Get(cmd.createBindGroup.info->entries[i].texture);

                        VkDescriptorImageInfo& imageBufferInfo = imageInfos[i];
                        imageBufferInfo.sampler = texData.sampler;
                        imageBufferInfo.imageView = texData.imageView;
                        imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        VkWriteDescriptorSet& setWrite = writes[i];
                        setWrite = vkinit::write_descriptor_image(
                            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
                            bindGroupPool.Get(cmd.createBindGroup.id).descriptorSet, 
                            &imageBufferInfo, 
                            cmd.createBindGroup.info->entries[i].binding
                        );
                    } else {
                        Assert(false);
                    }
                }

                vkUpdateDescriptorSets(_device, writes.size(), writes.data(), 0, nullptr);
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
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    for(const GPUCommandBuffer::Command& renderCmd : frame.renderCommands.commands){
        switch(renderCmd.type){
            case GPUCommandBuffer::Type::Clear:{
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

            case GPUCommandBuffer::Type::Viewport:{
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
                break;
            }

            case GPUCommandBuffer::Type::SetPipeline:{
                const auto& pipeData = pipelinePool.Get(renderCmd.setPipeline.id);
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeData.pipeline);
                currentPipeline = renderCmd.setPipeline.id;
                break;
            }

            case GPUCommandBuffer::Type::SetVertexBuffer:{
                const auto& bufData = bufferPool.Get(renderCmd.setVertexBuffer.buffer);
                Assert(bufData.buffer != VK_NULL_HANDLE);
                Assert(renderCmd.setVertexBuffer.buffer != InvalidID);
                //Assert(renderCmd.setVertexBuffer.buffer < bufferPool.data.size());
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(cmd, renderCmd.setVertexBuffer.slot, 1, &bufData.buffer, offsets);
                break;
            }

            case GPUCommandBuffer::Type::SetIndexBuffer:{
                const auto& bufData = bufferPool.Get(renderCmd.setIndexBuffer.buffer);
                vkCmdBindIndexBuffer(cmd, bufData.buffer, 0, VK_INDEX_TYPE_UINT32);
                break;
            }

            case GPUCommandBuffer::Type::SetBindGroup:{
                const BindGroupData& bindGroupData = bindGroupPool.Get(renderCmd.setBindGroup.group);
                const PipelineData& pipelineData = pipelinePool.Get(currentPipeline);

                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.layout, 0, 1, &bindGroupData.descriptorSet, 0, nullptr);
                break;
            };

            case GPUCommandBuffer::Type::Draw:{
                vkCmdDraw(cmd, renderCmd.draw.vertexCount, 1, 0, 0);
                break;
            }

            case GPUCommandBuffer::Type::DrawIndexed:{
                vkCmdDrawIndexed(cmd, renderCmd.drawIndexed.indexCount, 1, 0, 0, 0);
                break;
            }
        }
    }

    vkCmdEndRenderPass(cmd);
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
    pipelinePool.SyncSingleThreadData();

    bindGroupLayoutPool.SyncSingleThreadData();
    bindGroupPool.SyncSingleThreadData();
}

BufferId VulkanGPUDevice::AllocBufferId(){
    return bufferPool.AllocId();
}

PipelineId VulkanGPUDevice::AllocPipelineId(){
    return pipelinePool.AllocId();
}

BindGroupLayoutId VulkanGPUDevice::AllocCreateBindGroupLayoutId(){
    return bindGroupLayoutPool.AllocId();
}

BindGroupId VulkanGPUDevice::AllocCreateBindGroupId(){
    return bindGroupPool.AllocId();
}

Texture2DId VulkanGPUDevice::AllocTexture2DId(){ 
    return texture2DDataPool.AllocId(); 
}

BindGroupLayoutId VulkanGPUDevice::CreateBindGroupLayout(GPUBindGroupLayoutInfo& info){
    auto id = bindGroupLayoutPool.AllocId();
    BindGroupLayoutData& data = bindGroupLayoutPool.Get(id);

    //data.info = cmd.createBindGroupLayout.info;

    auto Convert = [](GPUBindLayoutEntry& e) -> VkDescriptorSetLayoutBinding{
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
    return id;
}

BindGroupId VulkanGPUDevice::CreateBindGroup(GPUBindGroupInfo& info){
    auto id = bindGroupPool.AllocId();
    BindGroupData& data = bindGroupPool.Get(id);

    VkDescriptorSetLayout layout = bindGroupLayoutPool.Get(info.layout).layout;
    
    VkDescriptorSetAllocateInfo allocInfo ={};
    allocInfo.pNext = nullptr;
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;
    VK_CHECK(vkAllocateDescriptorSets(_device, &allocInfo, &bindGroupPool.Get(id).descriptorSet));

    std::vector<VkDescriptorBufferInfo> bInfos;
    std::vector<VkWriteDescriptorSet> writes;
    bInfos.resize(info.entriesCount);
    writes.resize(info.entriesCount);

    for(int i = 0; i < info.entriesCount; i++){
        if(bindGroupLayoutPool.Get(info.layout).info.entries[i].type == GPUBindingType::UniformBuffer){
            BufferData& bufferData = bufferPool.Get(info.entries[i].buffer);

            VkDescriptorBufferInfo& binfo = bInfos[i];
            binfo = {};
            binfo.buffer = bufferData.buffer;// _frames[i].cameraBuffer._buffer;
            binfo.offset = info.entries[i].offset;
            binfo.range = info.entries[i].size;// ssizeof(GPUCameraData);

            VkWriteDescriptorSet setWrite = writes[i];
            setWrite = {};
            setWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            setWrite.pNext = nullptr;
            setWrite.dstBinding = info.entries[i].binding;
            setWrite.dstSet = bindGroupPool.Get(id).descriptorSet;
            setWrite.descriptorCount = 1;
            setWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            setWrite.pBufferInfo = &binfo;
        } else {
            Assert(false);
        }
    }

    vkUpdateDescriptorSets(_device, writes.size(), writes.data(), 0, nullptr);
    return id;
}

} // namespace OD