#include "VulkanGPU.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Core/Application.h"
#include "OD/Platform/Platform.h"

#include <vulkan/vulkan.h>
#include <vk-bootstrap/VkBootstrap.h>
#include "vk_initializers.h"

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>

#include <string>
#include <vector>
#include <stdexcept>

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

GraphicsDeviceInfo vkInfo;

GraphicsStats vkGraphicsStats;
GPUMemoryStats vkGPUMemoryStats;
GraphicsDebug vkGraphicsDebug;

GraphicsStats& VulkanGPUDevice::GetStats(){ return vkGraphicsStats; }
GPUMemoryStats& VulkanGPUDevice::GetMemoryStats(){ return vkGPUMemoryStats; }
GraphicsDebug& VulkanGPUDevice::GetGraphicsDebug(){ return vkGraphicsDebug; }
GraphicsDeviceInfo VulkanGPUDevice::GetInfo(){ return vkInfo; }

VulkanGPUDevice::VulkanGPUDevice(){
    vkInfo.apiName = "Vulkan";
    vkInfo.version = 1;
    vkInfo.supportUniformBuffer = true;
}

////////////////////////////////////
const char* vkShaderSource = R"GLSL(
    #ifdef Vertex
    void main(){
        const vec3 positions[3] = vec3[3](
            vec3(1.0f, 1.0f, 0.0f),
            vec3(-1.0f, 1.0f, 0.0f),
            vec3(0.0f, -1.0f, 0.0f)
        );
        gl_Position = vec4(positions[gl_VertexIndex], 1.0f);
    }
    #endif

    #ifdef Fragment
    layout(location = 0) out vec4 outFragColor;
    void main(){
        outFragColor = vec4(1.f,0.f,0.f,1.0f);
    }
    #endif
    )GLSL";

///////////////////////////////////////////

bool _isInitialized{ false };
int _frameNumber {0};

VkExtent2D _windowExtent{ 800, 600 };

VkInstance _instance; // Vulkan library handle
VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
VkPhysicalDevice _chosenGPU; // GPU chosen as the default device
VkDevice _device; // Vulkan device for commands
VkSurfaceKHR _surface; // Vulkan window surface

// --- other code ---
VkSwapchainKHR _swapchain; // from other articles

// image format expected by the windowing system
VkFormat _swapchainImageFormat;

//array of images from the swapchain
std::vector<VkImage> _swapchainImages;

//array of image-views from the swapchain
std::vector<VkImageView> _swapchainImageViews;

// ---- other code -----
VkQueue _graphicsQueue; //queue we will submit to
uint32_t _graphicsQueueFamily; //family of that queue

VkCommandPool _commandPool; //the command pool for our commands
VkCommandBuffer _mainCommandBuffer; //the buffer we will record into

VkRenderPass _renderPass;
std::vector<VkFramebuffer> _framebuffers;

VkSemaphore _presentSemaphore, _renderSemaphore;
VkFence _renderFence;

VkPipelineLayout _trianglePipelineLayout;
VkPipeline _trianglePipeline;

////////////////////////////////////

static EShLanguage ToGlslangStage(VkShaderStageFlagBits stage){
    switch (stage){
        case VK_SHADER_STAGE_VERTEX_BIT:
            return EShLangVertex;

        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return EShLangFragment;

        case VK_SHADER_STAGE_COMPUTE_BIT:
            return EShLangCompute;

        case VK_SHADER_STAGE_GEOMETRY_BIT:
            return EShLangGeometry;

        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            return EShLangTessControl;

        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            return EShLangTessEvaluation;

        default:
            throw std::runtime_error("Unsupported Vulkan shader stage");
    }
}

std::vector<uint32_t> CompileGLSL(const std::string& source, VkShaderStageFlagBits stage){
    static bool initialized = false;
    if(!initialized){
        
        initialized = true;
    }

    const EShLanguage shaderStage = ToGlslangStage(stage);

    const char* sourceString = source.c_str();

    glslang::TShader shader(shaderStage);

    shader.setStrings(&sourceString, 1);

    // GLSL -> Vulkan
    shader.setEnvInput(
        glslang::EShSourceGlsl,
        shaderStage,
        glslang::EShClientVulkan,
        100
    );

    shader.setEnvClient(
        glslang::EShClientVulkan,
        glslang::EShTargetVulkan_1_1
    );

    shader.setEnvTarget(
        glslang::EShTargetSpv,
        glslang::EShTargetSpv_1_3
    );

    // Default resource limits.
    const TBuiltInResource* resources = GetDefaultResources();// {}; //DefaultTBuiltInResource;

    EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    if(!shader.parse(resources, 450, false, messages)){
        std::string error = "GLSL compilation failed:\n";
        error += shader.getInfoLog();
        error += "\n";
        error += shader.getInfoDebugLog();
        LogInfo("Error: {}", error);
        throw std::runtime_error(error);
    }

    glslang::TProgram program;

    program.addShader(&shader);

    if(!program.link(messages)){
        std::string error = "GLSL linking failed:\n";
        error += program.getInfoLog();
        error += "\n";
        error += program.getInfoDebugLog();
        LogInfo("Error: {}", error);
        throw std::runtime_error(error);
    }

    std::vector<uint32_t> spirv;

    spv::SpvBuildLogger logger;

    glslang::SpvOptions options;
    options.generateDebugInfo = false;
    options.disableOptimizer = false;
    options.optimizeSize = false;

    glslang::GlslangToSpv(
        *program.getIntermediate(shaderStage),
        spirv,
        &logger,
        &options
    );

    if(!logger.getAllMessages().empty()){
        // Optional: print SPIR-V generation warnings.
        // printf("%s\n", logger.getAllMessages().c_str());
    }

    return spirv;
}

bool load_shader_module(const char* filePath, VkShaderStageFlagBits stage, VkShaderModule* outShaderModule){
    std::vector<uint32_t> spirv = CompileGLSL(filePath, stage);

    //create a new shader module, using the buffer we loaded
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;

    //codeSize has to be in bytes, so multiply the ints in the buffer by size of int to know the real size of the buffer
    createInfo.codeSize = spirv.size() * sizeof(uint32_t);
    createInfo.pCode = spirv.data();

    //check that the creation goes well.
    VkShaderModule shaderModule;
    if(vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) return false;

    *outShaderModule = shaderModule;
    return true;
}

//////////////////////////////////

class PipelineBuilder {
public:
	std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
	VkViewport _viewport;
	VkRect2D _scissor;
	VkPipelineRasterizationStateCreateInfo _rasterizer;
	VkPipelineColorBlendAttachmentState _colorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo _multisampling;
	VkPipelineLayout _pipelineLayout;

	VkPipeline build_pipeline(VkDevice device, VkRenderPass pass){
        //make viewport state from our stored viewport and scissor.
        //at the moment we won't support multiple viewports or scissors
        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.pNext = nullptr;

        viewportState.viewportCount = 1;
        viewportState.pViewports = &_viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &_scissor;

        //setup dummy color blending. We aren't using transparent objects yet
        //the blending is just "no blend", but we do write to the color attachment
        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.pNext = nullptr;

        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &_colorBlendAttachment;

        //build the actual pipeline
        //we now use all of the info structs we have been writing into into this one to create the pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = nullptr;

        pipelineInfo.stageCount = _shaderStages.size();
        pipelineInfo.pStages = _shaderStages.data();
        pipelineInfo.pVertexInputState = &_vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &_inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &_rasterizer;
        pipelineInfo.pMultisampleState = &_multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = _pipelineLayout;
        pipelineInfo.renderPass = pass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        //it's easy to error out on create graphics pipeline, so we handle it a bit better than the common VK_CHECK case
        VkPipeline newPipeline;
        if (vkCreateGraphicsPipelines(
            device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
            std::cout << "failed to create pipeline\n";
            return VK_NULL_HANDLE; // failed to create graphics pipeline
        }
        else
        {
            return newPipeline;
        }
    }
};

///////////////////////////////////

void init_vulkan(){
    vkb::InstanceBuilder builder;

	//make the Vulkan instance, with basic debug features
	auto inst_ret = builder.set_app_name("Example Vulkan Application")
		.request_validation_layers(true)
		.require_api_version(1, 1, 0)
		.use_default_debug_messenger()
		.build();

	vkb::Instance vkb_inst = inst_ret.value();

	//store the instance
	_instance = vkb_inst.instance;
	//store the debug messenger
	_debug_messenger = vkb_inst.debug_messenger;

    // get the surface of the window we opened with SDL
	//SDL_Vulkan_CreateSurface(_window, _instance, &_surface);
    Platform::CreateVulkanSurface(_instance, &_surface);

	//use vkbootstrap to select a GPU.
	//We want a GPU that can write to the SDL surface and supports Vulkan 1.1
	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 1)
		.set_surface(_surface)
		.select()
		.value();

	//create the final Vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();

	// Get the VkDevice handle used in the rest of a Vulkan application
	_device = vkbDevice.device;
	_chosenGPU = physicalDevice.physical_device;

    // use vkbootstrap to get a Graphics queue
	_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
}

void init_swapchain(){
    vkb::SwapchainBuilder swapchainBuilder{_chosenGPU,_device,_surface };

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.use_default_format_selection()
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(_windowExtent.width, _windowExtent.height)
		.build()
		.value();

	//store swapchain and its related images
	_swapchain = vkbSwapchain.swapchain;
	_swapchainImages = vkbSwapchain.get_images().value();
	_swapchainImageViews = vkbSwapchain.get_image_views().value();

	_swapchainImageFormat = vkbSwapchain.image_format;
}

void init_commands(){
    //create a command pool for commands submitted to the graphics queue.
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_commandPool));

    //allocate the default command buffer that we will use for rendering
	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_commandPool, 1);
	VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_mainCommandBuffer));
}

void init_default_renderpass(){
    // the renderpass will use this color attachment.
	VkAttachmentDescription color_attachment = {};
	//the attachment will have the format needed by the swapchain
	color_attachment.format = _swapchainImageFormat;
	//1 sample, we won't be doing MSAA
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// we Clear when this attachment is loaded
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// we keep the attachment stored when the renderpass ends
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	//we don't care about stencil
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	//we don't know or care about the starting layout of the attachment
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	//after the renderpass ends, the image has to be on a layout ready for display
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
	//attachment number will index into the pAttachments array in the parent renderpass itself
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//we are going to create 1 subpass, which is the minimum you can do
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

    VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	//connect the color attachment to the info
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &color_attachment;
	//connect the subpass to the info
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;

	VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_renderPass));
}

void init_framebuffers(){
    //create the framebuffers for the swapchain images. This will connect the render-pass to the images for rendering
	VkFramebufferCreateInfo fb_info = {};
	fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_info.pNext = nullptr;

	fb_info.renderPass = _renderPass;
	fb_info.attachmentCount = 1;
	fb_info.width = _windowExtent.width;
	fb_info.height = _windowExtent.height;
	fb_info.layers = 1;

	//grab how many images we have in the swapchain
	const uint32_t swapchain_imagecount = _swapchainImages.size();
	_framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

	//create framebuffers for each of the swapchain image views
	for(int i = 0; i < swapchain_imagecount; i++){
		fb_info.pAttachments = &_swapchainImageViews[i];
		VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr, &_framebuffers[i]));
	}
}

void init_sync_structures(){
    //create synchronization structures

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;

	//we want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU command (for the first frame)
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_renderFence));

	//for the semaphores we don't need any flags
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_presentSemaphore));
	VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_renderSemaphore));
}

void init_pipelines(){
    std::string source = std::string(vkShaderSource);
    std::string vertexSource =
        "#version 450\n"
        "#define Vertex\n" +
        source;

    std::string fragmentSource =
        "#version 450\n"
        "#define Fragment\n" +
        source;

    VkShaderModule triangleFragShader;
	if(!load_shader_module(fragmentSource.c_str(), VK_SHADER_STAGE_FRAGMENT_BIT, &triangleFragShader)){
		Assert(false && "Error when building the triangle fragment shader module");
	}

	VkShaderModule triangleVertexShader;
	if(!load_shader_module(vertexSource.c_str(), VK_SHADER_STAGE_VERTEX_BIT, &triangleVertexShader)){
		Assert(false && "Error when building the triangle vertex shader module");
	}

    //build the pipeline layout that controls the inputs/outputs of the shader
	//we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
	VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();

	VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_trianglePipelineLayout));

    //build the stage-create-info for both vertex and fragment stages. This lets the pipeline know the shader modules per stage
	PipelineBuilder pipelineBuilder;
	pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, triangleVertexShader));
	pipelineBuilder._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));
	//vertex input controls how to read vertices from vertex buffers. We aren't using it yet
	pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
	//input assembly is the configuration for drawing triangle lists, strips, or individual points.
	//we are just going to draw triangle list
	pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	//build viewport and scissor from the swapchain extents
	pipelineBuilder._viewport.x = 0.0f;
	pipelineBuilder._viewport.y = 0.0f;
	pipelineBuilder._viewport.width = (float)_windowExtent.width;
	pipelineBuilder._viewport.height = (float)_windowExtent.height;
	pipelineBuilder._viewport.minDepth = 0.0f;
	pipelineBuilder._viewport.maxDepth = 1.0f;
	pipelineBuilder._scissor.offset = { 0, 0 };
	pipelineBuilder._scissor.extent = _windowExtent;
	//configure the rasterizer to draw filled triangles
	pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
	//we don't use multisampling, so just run the default one
	pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();
	//a single blend attachment with no blending and writing to RGBA
	pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();
	//use the triangle layout we created
	pipelineBuilder._pipelineLayout = _trianglePipelineLayout;

	//finally build the pipeline
	_trianglePipeline = pipelineBuilder.build_pipeline(_device, _renderPass);
}

void cleanup(){
    if(!_isInitialized) return;

    glslang::FinalizeProcess();

    vkDeviceWaitIdle(_device);

    vkDestroyCommandPool(_device, _commandPool, nullptr);

    //destroy sync objects
    vkDestroyFence(_device, _renderFence, nullptr);
    vkDestroySemaphore(_device, _renderSemaphore, nullptr);
    vkDestroySemaphore(_device, _presentSemaphore, nullptr);

    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    vkDestroyRenderPass(_device, _renderPass, nullptr);

    //destroy swapchain resources
    for(int i = 0; i < _framebuffers.size(); i++){
        vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);

        vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
    }

    vkDestroySurfaceKHR(_instance, _surface, nullptr);

    vkDestroyDevice(_device, nullptr);
    vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
    vkDestroyInstance(_instance, nullptr);
}

///////////////////////////////////

void VulkanGPUDevice::Init(){
    LogInfo("VulkanGPUDevice::Initialize");

    _windowExtent.width = Application::ScreenWidth();
    _windowExtent.height = Application::ScreenHeight();

    init_vulkan();
    init_swapchain();
    init_commands();
    init_default_renderpass();
	init_framebuffers();
    init_sync_structures();
    init_pipelines();
    _isInitialized = true;
}

void VulkanGPUDevice::Shut(){
    LogInfo("VulkanGPUDevice::Shut");

    cleanup();
}

void VulkanGPUDevice::RunRender(GPURenderFrame& frame){
    //return;
    VK_CHECK(vkWaitForFences(_device, 1, &_renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(_device, 1, &_renderFence));

    uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, _presentSemaphore, nullptr, &swapchainImageIndex));

    //now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
	VK_CHECK(vkResetCommandBuffer(_mainCommandBuffer, 0));

    //naming it cmd for shorter writing
	VkCommandBuffer cmd = _mainCommandBuffer;

	//begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan know that
	VkCommandBufferBeginInfo cmdBeginInfo = {};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.pNext = nullptr;
	cmdBeginInfo.pInheritanceInfo = nullptr;
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    //make a clear-color from frame number. This will flash with a 120*pi frame period.
	VkClearValue clearValue;
	float flash = abs(sin(_frameNumber / 120.f));
	clearValue.color = { { 0.0f, 0.0f, flash, 1.0f } };

	//start the main renderpass.
	//We will use the clear color from above, and the framebuffer of the index the swapchain gave us
	VkRenderPassBeginInfo rpInfo = {};
	rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpInfo.pNext = nullptr;
	rpInfo.renderPass = _renderPass;
	rpInfo.renderArea.offset.x = 0;
	rpInfo.renderArea.offset.y = 0;
	rpInfo.renderArea.extent = _windowExtent;
	rpInfo.framebuffer = _framebuffers[swapchainImageIndex];
	//connect clear values
	rpInfo.clearValueCount = 1;
	rpInfo.pClearValues = &clearValue;

	vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _trianglePipeline);
    vkCmdDraw(cmd, 3, 1, 0, 0);

    //finalize the render pass
	vkCmdEndRenderPass(cmd);
	//finalize the command buffer (we can no longer add commands, but it can now be executed)
	VK_CHECK(vkEndCommandBuffer(cmd));

    //prepare the submission to the queue.
	//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
	//we will signal the _renderSemaphore, to signal that rendering has finished

	VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit.pWaitDstStageMask = &waitStage;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &_presentSemaphore;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &_renderSemaphore;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmd;

	//submit command buffer to the queue and execute it.
	// _renderFence will now block until the graphic commands finish execution
	VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, _renderFence));

    // this will put the image we just rendered into the visible window.
	// we want to wait on the _renderSemaphore for that,
	// as it's necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &_swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pWaitSemaphores = &_renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pImageIndices = &swapchainImageIndex;

	VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

	//increase the number of frames drawn
	_frameNumber++;

    for(const GPUCommandBuffer::Command& cmd : frame.commands.commands){
        switch(cmd.type){
        }
    }
}

void VulkanGPUDevice::SyncSingleThreadData(){

}

///////////////////////////////////

MeshId VulkanGPUDevice::AllocMeshId(){
    return 0;
}

PipelineId VulkanGPUDevice::AllocPipelineId(){
    return 0;
}

}