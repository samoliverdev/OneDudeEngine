#ifdef WEBGPU_SUPPORT
#include "WebGPUGraphicsDevice.h"
#include "OD/Platform/Platform.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Camera.h"
#include "OD/Graphics/Framebuffer.h"
#include "OD/Graphics/Font.h"
#include "OD/Graphics/Mesh.h"
#include "OD/Graphics/Model.h"
#include "OD/Graphics/SubShader.h"
#include "OD/Graphics/Shader.h"
#include "OD/Graphics/Material.h"
#include "OD/Graphics/Font.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Serialization/SerializationFull.h"
#include "OD/Core/Application.h"
#include "OD/Core/ImGui.h"
#include "OD/Platform/Spirv.h"

#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>

#include "glslang/Include/glslang_c_interface.h"
#include "glslang/Public/resource_limits_c.h"
//#include <spirv_cross_c.h>

#include <imgui/backends/imgui_impl_wgpu.h>

namespace OD{

namespace Ultis{
    WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const * options) {
        // A simple structure holding the local information shared with the
        // onAdapterRequestEnded callback.
        struct UserData {
            WGPUAdapter adapter = nullptr;
            bool requestEnded = false;
        };
        UserData userData;
    
        // Callback called by wgpuInstanceRequestAdapter when the request returns
        // This is a C++ lambda function, but could be any function defined in the
        // global scope. It must be non-capturing (the brackets [] are empty) so
        // that it behaves like a regular C function pointer, which is what
        // wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
        // is to convey what we want to capture through the pUserData pointer,
        // provided as the last argument of wgpuInstanceRequestAdapter and received
        // by the callback as its last argument.
        auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * pUserData) {
            UserData& userData = *reinterpret_cast<UserData*>(pUserData);
            if (status == WGPURequestAdapterStatus_Success) {
                userData.adapter = adapter;
            } else {
                std::cout << "Could not get WebGPU adapter: " << message << std::endl;
            }
            userData.requestEnded = true;
        };
    
        // Call to the WebGPU request adapter procedure
        wgpuInstanceRequestAdapter(
            instance /* equivalent of navigator.gpu */,
            options,
            onAdapterRequestEnded,
            (void*)&userData
        );
    
        // We wait until userData.requestEnded gets true
    #ifdef __EMSCRIPTEN__
        while (!userData.requestEnded) {
            emscripten_sleep(100);
        }
    #endif // __EMSCRIPTEN__
    
        assert(userData.requestEnded);
    
        return userData.adapter;
    }
    
    void inspectAdapter(WGPUAdapter adapter) {
    #ifndef __EMSCRIPTEN__
        WGPUSupportedLimits supportedLimits = {};
        supportedLimits.nextInChain = nullptr;
    
    #ifdef WEBGPU_BACKEND_DAWN
        bool success = wgpuAdapterGetLimits(adapter, &supportedLimits) == WGPUStatus_Success;
    #else
        bool success = wgpuAdapterGetLimits(adapter, &supportedLimits);
    #endif
    
        if (success) {
            std::cout << "Adapter limits:" << std::endl;
            std::cout << " - maxTextureDimension1D: " << supportedLimits.limits.maxTextureDimension1D << std::endl;
            std::cout << " - maxTextureDimension2D: " << supportedLimits.limits.maxTextureDimension2D << std::endl;
            std::cout << " - maxTextureDimension3D: " << supportedLimits.limits.maxTextureDimension3D << std::endl;
            std::cout << " - maxTextureArrayLayers: " << supportedLimits.limits.maxTextureArrayLayers << std::endl;
        }
    #endif // NOT __EMSCRIPTEN__
        std::vector<WGPUFeatureName> features;
    
        // Call the function a first time with a null return address, just to get
        // the entry count.
        size_t featureCount = wgpuAdapterEnumerateFeatures(adapter, nullptr);
    
        // Allocate memory (could be a new, or a malloc() if this were a C program)
        features.resize(featureCount);
    
        // Call the function a second time, with a non-null return address
        wgpuAdapterEnumerateFeatures(adapter, features.data());
    
        std::cout << "Adapter features:" << std::endl;
        std::cout << std::hex; // Write integers as hexadecimal to ease comparison with webgpu.h literals
        for (auto f : features) {
            std::cout << " - 0x" << f << std::endl;
        }
        std::cout << std::dec; // Restore decimal numbers
        WGPUAdapterProperties properties = {};
        properties.nextInChain = nullptr;
        wgpuAdapterGetProperties(adapter, &properties);
        std::cout << "Adapter properties:" << std::endl;
        std::cout << " - vendorID: " << properties.vendorID << std::endl;
        if (properties.vendorName) {
            std::cout << " - vendorName: " << properties.vendorName << std::endl;
        }
        if (properties.architecture) {
            std::cout << " - architecture: " << properties.architecture << std::endl;
        }
        std::cout << " - deviceID: " << properties.deviceID << std::endl;
        if (properties.name) {
            std::cout << " - name: " << properties.name << std::endl;
        }
        if (properties.driverDescription) {
            std::cout << " - driverDescription: " << properties.driverDescription << std::endl;
        }
        std::cout << std::hex;
        std::cout << " - adapterType: 0x" << properties.adapterType << std::endl;
        std::cout << " - backendType: 0x" << properties.backendType << std::endl;
        std::cout << std::dec; // Restore decimal numbers
    }
    
    WGPUDevice requestDeviceSync(WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor) {
        struct UserData {
            WGPUDevice device = nullptr;
            bool requestEnded = false;
        };
        UserData userData;
    
        auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void * pUserData) {
            UserData& userData = *reinterpret_cast<UserData*>(pUserData);
            if (status == WGPURequestDeviceStatus_Success) {
                userData.device = device;
            } else {
                std::cout << "Could not get WebGPU device: " << message << std::endl;
            }
            userData.requestEnded = true;
        };
    
        wgpuAdapterRequestDevice(
            adapter,
            descriptor,
            onDeviceRequestEnded,
            (void*)&userData
        );
    
    #ifdef __EMSCRIPTEN__
        while (!userData.requestEnded) {
            emscripten_sleep(100);
        }
    #endif // __EMSCRIPTEN__
    
        assert(userData.requestEnded);
    
        return userData.device;
    }
    
    void inspectDevice(WGPUDevice device) {
        std::vector<WGPUFeatureName> features;
        size_t featureCount = wgpuDeviceEnumerateFeatures(device, nullptr);
        features.resize(featureCount);
        wgpuDeviceEnumerateFeatures(device, features.data());
    
        std::cout << "Device features:" << std::endl;
        std::cout << std::hex;
        for (auto f : features) {
            std::cout << " - 0x" << f << std::endl;
        }
        std::cout << std::dec;
    
        WGPUSupportedLimits limits = {};
        limits.nextInChain = nullptr;
    
    #ifdef WEBGPU_BACKEND_DAWN
        bool success = wgpuDeviceGetLimits(device, &limits) == WGPUStatus_Success;
    #else
        bool success = wgpuDeviceGetLimits(device, &limits);
    #endif
        
        if (success) {
            std::cout << "Device limits:" << std::endl;
            std::cout << " - maxTextureDimension1D: " << limits.limits.maxTextureDimension1D << std::endl;
            std::cout << " - maxTextureDimension2D: " << limits.limits.maxTextureDimension2D << std::endl;
            std::cout << " - maxTextureDimension3D: " << limits.limits.maxTextureDimension3D << std::endl;
            std::cout << " - maxTextureArrayLayers: " << limits.limits.maxTextureArrayLayers << std::endl;
            std::cout << " - maxBindGroups: " << limits.limits.maxBindGroups << std::endl;
            std::cout << " - maxDynamicUniformBuffersPerPipelineLayout: " << limits.limits.maxDynamicUniformBuffersPerPipelineLayout << std::endl;
            std::cout << " - maxDynamicStorageBuffersPerPipelineLayout: " << limits.limits.maxDynamicStorageBuffersPerPipelineLayout << std::endl;
            std::cout << " - maxSampledTexturesPerShaderStage: " << limits.limits.maxSampledTexturesPerShaderStage << std::endl;
            std::cout << " - maxSamplersPerShaderStage: " << limits.limits.maxSamplersPerShaderStage << std::endl;
            std::cout << " - maxStorageBuffersPerShaderStage: " << limits.limits.maxStorageBuffersPerShaderStage << std::endl;
            std::cout << " - maxStorageTexturesPerShaderStage: " << limits.limits.maxStorageTexturesPerShaderStage << std::endl;
            std::cout << " - maxUniformBuffersPerShaderStage: " << limits.limits.maxUniformBuffersPerShaderStage << std::endl;
            std::cout << " - maxUniformBufferBindingSize: " << limits.limits.maxUniformBufferBindingSize << std::endl;
            std::cout << " - maxStorageBufferBindingSize: " << limits.limits.maxStorageBufferBindingSize << std::endl;
            std::cout << " - minUniformBufferOffsetAlignment: " << limits.limits.minUniformBufferOffsetAlignment << std::endl;
            std::cout << " - minStorageBufferOffsetAlignment: " << limits.limits.minStorageBufferOffsetAlignment << std::endl;
            std::cout << " - maxVertexBuffers: " << limits.limits.maxVertexBuffers << std::endl;
            std::cout << " - maxVertexAttributes: " << limits.limits.maxVertexAttributes << std::endl;
            std::cout << " - maxVertexBufferArrayStride: " << limits.limits.maxVertexBufferArrayStride << std::endl;
            std::cout << " - maxInterStageShaderComponents: " << limits.limits.maxInterStageShaderComponents << std::endl;
            std::cout << " - maxComputeWorkgroupStorageSize: " << limits.limits.maxComputeWorkgroupStorageSize << std::endl;
            std::cout << " - maxComputeInvocationsPerWorkgroup: " << limits.limits.maxComputeInvocationsPerWorkgroup << std::endl;
            std::cout << " - maxComputeWorkgroupSizeX: " << limits.limits.maxComputeWorkgroupSizeX << std::endl;
            std::cout << " - maxComputeWorkgroupSizeY: " << limits.limits.maxComputeWorkgroupSizeY << std::endl;
            std::cout << " - maxComputeWorkgroupSizeZ: " << limits.limits.maxComputeWorkgroupSizeZ << std::endl;
            std::cout << " - maxComputeWorkgroupsPerDimension: " << limits.limits.maxComputeWorkgroupsPerDimension << std::endl;
        }
    }

    void setDefault(WGPULimits& limits){
        limits.maxTextureDimension1D =  WGPU_LIMIT_U32_UNDEFINED;
        limits.maxTextureDimension2D = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxTextureDimension3D = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxTextureArrayLayers = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxBindGroups = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxBindGroupsPlusVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxBindingsPerBindGroup = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxDynamicUniformBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxDynamicStorageBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxSampledTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxSamplersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxStorageBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxStorageTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxUniformBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxUniformBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
        limits.maxStorageBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
        limits.minUniformBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
        limits.minStorageBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxBufferSize = WGPU_LIMIT_U64_UNDEFINED;
        limits.maxVertexAttributes = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxVertexBufferArrayStride = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxInterStageShaderComponents = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxInterStageShaderVariables = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxColorAttachments = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxColorAttachmentBytesPerSample = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxComputeWorkgroupStorageSize = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxComputeInvocationsPerWorkgroup = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxComputeWorkgroupSizeX = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxComputeWorkgroupSizeY = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxComputeWorkgroupSizeZ = WGPU_LIMIT_U32_UNDEFINED;
        limits.maxComputeWorkgroupsPerDimension = WGPU_LIMIT_U32_UNDEFINED;
    }

    void setDefault(WGPUBindGroupLayoutEntry &bindingLayout) {
        bindingLayout.buffer.nextInChain = nullptr;
        bindingLayout.buffer.type = WGPUBufferBindingType_Undefined;
        bindingLayout.buffer.hasDynamicOffset = false;
    
        bindingLayout.sampler.nextInChain = nullptr;
        bindingLayout.sampler.type = WGPUSamplerBindingType_Undefined;
    
        bindingLayout.storageTexture.nextInChain = nullptr;
        bindingLayout.storageTexture.access = WGPUStorageTextureAccess_Undefined;
        bindingLayout.storageTexture.format = WGPUTextureFormat_Undefined;
        bindingLayout.storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;
    
        bindingLayout.texture.nextInChain = nullptr;
        bindingLayout.texture.multisampled = false;
        bindingLayout.texture.sampleType = WGPUTextureSampleType_Undefined;
        bindingLayout.texture.viewDimension = WGPUTextureViewDimension_Undefined;
    }
    
    void setDefault(WGPUStencilFaceState &stencilFaceState){
        stencilFaceState.compare = WGPUCompareFunction_Always;
        stencilFaceState.failOp = WGPUStencilOperation_Keep;
        stencilFaceState.depthFailOp = WGPUStencilOperation_Keep;
        stencilFaceState.passOp = WGPUStencilOperation_Keep;
    }
    
    void setDefault(WGPUDepthStencilState &depthStencilState){
        depthStencilState.format = WGPUTextureFormat_Undefined;
        depthStencilState.depthWriteEnabled = false;
        depthStencilState.depthCompare = WGPUCompareFunction_Always;
        depthStencilState.stencilReadMask = 0xFFFFFFFF;
        depthStencilState.stencilWriteMask = 0xFFFFFFFF;
        depthStencilState.depthBias = 0;
        depthStencilState.depthBiasSlopeScale = 0;
        depthStencilState.depthBiasClamp = 0;
        setDefault(depthStencilState.stencilFront);
        setDefault(depthStencilState.stencilBack);
    }
};

using namespace Ultis;

WebGPUGraphicsDevice::WebGPUGraphicsDevice(){

}

void WebGPUGraphicsDevice::LoadContext(void* data){

}

WGPURequiredLimits WebGPUGraphicsDevice::GetRequiredLimits(WGPUAdapter adapter) const {
	// Get adapter supported limits, in case we need them
	WGPUSupportedLimits supportedLimits{};
	supportedLimits.nextInChain = nullptr;
	wgpuAdapterGetLimits(adapter, &supportedLimits);

	WGPURequiredLimits requiredLimits{};
	setDefault(requiredLimits.limits);

    requiredLimits.limits.maxTextureDimension1D = 2048;
    requiredLimits.limits.maxTextureDimension2D = 2048;

	// We use at most 2 vertex attributes
	requiredLimits.limits.maxVertexAttributes = 15;
	// We should also tell that we use 1 vertex buffers
	requiredLimits.limits.maxVertexBuffers = 8; //15;
	// Maximum size of a buffer is 6 vertices of 5 float each
	requiredLimits.limits.maxBufferSize = sizeof(Matrix4) * 10000;
	// Maximum stride between 2 consecutive vertices in the vertex buffer
	requiredLimits.limits.maxVertexBufferArrayStride = sizeof(Matrix4) * 2;

	// There is a maximum of 3 float forwarded from vertex to fragment shader
	requiredLimits.limits.maxInterStageShaderComponents = 40;

	// We use at most 1 bind group for now
	requiredLimits.limits.maxBindGroups = 4;
	// We use at most 1 uniform buffer per stage
	requiredLimits.limits.maxUniformBuffersPerShaderStage = 10;
	// Uniform structs have a size of maximum 16 float (more than what we need)
	requiredLimits.limits.maxUniformBufferBindingSize = sizeof(Matrix4) * 200;

	// These two limits are different because they are "minimum" limits,
	// they are the only ones we are may forward from the adapter's supported
	// limits.
	requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
	requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;

	return requiredLimits;
}

void WebGPUGraphicsDevice::Initialize(){
    WGPUInstance instance = wgpuCreateInstance(nullptr);
	
	std::cout << "Requesting adapter..." << std::endl;
	surface = glfwGetWGPUSurface(instance, (GLFWwindow*)Platform::GetInternalData());
	WGPURequestAdapterOptions adapterOpts = {};
	adapterOpts.nextInChain = nullptr;
	adapterOpts.compatibleSurface = surface;
	WGPUAdapter adapter = requestAdapterSync(instance, &adapterOpts);
	std::cout << "Got adapter: " << adapter << std::endl;
	
	wgpuInstanceRelease(instance);
	
	std::cout << "Requesting device..." << std::endl;
	WGPUDeviceDescriptor deviceDesc = {};
	deviceDesc.nextInChain = nullptr;
	deviceDesc.label = "My Device";
	deviceDesc.requiredFeatureCount = 0;
	WGPURequiredLimits requiredLimits = GetRequiredLimits(adapter);
    deviceDesc.requiredLimits = &requiredLimits;
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = "The default queue";
	deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
		std::cout << "Device lost: reason " << reason;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	};
	device = requestDeviceSync(adapter, &deviceDesc);
	std::cout << "Got device: " << device << std::endl;
	
	auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
		std::cout << "Uncaptured device error: type " << type;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	};
	wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData */);
	
	queue = wgpuDeviceGetQueue(device);

	// Configure the surface
	WGPUSurfaceConfiguration config = {};
	config.nextInChain = nullptr;

	// Configuration of the textures created for the underlying swap chain
	config.width = Application::ScreenWidth();// 640;
	config.height = Application::ScreenHeight();// 480;
	config.usage = WGPUTextureUsage_RenderAttachment;
	surfaceFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);
	config.format = surfaceFormat;

	// And we do not need any particular view format:
	config.viewFormatCount = 0;
	config.viewFormats = nullptr;
	config.device = device;
	config.presentMode = WGPUPresentMode_Fifo;
	config.alphaMode = WGPUCompositeAlphaMode_Auto;

    // Create the depth texture
    WGPUTextureDescriptor depthTextureDesc{};
    depthTextureDesc.dimension = WGPUTextureDimension_2D;
    depthTextureDesc.format = depthTextureFormat;
    depthTextureDesc.mipLevelCount = 1;
    depthTextureDesc.sampleCount = 1;
    depthTextureDesc.size = {(uint32_t)Application::ScreenWidth(), (uint32_t)Application::ScreenHeight(), 1};
    depthTextureDesc.usage = WGPUTextureUsage_RenderAttachment;
    depthTextureDesc.viewFormatCount = 1;
    depthTextureDesc.viewFormats = &depthTextureFormat;
    depthTexture = wgpuDeviceCreateTexture(device, &depthTextureDesc);

    WGPUTextureViewDescriptor depthTextureViewDesc{};
    depthTextureViewDesc.nextInChain = nullptr;
    depthTextureViewDesc.aspect = WGPUTextureAspect_DepthOnly;
    depthTextureViewDesc.baseArrayLayer = 0;
    depthTextureViewDesc.arrayLayerCount = 1;
    depthTextureViewDesc.baseMipLevel = 0;
    depthTextureViewDesc.mipLevelCount = 1;
    depthTextureViewDesc.dimension = WGPUTextureViewDimension_2D;
    depthTextureViewDesc.format = depthTextureFormat;
    depthTextureView = wgpuTextureCreateView(depthTexture, &depthTextureViewDesc);

	wgpuSurfaceConfigure(surface, &config);

	// Release the adapter only after it has been fully utilized
	wgpuAdapterRelease(adapter);

    /////////////////////////////
    auto CreateVertexBuffer = [&](size_t size, const char* label){
        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.nextInChain = nullptr;
        bufferDesc.label = label;
        bufferDesc.size = size;
        bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex; // Vertex usage here!
        bufferDesc.mappedAtCreation = false;
        WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
        return buffer;
    };

    auto CreateUniformBuffer = [&](size_t size, const char* label){
        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.nextInChain = nullptr;
        bufferDesc.label = label;
        bufferDesc.size = size;
        bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
        bufferDesc.mappedAtCreation = false;
        WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
        return buffer;
    };

    WGPUBindGroupLayoutEntry cambindingLayout{};
    setDefault(cambindingLayout);
    cambindingLayout.binding = 0;// The binding index as used in the @binding attribute in the shader
    cambindingLayout.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;// The stage that needs to access this resource
    cambindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
    cambindingLayout.buffer.minBindingSize = sizeof(CameraDrawData);
    WGPUBindGroupLayoutDescriptor cambindGroupLayoutDesc{};
    cambindGroupLayoutDesc.nextInChain = nullptr;
    cambindGroupLayoutDesc.entryCount = 1;
    cambindGroupLayoutDesc.entries = &cambindingLayout;
    cameraBindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &cambindGroupLayoutDesc);

    cameraUniformBuffer = CreateUniformBuffer(sizeof(CameraDrawData), "CameraUniformBuffer");

    WGPUBindGroupEntry binding{};
    binding.nextInChain = nullptr;
    binding.binding = 0;// The index of the binding (the entries in bindGroupDesc can be in any order)
    binding.buffer = cameraUniformBuffer;// The buffer it is actually bound to
    binding.offset = 0;
    binding.size = sizeof(CameraDrawData);
    WGPUBindGroupDescriptor bindGroupDesc{};// A bind group contains one or multiple bindings
    bindGroupDesc.nextInChain = nullptr;
    bindGroupDesc.layout = cameraBindGroupLayout;// bindGroupLayout;
    bindGroupDesc.entryCount = 1;// There must be as many bindings as declared in the layout!
    bindGroupDesc.entries = &binding;
    cameraBindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);

    ///////////////////////////////

    perDrawDatas.resize(maxPerDraw);

    WGPUBindGroupLayoutEntry bindingLayout{};
    setDefault(bindingLayout);
    bindingLayout.binding = 0;// The binding index as used in the @binding attribute in the shader
    bindingLayout.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;// The stage that needs to access this resource
    bindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
    bindingLayout.buffer.minBindingSize = sizeof(Matrix4);
    WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.nextInChain = nullptr;
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries = &bindingLayout;
    perDrawBindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);

    for(int i = 0; i < perDrawDatas.size(); i++){
        perDrawDatas[i].uniformBuffer = CreateUniformBuffer(sizeof(Matrix4), "PerDraw");

        WGPUBindGroupEntry binding{};
        binding.nextInChain = nullptr;
        binding.binding = 0;// The index of the binding (the entries in bindGroupDesc can be in any order)
        binding.buffer = perDrawDatas[i].uniformBuffer;// The buffer it is actually bound to
        binding.offset = 0;
        binding.size = sizeof(Matrix4);
        WGPUBindGroupDescriptor bindGroupDesc{};// A bind group contains one or multiple bindings
        bindGroupDesc.nextInChain = nullptr;
        bindGroupDesc.layout = perDrawBindGroupLayout;// bindGroupLayout;
        bindGroupDesc.entryCount = 1;// There must be as many bindings as declared in the layout!
        bindGroupDesc.entries = &binding;
        perDrawDatas[i].bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
    }

    ////////////////////////////////
    perDrawSkinnedDatas.resize(maxPerDrawSkinned);

    std::vector<WGPUBindGroupLayoutEntry> skinnedBindingLayout(2);
    setDefault(skinnedBindingLayout[0]);
    setDefault(skinnedBindingLayout[1]);
    skinnedBindingLayout[0].binding = 0;// The binding index as used in the @binding attribute in the shader
    skinnedBindingLayout[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;// The stage that needs to access this resource
    skinnedBindingLayout[0].buffer.type = WGPUBufferBindingType_Uniform;
    skinnedBindingLayout[0].buffer.minBindingSize = sizeof(Matrix4);
    skinnedBindingLayout[1].binding = 1;// The binding index as used in the @binding attribute in the shader
    skinnedBindingLayout[1].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;// The stage that needs to access this resource
    skinnedBindingLayout[1].buffer.type = WGPUBufferBindingType_Uniform;
    skinnedBindingLayout[1].buffer.minBindingSize = sizeof(Matrix4) * 120;
    //WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.nextInChain = nullptr;
    bindGroupLayoutDesc.entryCount = skinnedBindingLayout.size();
    bindGroupLayoutDesc.entries = skinnedBindingLayout.data();
    perDrawSkinnedBindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);

    for(int i = 0; i < perDrawSkinnedDatas.size(); i++){
        perDrawSkinnedDatas[i].uniformBuffer = CreateUniformBuffer(sizeof(Matrix4), "PerDrawSkinned0");
        perDrawSkinnedDatas[i].uniformBuffer1 = CreateUniformBuffer(sizeof(Matrix4) * 120, "PerDrawSkinned1");

        std::vector<WGPUBindGroupEntry> binding(2);
        binding[0].nextInChain = nullptr;
        binding[0].binding = 0;// The index of the binding (the entries in bindGroupDesc can be in any order)
        binding[0].buffer = perDrawSkinnedDatas[i].uniformBuffer;// The buffer it is actually bound to
        binding[0].offset = 0;
        binding[0].size = sizeof(Matrix4);
        binding[1].nextInChain = nullptr;
        binding[1].binding = 1;// The index of the binding (the entries in bindGroupDesc can be in any order)
        binding[1].buffer = perDrawSkinnedDatas[i].uniformBuffer1;// The buffer it is actually bound to
        binding[1].offset = 0;
        binding[1].size = sizeof(Matrix4) * 120;
        WGPUBindGroupDescriptor bindGroupDesc{};// A bind group contains one or multiple bindings
        bindGroupDesc.nextInChain = nullptr;
        bindGroupDesc.layout = perDrawSkinnedBindGroupLayout;// bindGroupLayout;
        bindGroupDesc.entryCount = binding.size();// There must be as many bindings as declared in the layout!
        bindGroupDesc.entries = binding.data();

        perDrawSkinnedDatas[i].bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
    }
    ///////////////////////////////////////////////////////////////////
    perDrawInstancingDatas.resize(maxPerDrawInstancing);
    for(int i = 0; i < perDrawInstancingDatas.size(); i++){
        perDrawInstancingDatas[i] = CreateVertexBuffer(sizeof(Matrix4) * 1000, "PerDrawInstancing");
    }

    InitRenderPasses();

    /////////////////////
    Texture2DCreate(defaultTex, "Engine/Textures/brickwall.jpg", Texture2DSetting());

    ImGui_ImplWGPU_Init(device, 3, surfaceFormat, depthTextureFormat);
}

void WebGPUGraphicsDevice::InitRenderPasses(){
    renderPasses.resize(2);

    renderPasses[0].colorAttachments.resize(1);
    renderPasses[0].colorAttachments[0] = {
        surfaceFormat
    };
    renderPasses[0].depthStencilAttachments = {
        depthTextureFormat
    };

    renderPasses[1].colorAttachments.resize(1);
    renderPasses[1].colorAttachments[0] = {
        WGPUTextureFormat_RGBA16Float //WGPUTextureFormat_BGRA8Unorm
    };
    renderPasses[1].depthStencilAttachments = {
        WGPUTextureFormat_Depth24Plus
    };
}

void WebGPUGraphicsDevice::Shutdown(){
    ImGui_ImplWGPU_Shutdown();

    wgpuTextureViewRelease(depthTextureView);
    wgpuTextureDestroy(depthTexture);
    wgpuTextureRelease(depthTexture);

	wgpuSurfaceUnconfigure(surface);
	wgpuQueueRelease(queue);
	wgpuSurfaceRelease(surface);
	wgpuDeviceRelease(device);
}

WGPUTextureView WebGPUGraphicsDevice::GetNextSurfaceTextureView(){
	// Get the surface texture
	WGPUSurfaceTexture surfaceTexture{};
	wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);
	if(surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success){
		return nullptr;
	}

	// Create a view for this surface texture
	WGPUTextureViewDescriptor viewDescriptor{};
	viewDescriptor.nextInChain = nullptr;
	viewDescriptor.label = "Surface texture view";
	viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
	viewDescriptor.dimension = WGPUTextureViewDimension_2D;
	viewDescriptor.baseMipLevel = 0;
	viewDescriptor.mipLevelCount = 1;
	viewDescriptor.baseArrayLayer = 0;
	viewDescriptor.arrayLayerCount = 1;
	viewDescriptor.aspect = WGPUTextureAspect_All;
	WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);

#ifndef WEBGPU_BACKEND_WGPU
	// We no longer need the texture, only its view
	// (NB: with wgpu-native, surface textures must not be manually released)
	wgpuTextureRelease(surfaceTexture.texture);
#endif // WEBGPU_BACKEND_WGPU

	return targetView;
}

void WebGPUGraphicsDevice::_Begin(){
	// Get the next target texture view
	targetView = GetNextSurfaceTextureView();
    Assert(targetView != nullptr);
	//if(!targetView) return;

	// Create a command encoder for the draw call
	WGPUCommandEncoderDescriptor encoderDesc = {};
	encoderDesc.nextInChain = nullptr;
	encoderDesc.label = "My command encoder";
	/*WGPUCommandEncoder*/ encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

	// Create the render pass that clears the screen with our color
	/*WGPURenderPassDescriptor renderPassDesc = {};
	renderPassDesc.nextInChain = nullptr;

	// The attachment part of the render pass descriptor describes the target texture of the pass
	WGPURenderPassColorAttachment renderPassColorAttachment = {};
	renderPassColorAttachment.view = targetView;
	renderPassColorAttachment.resolveTarget = nullptr;
	renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
	renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
	renderPassColorAttachment.clearValue = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };
#ifndef WEBGPU_BACKEND_WGPU
	renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif // NOT WEBGPU_BACKEND_WGPU

    WGPURenderPassDepthStencilAttachment depthStencilAttachment;
    depthStencilAttachment.view = depthTextureView;
    depthStencilAttachment.depthClearValue = 1.0f;// The initial value of the depth buffer, meaning "far"
    depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;// Operation settings comparable to the color attachment
    depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
    depthStencilAttachment.depthReadOnly = false;// we could turn off writing to the depth buffer globally here
    depthStencilAttachment.stencilClearValue = 0;// Stencil setup, mandatory but unused
#ifdef WEBGPU_BACKEND_WGPU
    depthStencilAttachment.stencilLoadOp = WGPULoadOp_Clear;
    depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Store;
#else
    depthStencilAttachment.stencilLoadOp = LoadOp::Undefined;
    depthStencilAttachment.stencilStoreOp = StoreOp::Undefined;
#endif
    depthStencilAttachment.stencilReadOnly = true;

	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;
	renderPassDesc.depthStencilAttachment = &depthStencilAttachment;;
	renderPassDesc.timestampWrites = nullptr;

	// Create the render pass and end it immediately (we only clear the screen but do not draw anything)
	renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);*/
}

void WebGPUGraphicsDevice::_End(){
    //wgpuRenderPassEncoderEnd(renderPass);
	//wgpuRenderPassEncoderRelease(renderPass);

	// Finally encode and submit the render pass
	WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
	cmdBufferDescriptor.nextInChain = nullptr;
	cmdBufferDescriptor.label = "Command buffer";
	WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
	wgpuCommandEncoderRelease(encoder);

	//std::cout << "Submitting command..." << std::endl;
	wgpuQueueSubmit(queue, 1, &command);
	wgpuCommandBufferRelease(command);
	//std::cout << "Command submitted." << std::endl;

	// At the end of the frame
	wgpuTextureViewRelease(targetView);
#ifndef __EMSCRIPTEN__
	wgpuSurfacePresent(surface);
#endif

#if defined(WEBGPU_BACKEND_DAWN)
	wgpuDeviceTick(device);
#elif defined(WEBGPU_BACKEND_WGPU)
	wgpuDevicePoll(device, false, nullptr);
#endif
}

GraphicsStats& WebGPUGraphicsDevice::GetStats(){
    return stats;
}

GraphicsDeviceInfo WebGPUGraphicsDevice::GetInfo(){
    return info;
}

void WebGPUGraphicsDevice::Begin(){
    stats.drawCalls = 0;
    stats.vertices = 0;
    stats.tris = 0;
    stats.shaderBinds = 0;
    stats.uniformSet = 0;
    stats.materialSubmitDatas = 0;
    //begin = true;
    lastMat = nullptr;
    lastShader = nullptr;
    lastMesh = nullptr;

    curPerDrawData = 0;
    curPerDrawSkinnedData = 0;
    curPerDrawInstancingData = 0;
}

void WebGPUGraphicsDevice::End(){

}

bool WebGPUGraphicsDevice::HasBegin(){
    return false;
}

void WebGPUGraphicsDevice::SetCamera(Camera& inCamera){
    camera = inCamera;
    cameraDrawData.projection = camera.projection;
    cameraDrawData.view = camera.view;
    wgpuQueueWriteBuffer(queue, cameraUniformBuffer, 0, &cameraDrawData, sizeof(CameraDrawData));
}

Camera WebGPUGraphicsDevice::GetCamera(){
    return camera;
}

void WebGPUGraphicsDevice::BeginRenderToScreen(Vector4 clearColor){
    lastMat = nullptr;
    lastShader = nullptr;
    lastMesh = nullptr;
    currentRendePassTarget = 0;

    // Create the render pass that clears the screen with our color
	WGPURenderPassDescriptor renderPassDesc = {};
	renderPassDesc.nextInChain = nullptr;

    // The attachment part of the render pass descriptor describes the target texture of the pass
	WGPURenderPassColorAttachment renderPassColorAttachment = {};
	renderPassColorAttachment.view = targetView;
	renderPassColorAttachment.resolveTarget = nullptr;
	renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
	renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
	renderPassColorAttachment.clearValue = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };
#ifndef WEBGPU_BACKEND_WGPU
	renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif // NOT WEBGPU_BACKEND_WGPU

    WGPURenderPassDepthStencilAttachment depthStencilAttachment = {};
    depthStencilAttachment.view = depthTextureView;
    depthStencilAttachment.depthClearValue = 1.0f;// The initial value of the depth buffer, meaning "far"
    depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;// Operation settings comparable to the color attachment
    depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
    depthStencilAttachment.depthReadOnly = false;// we could turn off writing to the depth buffer globally here
    depthStencilAttachment.stencilClearValue = 0;// Stencil setup, mandatory but unused
#ifdef WEBGPU_BACKEND_WGPU
    depthStencilAttachment.stencilLoadOp = WGPULoadOp_Clear;
    depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Store;
#else
    depthStencilAttachment.stencilLoadOp = WGPULoadOp_Undefined;
    depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Undefined;
#endif
    depthStencilAttachment.stencilReadOnly = true;

	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;
	renderPassDesc.depthStencilAttachment = &depthStencilAttachment;;
	renderPassDesc.timestampWrites = nullptr;

	// Create the render pass and end it immediately (we only clear the screen but do not draw anything)
	renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
}

void WebGPUGraphicsDevice::EndRenderToScreen(){
    Application::DrawImGui();
    wgpuRenderPassEncoderEnd(renderPass);
	//wgpuRenderPassEncoderRelease(renderPass);
}

void WebGPUGraphicsDevice::Clean(float r, float g, float b, float a){
    
}

void WebGPUGraphicsDevice::SetViewport(unsigned int x, unsigned int y, unsigned int w, unsigned int h){

}

void WebGPUGraphicsDevice::GetViewport(unsigned int*x, unsigned int* y, unsigned int* w, unsigned int* h){

}

void WebGPUGraphicsDevice::BindMaterial(Material& mat){
    /*auto ContainUniformName = [&](SubShader shader, const std::string& name){ 
        return std::find(shader.glData._uniforms.begin(), shader.glData._uniforms.end(), name) != shader.glData._uniforms.end(); 
    };*/

    auto ApplyUniformTo = [&](Material& material, SubShader& shader, std::unordered_map<std::string, MaterialMap>& maps, std::vector<TexTarget>& texs){
        for(auto& i: maps){
            MaterialMap& map = i.second;

            //if(material.wgData.materialMainSetDef.bufferMembers.count(i.first) <= 0) continue;
            //MaterialMainSetDef::Member m = material.wgData.materialMainSetDef.bufferMembers[i.first];

            if(map.type == MaterialMap::Type::Int){
                if(material.wgData.materialMainSetDef.bufferMembers.count(i.first) <= 0) continue;
                MaterialMainSetDef::Member m = material.wgData.materialMainSetDef.bufferMembers[i.first];
                Assert(m.size >= sizeof(int));
                memcpy((char*)material.wgData.mainUniformData + m.pos, &map.valueInt, sizeof(int));
                //SubShaderSetInt(shader, i.first.c_str(), map.valueInt);
            }
            if(map.type == MaterialMap::Type::Float){
                if(material.wgData.materialMainSetDef.bufferMembers.count(i.first) <= 0) continue;
                MaterialMainSetDef::Member m = material.wgData.materialMainSetDef.bufferMembers[i.first];
                Assert(m.size >= sizeof(float));
                memcpy((char*)material.wgData.mainUniformData + m.pos, &map.valueFloat, sizeof(float));
                //SubShaderSetFloat(shader, i.first.c_str(), map.valueFloat);
            }
            if(map.type == MaterialMap::Type::Vector2){
                if(material.wgData.materialMainSetDef.bufferMembers.count(i.first) <= 0) continue;
                MaterialMainSetDef::Member m = material.wgData.materialMainSetDef.bufferMembers[i.first];
                Assert(m.size >= sizeof(Vector2));
                memcpy((char*)material.wgData.mainUniformData + m.pos, &map.vec.vector, sizeof(Vector2));
                //SubShaderSetVector2(shader, i.first.c_str(), Vector2(map.vec.vector.x, map.vec.vector.y));
            }
            if(map.type == MaterialMap::Type::Vector3){
                //SubShaderSetVector3(shader, i.first.c_str(), Vector3(map.vec.vector.x, map.vec.vector.y, map.vec.vector.z));
                if(material.wgData.materialMainSetDef.bufferMembers.count(i.first) <= 0) continue;
                MaterialMainSetDef::Member m = material.wgData.materialMainSetDef.bufferMembers[i.first];
                Assert(m.size >= sizeof(Vector3));
                memcpy((char*)material.wgData.mainUniformData + m.pos, &map.vec.vector, sizeof(Vector3));
            }
            if(map.type == MaterialMap::Type::Vector4){
                //SubShaderSetVector4(shader, i.first.c_str(), map.vec.vector);
                if(material.wgData.materialMainSetDef.bufferMembers.count(i.first) <= 0) continue;
                MaterialMainSetDef::Member m = material.wgData.materialMainSetDef.bufferMembers[i.first];
                Assert(m.size >= sizeof(Vector4));
                memcpy((char*)material.wgData.mainUniformData + m.pos, &map.vec.vector, sizeof(Vector4));
            }
            if(map.type == MaterialMap::Type::Matrix4){
                //SubShaderSetMatrix4(shader, i.first.c_str(), i.second.matrix);
                if(material.wgData.materialMainSetDef.bufferMembers.count(i.first) <= 0) continue;
                MaterialMainSetDef::Member m = material.wgData.materialMainSetDef.bufferMembers[i.first];
                Assert(m.size >= sizeof(Matrix4));
                memcpy((char*)material.wgData.mainUniformData + m.pos, &i.second.matrix, sizeof(Matrix4));
            }
            if(map.type == MaterialMap::Type::Texture){
                if(mat.wgData.materialMainSetDef.textureBindings.count(i.first) <= 0) continue;
                int slot = mat.wgData.materialMainSetDef.textureBindings[i.first] - 1;
                texs[slot].textureView = i.second.texture->wgData.textureView;
                texs[slot].sampler = i.second.texture->wgData.sampler;

                //texs.push_back(std::make_pair(i.first, i.second.texture.get()));
                //Assert(false);
                /*Assert(i.second.texture != nullptr);
                SubShaderSetTexture2D(shader, i.first.c_str(), *i.second.texture, material.currentTextureSlot);
                material.currentTextureSlot += 1;*/
            }
            if(map.type == MaterialMap::Type::TextureArray){
                //Assert(false);
                //SubShaderSetTexture2DArray(shader, i.first.c_str(), *i.second.textureArray, material.currentTextureSlot);
                //material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::Framebuffer){
                if(mat.wgData.materialMainSetDef.textureBindings.count(i.first) <= 0) continue;
                int slot = mat.wgData.materialMainSetDef.textureBindings[i.first] - 1;
                texs[slot].textureView = i.second.framebuffer->wgData.textureView;
                texs[slot].sampler = i.second.framebuffer->wgData.sampler;
                //Assert(false);
                //SubShaderSetFramebuffer(shader, i.first.c_str(), *i.second.framebuffer, material.currentTextureSlot, map.framebufferAttachment);
                //material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::Cubemap){
                //Assert(false);
                //SubShaderSetCubemap(shader, i.first.c_str(), *i.second.cubemap, material.currentTextureSlot);
                //material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::FloatList){
                //SubShaderSetFloat(shader, i.first.c_str(), static_cast<float*>(map.list), map.listCount);
                if(material.wgData.materialMainSetDef.bufferMembers.count(i.first) <= 0) continue;
                MaterialMainSetDef::Member m = material.wgData.materialMainSetDef.bufferMembers[i.first];
                Assert(m.size >= sizeof(float) * map.listCount);
                memcpy((char*)material.wgData.mainUniformData + m.pos, static_cast<float*>(map.list), sizeof(float) * map.listCount);
            }
            if(map.type == MaterialMap::Type::Vector4List){
                //SubShaderSetVector4(shader, i.first.c_str(), static_cast<Vector4*>(map.list), map.listCount);
                if(material.wgData.materialMainSetDef.bufferMembers.count(i.first) <= 0) continue;
                MaterialMainSetDef::Member m = material.wgData.materialMainSetDef.bufferMembers[i.first];
                Assert(m.size >= sizeof(Vector4) * map.listCount);
                memcpy((char*)material.wgData.mainUniformData + m.pos, static_cast<float*>(map.list), sizeof(Vector4) * map.listCount);
            }
            if(map.type == MaterialMap::Type::Matrix4List){
                //SubShaderSetMatrix4(shader, i.first.c_str(), static_cast<Matrix4*>(map.list), map.listCount);
                if(material.wgData.materialMainSetDef.bufferMembers.count(i.first) <= 0) continue;
                MaterialMainSetDef::Member m = material.wgData.materialMainSetDef.bufferMembers[i.first];
                Assert(m.size >= sizeof(Matrix4) * map.listCount);
                memcpy((char*)material.wgData.mainUniformData + m.pos, static_cast<float*>(map.list), sizeof(Matrix4) * map.listCount);
            }
        }
    };

    auto SubmitGraphicDatas = [&](Material& material){
        stats.materialSubmitDatas += 1;
        material.UpdateCurrentShader();

        Assert(material.GetShader() != nullptr);
        if(material.GetShader() == nullptr) return;

        SubShaderBind(*material.currentShader);

        std::vector<TexTarget> texs(maxTexSlots);
        //texs.resize(maxTexSlots);
        material.currentTextureSlot = 0;
        ApplyUniformTo(material, *material.currentShader, material.maps, texs);
        ApplyUniformTo(material, *material.currentShader, Material::globalMaps, texs);
        Assert(material.currentTextureSlot < maxTexSlots /*32*/);
        //memset(material.wgData.mainUniformData, 0, material.wgData.materialMainSetDef.bufferSize);
        wgpuQueueWriteBuffer(queue, material.wgData.mainUniformBuffer, 0, material.wgData.mainUniformData, material.wgData.materialMainSetDef.bufferSize);
        //if(material.isDirty){
        UpdateMaterialMainSet(mat, texs);
        //}
    };

    Assert(mat.currentShader != nullptr && "Shader is not vali!");
    Assert(mat.GetShader()->IsComplete() == true && "Shader is not vali!");

    if(&mat != lastMat || mat.isDirty == true){
        SubmitGraphicDatas(mat);
        mat.isDirty = false;
    }
    lastMat = &mat;
    
    if(mat.currentShader.get() != lastShader){
        //SubShaderBind(*mat.currentShader);
        //SubShaderSetMatrix4(*mat.currentShader, "projection", camera.projection); //mat.currentShader->SetMatrix4("projection", camera.projection);
        //SubShaderSetMatrix4(*mat.currentShader, "view", camera.view); //mat.currentShader->SetMatrix4("view", camera.view);
    }
    lastShader = mat.currentShader.get();
}

void WebGPUGraphicsDevice::DrawMesh(Mesh& mesh, Matrix4 modelMatrix, OD::PerDrawData* perDrawData){
    if(&mesh != lastMesh){
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, mesh.wgData.vertexBuffer, 0, wgpuBufferGetSize(mesh.wgData.vertexBuffer));
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 1, mesh.wgData.uvBuffer, 0, wgpuBufferGetSize(mesh.wgData.uvBuffer));
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 2, mesh.wgData.normalBuffer, 0, wgpuBufferGetSize(mesh.wgData.normalBuffer));
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 3, mesh.wgData.colorBuffer, 0, wgpuBufferGetSize(mesh.wgData.colorBuffer));
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 4, mesh.wgData.tangentBuffer, 0, wgpuBufferGetSize(mesh.wgData.tangentBuffer));

        if(mesh.wgData.indexBuffer != nullptr)
            wgpuRenderPassEncoderSetIndexBuffer(renderPass, mesh.wgData.indexBuffer, WGPUIndexFormat_Uint32, 0, wgpuBufferGetSize(mesh.wgData.indexBuffer));

        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, lastMat->wgData.mainBindGroup, 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(renderPass, 2, cameraBindGroup, 0, nullptr);
    }
    lastMesh = &mesh;

    Assert(curPerDrawData < perDrawDatas.size());
    wgpuQueueWriteBuffer(queue, perDrawDatas[curPerDrawData].uniformBuffer, 0, &modelMatrix, sizeof(Matrix4));
    wgpuRenderPassEncoderSetBindGroup(renderPass, 1, perDrawDatas[curPerDrawData].bindGroup, 0, nullptr);
    curPerDrawData += 1;

    if(mesh.wgData.indexBuffer != nullptr){
        wgpuRenderPassEncoderDrawIndexed(renderPass, mesh.indiceCount, 1, 0, 0, 0);
    } else {
        wgpuRenderPassEncoderDraw(renderPass, mesh.vertexCount, 1, 0, 0);
    }
}

void WebGPUGraphicsDevice::DrawMeshSkinned(Mesh& mesh, Matrix4 model, Matrix4* animMatrix, int count, OD::PerDrawData* perDrawData){
    //DrawMesh(mesh, model); return;
    if(&mesh != lastMesh){
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, mesh.wgData.vertexBuffer, 0, wgpuBufferGetSize(mesh.wgData.vertexBuffer));
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 1, mesh.wgData.uvBuffer, 0, wgpuBufferGetSize(mesh.wgData.uvBuffer));
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 2, mesh.wgData.normalBuffer, 0, wgpuBufferGetSize(mesh.wgData.normalBuffer));
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 3, mesh.wgData.colorBuffer, 0, wgpuBufferGetSize(mesh.wgData.colorBuffer));
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 4, mesh.wgData.tangentBuffer, 0, wgpuBufferGetSize(mesh.wgData.tangentBuffer));
        
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 5, mesh.wgData.jointBuffer, 0, wgpuBufferGetSize(mesh.wgData.jointBuffer));
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 6, mesh.wgData.weightsBuffer, 0, wgpuBufferGetSize(mesh.wgData.weightsBuffer));
        if(mesh.wgData.indexBuffer != nullptr)
            wgpuRenderPassEncoderSetIndexBuffer(renderPass, mesh.wgData.indexBuffer, WGPUIndexFormat_Uint32, 0, wgpuBufferGetSize(mesh.wgData.indexBuffer));

        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, lastMat->wgData.mainBindGroup, 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(renderPass, 2, cameraBindGroup, 0, nullptr);
    }
    lastMesh = &mesh;

    Assert(curPerDrawSkinnedData < perDrawSkinnedDatas.size());
    wgpuQueueWriteBuffer(queue, perDrawSkinnedDatas[curPerDrawSkinnedData].uniformBuffer, 0, &model, sizeof(Matrix4));
    wgpuQueueWriteBuffer(queue, perDrawSkinnedDatas[curPerDrawSkinnedData].uniformBuffer1, 0, animMatrix, sizeof(Matrix4) * count);
    wgpuRenderPassEncoderSetBindGroup(renderPass, 1, perDrawSkinnedDatas[curPerDrawSkinnedData].bindGroup, 0, nullptr);
    curPerDrawSkinnedData += 1;

    if(mesh.wgData.indexBuffer != nullptr){
        wgpuRenderPassEncoderDrawIndexed(renderPass, mesh.indiceCount, 1, 0, 0, 0);
    } else {
        wgpuRenderPassEncoderDraw(renderPass, mesh.vertexCount, 1, 0, 0);
    }
}

void WebGPUGraphicsDevice::DrawMeshInstancing(Mesh& mesh, Matrix4* modelMatrixs, int count){
    auto Draw = [&](Matrix4* inmodelMatrixs, int inCount){
        //if(&mesh != lastMesh){
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, mesh.wgData.vertexBuffer, 0, wgpuBufferGetSize(mesh.wgData.vertexBuffer));
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 1, mesh.wgData.uvBuffer, 0, wgpuBufferGetSize(mesh.wgData.uvBuffer));
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 2, mesh.wgData.normalBuffer, 0, wgpuBufferGetSize(mesh.wgData.normalBuffer));
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 3, mesh.wgData.colorBuffer, 0, wgpuBufferGetSize(mesh.wgData.colorBuffer));
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 4, mesh.wgData.tangentBuffer, 0, wgpuBufferGetSize(mesh.wgData.tangentBuffer));

        Assert(curPerDrawInstancingData < perDrawInstancingDatas.size());
        wgpuQueueWriteBuffer(queue, perDrawInstancingDatas[curPerDrawInstancingData], 0, inmodelMatrixs, sizeof(Matrix4) * inCount);
        wgpuRenderPassEncoderSetVertexBuffer(
            renderPass, 
            5, 
            perDrawInstancingDatas[curPerDrawInstancingData], 
            0, 
            wgpuBufferGetSize(perDrawInstancingDatas[curPerDrawInstancingData])
        );
        curPerDrawInstancingData += 1;

        if(mesh.wgData.indexBuffer != nullptr)
            wgpuRenderPassEncoderSetIndexBuffer(renderPass, mesh.wgData.indexBuffer, WGPUIndexFormat_Uint32, 0, wgpuBufferGetSize(mesh.wgData.indexBuffer));

        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, lastMat->wgData.mainBindGroup, 0, nullptr);
        wgpuRenderPassEncoderSetBindGroup(renderPass, 2, cameraBindGroup, 0, nullptr);
        //}
        lastMesh = &mesh;
    
        wgpuRenderPassEncoderSetBindGroup(renderPass, 1, perDrawDatas[0].bindGroup, 0, nullptr);
    
        if(mesh.wgData.indexBuffer != nullptr){
            wgpuRenderPassEncoderDrawIndexed(renderPass, mesh.indiceCount, inCount, 0, 0, 0);
        } else {
            wgpuRenderPassEncoderDraw(renderPass, mesh.vertexCount, inCount, 0, 0);
        }
    };

    int drawCounts = math::ceil(count / 1000);
    int total = count;
    int offset = 0;
    for(int i = 0; i < drawCounts; i++){
        int toDrawCount = 0;
        if(total - 1000 > 0){
            total -= 1000;
            toDrawCount = 1000;
        } else {
            toDrawCount = total;
        }

        Draw(modelMatrixs + offset, toDrawCount);
        offset += toDrawCount;
    }
}

void WebGPUGraphicsDevice::DrawMesh(Mesh& mesh, Material& mat, Matrix4 modelMatrix, OD::PerDrawData* perDrawData){
    BindMaterial(mat);
    DrawMesh(mesh, modelMatrix, perDrawData);
}

void WebGPUGraphicsDevice::DrawMeshSkinned(Mesh& mesh, Material& mat, Matrix4 model, Matrix4* animMatrix, int count, OD::PerDrawData* perDrawData){
    BindMaterial(mat);
    DrawMeshSkinned(mesh, model, animMatrix, count, perDrawData);
}

void WebGPUGraphicsDevice::DrawMeshInstancing(Mesh& mesh, Material& mat, Matrix4* animMatrixs, int count){
    BindMaterial(mat);
    DrawMeshInstancing(mesh, animMatrixs, count);
}

void WebGPUGraphicsDevice::DrawModel(Model& model, Matrix4 modelMatrix){
    int index = 0;
    for(auto i: model.renderTargets){
        Ref<Material> targetMaterial = model.materials[i.materialIndex];
        Ref<Mesh> targetMesh = model.meshs[i.meshIndex];
        Matrix4 targetMatrix =  modelMatrix * model.skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex);
        BindMaterial(*targetMaterial);
        DrawMesh(*targetMesh, targetMatrix, nullptr);
    }
}

void WebGPUGraphicsDevice::AddDrawLineCommand(Vector3 start, Vector3 end){

}

void WebGPUGraphicsDevice::DrawLinesComamnd(Vector3 color, int lineWidth){

}

void WebGPUGraphicsDevice::DrawLine(Vector3 start, Vector3 end, Vector3 color, int lineWidth){

}

void WebGPUGraphicsDevice::DrawLine(Matrix4 model, Vector3 start, Vector3 end, Vector3 color, int lineWidth){

}

void WebGPUGraphicsDevice::DrawWireCube(Matrix4 modelMatrix, Vector3 color, int lineWidth){

}

void WebGPUGraphicsDevice::DrawFullScreenQuad(Material& mat, Matrix4 modelMatrix){

}

void WebGPUGraphicsDevice::DrawQuadPostProcessing(Framebuffer* src, Framebuffer* dst, Material& shader, int pass){

}

void WebGPUGraphicsDevice::DrawQuadPostProcessing(Framebuffer* dst, Material& shader, int pass){

}

void WebGPUGraphicsDevice::BlitFramebuffer(Framebuffer* src, Framebuffer* dst, int srcPass){

}

bool WebGPUGraphicsDevice::MeshCreateOrSubmit(
    Mesh& mesh,
    std::vector<unsigned int>* indices,
    std::vector<Vector3>* vertices,
    std::vector<Vector3>* uv,
    std::vector<Vector3>* normals,
    std::vector<Vector4>* colors,
    std::vector<Vector3>* tangents,
    std::vector<Vector4>* weights,
    std::vector<IVector4>* influences
){
    MeshDestroy(mesh);

    auto CreateBuffer = [&](size_t size, const char* label){
        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.nextInChain = nullptr;
        bufferDesc.label = label;
        bufferDesc.size = size;
        bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex; // Vertex usage here!
        bufferDesc.mappedAtCreation = false;
        WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
        return buffer;
    };

    auto CreateIndexBuffer = [&](size_t size, const char* label){
        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.nextInChain = nullptr;
        bufferDesc.label = label;
        bufferDesc.size = size;
        bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
        bufferDesc.mappedAtCreation = false;
        WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
        return buffer;
    };

    mesh.wgData.vertexBuffer = CreateBuffer(sizeof(Vector3) * vertices->size(), "VertexBuffer");
    wgpuQueueWriteBuffer(queue, mesh.wgData.vertexBuffer, 0, vertices->data(), sizeof(Vector3) * vertices->size());
    mesh.vertexCount = vertices->size();

    if(indices != nullptr && indices->size() > 0){
        mesh.wgData.indexBuffer = CreateIndexBuffer(sizeof(unsigned int) * indices->size(), "IndexBuffer");
        wgpuQueueWriteBuffer(queue, mesh.wgData.indexBuffer, 0, indices->data(), sizeof(unsigned int) * indices->size());
        mesh.indiceCount = indices->size();
    }

    size_t targetUvSize = uv == nullptr || uv->size() <= 0 ? sizeof(Vector3) : sizeof(Vector3) * uv->size();
    mesh.wgData.uvBuffer = CreateBuffer(targetUvSize, "UvBuffer");
    if(uv != nullptr) wgpuQueueWriteBuffer(queue, mesh.wgData.uvBuffer, 0, uv->data(), sizeof(Vector3) * uv->size());

    size_t targetNormalSize = normals == nullptr || normals->size() <= 0 ? sizeof(Vector3) : sizeof(Vector3) * normals->size();
    mesh.wgData.normalBuffer = CreateBuffer(targetNormalSize, "NormalBuffer");
    if(normals != nullptr) wgpuQueueWriteBuffer(queue, mesh.wgData.normalBuffer, 0, normals->data(), sizeof(Vector3) * normals->size());

    size_t colorTargetSize = colors == nullptr || colors->size() <= 0 ? sizeof(Vector4) : sizeof(Vector4) * colors->size();
    mesh.wgData.colorBuffer = CreateBuffer(colorTargetSize, "ColorBuffer");
    if(colors != nullptr) wgpuQueueWriteBuffer(queue, mesh.wgData.colorBuffer, 0, colors->data(), sizeof(Vector4) * colors->size());

    size_t tangentTargetSize = tangents == nullptr || tangents->size() <= 0 ? sizeof(Vector3) : sizeof(Vector3) * tangents->size();
    mesh.wgData.tangentBuffer = CreateBuffer(tangentTargetSize, "TangentBuffer");
    if(tangents != nullptr) wgpuQueueWriteBuffer(queue, mesh.wgData.tangentBuffer, 0, tangents->data(), sizeof(Vector3) * tangents->size());

    size_t jointTargetSize = influences == nullptr || influences->size() <= 0 ? sizeof(IVector4) : sizeof(IVector4) * influences->size();
    mesh.wgData.jointBuffer = CreateBuffer(jointTargetSize, "JointBuffer");
    if(influences != nullptr) wgpuQueueWriteBuffer(queue, mesh.wgData.jointBuffer, 0, influences->data(), sizeof(IVector4) * influences->size());

    size_t weightTargetSize = weights == nullptr || weights->size() <= 0 ? sizeof(Vector4) : sizeof(Vector4) * weights->size();
    mesh.wgData.weightsBuffer = CreateBuffer(weightTargetSize, "WeightsBuffer");
    if(weights != nullptr) wgpuQueueWriteBuffer(queue, mesh.wgData.weightsBuffer, 0, weights->data(), sizeof(Vector4) * weights->size());

    return true;
}

void WebGPUGraphicsDevice::MeshSubmitInstancingModelMatrixs(Mesh& mesh){

}

void WebGPUGraphicsDevice::MeshSubmitInstancingCustomModelMatrixs(Mesh& mesh, Matrix4* modelMatrixs, int count){

}

void WebGPUGraphicsDevice::MeshDestroy(Mesh& mesh){
    if(mesh.wgData.vertexBuffer != nullptr) wgpuBufferRelease(mesh.wgData.vertexBuffer);
    if(mesh.wgData.uvBuffer != nullptr) wgpuBufferRelease(mesh.wgData.uvBuffer);
    if(mesh.wgData.normalBuffer != nullptr) wgpuBufferRelease(mesh.wgData.normalBuffer);
    if(mesh.wgData.indexBuffer != nullptr) wgpuBufferRelease(mesh.wgData.indexBuffer);

    mesh.wgData.vertexBuffer = nullptr;
    mesh.wgData.uvBuffer = nullptr;
    mesh.wgData.normalBuffer = nullptr;
    mesh.wgData.indexBuffer = nullptr;
}

bool WebGPUGraphicsDevice::MeshIsValid(Mesh& mesh){
    return false;
}

void WebGPUGraphicsDevice::BeginFramebuffer(Framebuffer& frambuffer, Vector4 clearColor, int layer, int mip){
    lastMat = nullptr;
    lastShader = nullptr;
    lastMesh = nullptr;
    currentRendePassTarget = (int)frambuffer.type;

    WGPURenderPassDescriptor renderPassDesc = {};
	renderPassDesc.nextInChain = nullptr;

    std::vector<WGPURenderPassColorAttachment> renderPassColorAttachment(renderPasses[currentRendePassTarget].colorAttachments.size());
    WGPURenderPassDepthStencilAttachment depthStencilAttachment = {};

    for(int i = 0; i < renderPassColorAttachment.size(); i++){
        renderPassColorAttachment[i] = {};
        renderPassColorAttachment[i].nextInChain = nullptr;
        renderPassColorAttachment[i].view = frambuffer.wgData.textureView;// targetView;
        renderPassColorAttachment[i].resolveTarget = nullptr;
        renderPassColorAttachment[i].loadOp = WGPULoadOp_Clear;
        renderPassColorAttachment[i].storeOp = WGPUStoreOp_Store;
        renderPassColorAttachment[i].clearValue = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };
    #ifndef WEBGPU_BACKEND_WGPU
        renderPassColorAttachment[i].depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    #endif // NOT WEBGPU_BACKEND_WGPU
    }

    depthStencilAttachment = {};
    depthStencilAttachment.view = frambuffer.wgData.depthTextureView;// depthTextureView;
    depthStencilAttachment.depthClearValue = 1.0f;// The initial value of the depth buffer, meaning "far"
    depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;// Operation settings comparable to the color attachment
    depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
    depthStencilAttachment.depthReadOnly = false;// we could turn off writing to the depth buffer globally here
    depthStencilAttachment.stencilClearValue = 0;// Stencil setup, mandatory but unused
#ifdef WEBGPU_BACKEND_WGPU
    depthStencilAttachment.stencilLoadOp = WGPULoadOp_Clear;
    depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Store;
#else
    depthStencilAttachment.stencilLoadOp = WGPULoadOp_Undefined;
    depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Undefined;
#endif
    depthStencilAttachment.stencilReadOnly = true;

	renderPassDesc.colorAttachmentCount = renderPassColorAttachment.size();
	renderPassDesc.colorAttachments = renderPassColorAttachment.data();
	renderPassDesc.depthStencilAttachment = &depthStencilAttachment;
	renderPassDesc.timestampWrites = nullptr;
    renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
}

void WebGPUGraphicsDevice::EndFramebuffer(){
    wgpuRenderPassEncoderEnd(renderPass);
	//wgpuRenderPassEncoderRelease(renderPass);
}

bool WebGPUGraphicsDevice::FramebufferCreate(Framebuffer& frambuffer, FrameBufferSpecification specification){
    FramebufferDestroy(frambuffer);

    WGPUTextureDescriptor textureDesc = {};
    textureDesc.nextInChain = nullptr;
    textureDesc.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc | WGPUTextureUsage_TextureBinding;
    textureDesc.dimension = WGPUTextureDimension_2D;
    textureDesc.size = { (uint32_t)specification.width, (uint32_t)specification.height, 1 };
    textureDesc.format = surfaceFormat; //WGPUTextureFormat_BGRA8Unorm;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = 1;
    textureDesc.dimension = WGPUTextureDimension_2D;
    frambuffer.wgData.texture = wgpuDeviceCreateTexture(device, &textureDesc);
    WGPUTextureViewDescriptor viewDescriptor = {};
	viewDescriptor.nextInChain = nullptr;
	viewDescriptor.format = surfaceFormat;// wgpuTextureGetFormat(surfaceTexture.texture);
	viewDescriptor.dimension = WGPUTextureViewDimension_2D;
	viewDescriptor.baseMipLevel = 0;
	viewDescriptor.mipLevelCount = 1;
	viewDescriptor.baseArrayLayer = 0;
	viewDescriptor.arrayLayerCount = 1;
	viewDescriptor.aspect = WGPUTextureAspect_All;
	frambuffer.wgData.textureView = wgpuTextureCreateView(frambuffer.wgData.texture, &viewDescriptor);

    // Create texture for depth attachment
    WGPUTextureDescriptor depthTextureDesc = {};
    depthTextureDesc.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc | WGPUTextureUsage_TextureBinding;// WGPUTextureUsage_Sampled;
    depthTextureDesc.dimension = WGPUTextureDimension_2D;
    depthTextureDesc.size = { (uint32_t)specification.width, (uint32_t)specification.height, 1 };
    depthTextureDesc.format = depthTextureFormat;// WGPUTextureFormat_Depth32Float;
    depthTextureDesc.mipLevelCount = 1;
    depthTextureDesc.sampleCount = 1;
    depthTextureDesc.dimension = WGPUTextureDimension_2D;
    frambuffer.wgData.depthTexture = wgpuDeviceCreateTexture(device, &depthTextureDesc);
    WGPUTextureViewDescriptor depthTextureViewDesc = {};
    depthTextureViewDesc.nextInChain = nullptr;
    depthTextureViewDesc.aspect = WGPUTextureAspect_DepthOnly;
    depthTextureViewDesc.baseArrayLayer = 0;
    depthTextureViewDesc.arrayLayerCount = 1;
    depthTextureViewDesc.baseMipLevel = 0;
    depthTextureViewDesc.mipLevelCount = 1;
    depthTextureViewDesc.dimension = WGPUTextureViewDimension_2D;
    depthTextureViewDesc.format = depthTextureFormat;
    frambuffer.wgData.depthTextureView = wgpuTextureCreateView(depthTexture, &depthTextureViewDesc);

    WGPUSamplerDescriptor samplerDesc = {};
    samplerDesc.nextInChain = nullptr;
    samplerDesc.addressModeU = WGPUAddressMode_Repeat;
    samplerDesc.addressModeV = WGPUAddressMode_Repeat;
    samplerDesc.addressModeW = WGPUAddressMode_Repeat;
    samplerDesc.magFilter = WGPUFilterMode_Linear;
    samplerDesc.minFilter = WGPUFilterMode_Linear;
    samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Linear;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 8.0f;
    samplerDesc.compare = WGPUCompareFunction_Undefined;
    samplerDesc.maxAnisotropy = 1;
    frambuffer.wgData.sampler = wgpuDeviceCreateSampler(device, &samplerDesc);

    return true;
}

void WebGPUGraphicsDevice::FramebufferDestroy(Framebuffer& frambuffer){
    if(frambuffer.wgData.depthTextureView != nullptr) wgpuTextureViewRelease(frambuffer.wgData.depthTextureView);
    if(frambuffer.wgData.depthTexture != nullptr) wgpuTextureDestroy(frambuffer.wgData.depthTexture);
    if(frambuffer.wgData.textureView != nullptr) wgpuTextureViewRelease(frambuffer.wgData.textureView);
    if(frambuffer.wgData.texture != nullptr) wgpuTextureDestroy(frambuffer.wgData.texture);

    frambuffer.wgData.depthTextureView = nullptr;
    frambuffer.wgData.depthTexture = nullptr;
    frambuffer.wgData.textureView = nullptr;
    frambuffer.wgData.texture = nullptr;
}

bool WebGPUGraphicsDevice::FramebufferIsValid(Framebuffer& frambuffer){
    return false;
}

void* WebGPUGraphicsDevice::FramebufferColorAttachmentId(Framebuffer& framebuffer, int index){
    return nullptr;
}

void* WebGPUGraphicsDevice::FramebufferDepthAttachmentId(Framebuffer& framebuffer){
    return nullptr;
}

int WebGPUGraphicsDevice::FramebufferReadPixel(Framebuffer& frambuffer, int attachmentIndex, int x, int y){
    return 0;
}

void WebGPUGraphicsDevice::WriteMipMaps(
    WGPUDevice device,
    WGPUTexture texture,
    WGPUExtent3D textureSize,
    [[maybe_unused]] uint32_t mipLevelCount, // not used yet
    const unsigned char* pixelData
){
    WGPUQueue queue = wgpuDeviceGetQueue(device);

    // Arguments telling which part of the texture we upload to
    WGPUImageCopyTexture destination = {};
    destination.texture = texture;
    destination.origin = { 0, 0, 0 };
    destination.aspect = WGPUTextureAspect_All;

    // Arguments telling how the C++ side pixel memory is laid out
    WGPUTextureDataLayout source = {};
    source.offset = 0;

    // Create image data
    WGPUExtent3D mipLevelSize = textureSize;
    std::vector<unsigned char> previousLevelPixels;
    WGPUExtent3D previousMipLevelSize;
    for(uint32_t level = 0; level < mipLevelCount; ++level){
        // Pixel data for the current level
        std::vector<unsigned char> pixels(4 * mipLevelSize.width * mipLevelSize.height);
        if(level == 0){
            // We cannot really avoid this copy since we need this
            // in previousLevelPixels at the next iteration
            memcpy(pixels.data(), pixelData, pixels.size());
        } else {
            // Create mip level data
            for(uint32_t i = 0; i < mipLevelSize.width; ++i){
                for(uint32_t j = 0; j < mipLevelSize.height; ++j){
                    unsigned char* p = &pixels[4 * (j * mipLevelSize.width + i)];
                    // Get the corresponding 4 pixels from the previous level
                    unsigned char* p00 = &previousLevelPixels[4 * ((2 * j + 0) * previousMipLevelSize.width + (2 * i + 0))];
                    unsigned char* p01 = &previousLevelPixels[4 * ((2 * j + 0) * previousMipLevelSize.width + (2 * i + 1))];
                    unsigned char* p10 = &previousLevelPixels[4 * ((2 * j + 1) * previousMipLevelSize.width + (2 * i + 0))];
                    unsigned char* p11 = &previousLevelPixels[4 * ((2 * j + 1) * previousMipLevelSize.width + (2 * i + 1))];
                    // Average
                    p[0] = (p00[0] + p01[0] + p10[0] + p11[0]) / 4;
                    p[1] = (p00[1] + p01[1] + p10[1] + p11[1]) / 4;
                    p[2] = (p00[2] + p01[2] + p10[2] + p11[2]) / 4;
                    p[3] = (p00[3] + p01[3] + p10[3] + p11[3]) / 4;
                }
            }
        }

        // Upload data to the GPU texture
        destination.mipLevel = level;
        source.bytesPerRow = 4 * mipLevelSize.width;
        source.rowsPerImage = mipLevelSize.height;
        wgpuQueueWriteTexture(queue, &destination, pixels.data(), pixels.size(), &source, &mipLevelSize);

        previousLevelPixels = std::move(pixels);
        previousMipLevelSize = mipLevelSize;
        mipLevelSize.width /= 2;
        mipLevelSize.height /= 2;
    }

    wgpuQueueRelease(queue);
}

// Equivalent of std::bit_width that is available from C++20 onward
uint32_t bit_width(uint32_t m) {
    if (m == 0) return 0;
    else { uint32_t w = 0; while (m >>= 1) ++w; return w; }
}

bool WebGPUGraphicsDevice::Texture2DCreate(Texture2D& tex, const std::string path, Texture2DSetting settings){
    Texture2DDestroy(tex);

    int width, height, channels;
    unsigned char* pixelData = stbi_load(path.c_str(), &width, &height, &channels, 4 /* force 4 channels */);
    if(pixelData == nullptr){
        LogError("Cannot load file image {}\nSTB Reason: {}\n", tex.path, stbi_failure_reason());
        return false;
    }

    WGPUTextureDescriptor textureDesc = {};
    textureDesc.nextInChain = nullptr;
    textureDesc.dimension = WGPUTextureDimension_2D;
    textureDesc.format = WGPUTextureFormat_RGBA8Unorm; // by convention for bmp, png and jpg file. Be careful with other formats.
    textureDesc.sampleCount = 1;
    textureDesc.size = { (unsigned int)width, (unsigned int)height, 1 };
    textureDesc.mipLevelCount = bit_width(std::max(textureDesc.size.width, textureDesc.size.height));
    textureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    textureDesc.viewFormatCount = 0;
    textureDesc.viewFormats = nullptr;
    tex.wgData.texture = wgpuDeviceCreateTexture(device, &textureDesc);

    // Upload data to the GPU texture (to be implemented!)
    WriteMipMaps(device, tex.wgData.texture, textureDesc.size, textureDesc.mipLevelCount, pixelData);
    stbi_image_free(pixelData);

    WGPUTextureViewDescriptor textureViewDesc = {};
    textureViewDesc.nextInChain = nullptr;
    textureViewDesc.aspect = WGPUTextureAspect_All;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 1;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.mipLevelCount = textureDesc.mipLevelCount;
    textureViewDesc.dimension = WGPUTextureViewDimension_2D;
    textureViewDesc.format = textureDesc.format;
    tex.wgData.textureView = wgpuTextureCreateView(tex.wgData.texture, &textureViewDesc);

    WGPUSamplerDescriptor samplerDesc = {};
    samplerDesc.nextInChain = nullptr;
    samplerDesc.addressModeU = WGPUAddressMode_Repeat;
    samplerDesc.addressModeV = WGPUAddressMode_Repeat;
    samplerDesc.addressModeW = WGPUAddressMode_Repeat;
    samplerDesc.magFilter = WGPUFilterMode_Linear;
    samplerDesc.minFilter = WGPUFilterMode_Linear;
    samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Linear;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 8.0f;
    samplerDesc.compare = WGPUCompareFunction_Undefined;
    samplerDesc.maxAnisotropy = 1;
    tex.wgData.sampler = wgpuDeviceCreateSampler(device, &samplerDesc);

    
    return true;
    
    
    /*WGPUTextureDescriptor textureDesc;
    textureDesc.nextInChain = nullptr;
    textureDesc.dimension = WGPUTextureDimension_2D;
    textureDesc.size = { 256, 256, 1 };
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = 1;
    textureDesc.format = WGPUTextureFormat_RGBA8Unorm;
    textureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    textureDesc.viewFormatCount = 0;
    textureDesc.viewFormats = nullptr;
    tex.wgData.texture = wgpuDeviceCreateTexture(device, &textureDesc);

    // Create image data
    std::vector<uint8_t> pixels(4 * textureDesc.size.width * textureDesc.size.height);
    for(uint32_t i = 0; i < textureDesc.size.width; ++i){
        for(uint32_t j = 0; j < textureDesc.size.height; ++j){
            uint8_t *p = &pixels[4 * (j * textureDesc.size.width + i)];
            p[0] = (i / 16) % 2 == (j / 16) % 2 ? 255 : 0; // r
            p[1] = ((i - j) / 16) % 2 == 0 ? 255 : 0; // g
            p[2] = ((i + j) / 16) % 2 == 0 ? 255 : 0; // b
            p[3] = 255; // a
        }
    }

    // Arguments telling which part of the texture we upload to
    // (together with the last argument of writeTexture)
    WGPUImageCopyTexture destination;
    destination.nextInChain = nullptr;
    destination.texture = tex.wgData.texture ;
    destination.mipLevel = 0;
    destination.origin = { 0, 0, 0 }; // equivalent of the offset argument of Queue::writeBuffer
    destination.aspect = WGPUTextureAspect_All; // only relevant for depth/Stencil textures

    // Arguments telling how the C++ side pixel memory is laid out
    WGPUTextureDataLayout source;
    source.nextInChain = nullptr;
    source.offset = 0;
    source.bytesPerRow = 4 * textureDesc.size.width;
    source.rowsPerImage = textureDesc.size.height;

    wgpuQueueWriteTexture(queue, &destination, pixels.data(), pixels.size(), &source, &textureDesc.size);

    WGPUTextureViewDescriptor textureViewDesc;
    textureViewDesc.nextInChain = nullptr;
    textureViewDesc.label = "TextureViwer";
    textureViewDesc.aspect = WGPUTextureAspect_All;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 1;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.mipLevelCount = 1;
    textureViewDesc.dimension = WGPUTextureViewDimension_2D;
    textureViewDesc.format = textureDesc.format;
    tex.wgData.textureView = wgpuTextureCreateView(tex.wgData.texture, &textureViewDesc);

    WGPUSamplerDescriptor samplerDesc;
    samplerDesc.nextInChain = nullptr;
    samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
    samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
    samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;
    samplerDesc.magFilter = WGPUFilterMode_Linear;
    samplerDesc.minFilter = WGPUFilterMode_Linear;
    samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Linear;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 1.0f;
    samplerDesc.compare = WGPUCompareFunction_Undefined;
    samplerDesc.maxAnisotropy = 1;
    tex.wgData.sampler = wgpuDeviceCreateSampler(device, &samplerDesc);
    
    return true;*/
}

bool WebGPUGraphicsDevice::Texture2DCreate(Texture2D& tex, void* data, size_t size, Texture2DSetting settings){
    Texture2DDestroy(tex);

    int width, height, channels;
    unsigned char* pixelData = stbi_load_from_memory((const stbi_uc*)data, size, &width, &height, &channels, 4);
    if(pixelData == nullptr){
        LogError("Cannot load file image {}\nSTB Reason: {}\n", tex.path, stbi_failure_reason());
        return false;
    }

    WGPUTextureDescriptor textureDesc = {};
    textureDesc.nextInChain = nullptr;
    textureDesc.dimension = WGPUTextureDimension_2D;
    textureDesc.format = WGPUTextureFormat_RGBA8Unorm; // by convention for bmp, png and jpg file. Be careful with other formats.
    textureDesc.sampleCount = 1;
    textureDesc.size = { (unsigned int)width, (unsigned int)height, 1 };
    textureDesc.mipLevelCount = bit_width(std::max(textureDesc.size.width, textureDesc.size.height));
    textureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    textureDesc.viewFormatCount = 0;
    textureDesc.viewFormats = nullptr;
    tex.wgData.texture = wgpuDeviceCreateTexture(device, &textureDesc);

    // Upload data to the GPU texture (to be implemented!)
    WriteMipMaps(device, tex.wgData.texture, textureDesc.size, textureDesc.mipLevelCount, pixelData);
    stbi_image_free(pixelData);

    WGPUTextureViewDescriptor textureViewDesc = {};
    textureViewDesc.nextInChain = nullptr;
    textureViewDesc.aspect = WGPUTextureAspect_All;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 1;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.mipLevelCount = textureDesc.mipLevelCount;
    textureViewDesc.dimension = WGPUTextureViewDimension_2D;
    textureViewDesc.format = textureDesc.format;
    tex.wgData.textureView = wgpuTextureCreateView(tex.wgData.texture, &textureViewDesc);

    WGPUSamplerDescriptor samplerDesc = {};
    samplerDesc.nextInChain = nullptr;
    samplerDesc.addressModeU = WGPUAddressMode_Repeat;
    samplerDesc.addressModeV = WGPUAddressMode_Repeat;
    samplerDesc.addressModeW = WGPUAddressMode_Repeat;
    samplerDesc.magFilter = WGPUFilterMode_Linear;
    samplerDesc.minFilter = WGPUFilterMode_Linear;
    samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Linear;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 8.0f;
    samplerDesc.compare = WGPUCompareFunction_Undefined;
    samplerDesc.maxAnisotropy = 1;
    tex.wgData.sampler = wgpuDeviceCreateSampler(device, &samplerDesc);

    return true;
}

bool WebGPUGraphicsDevice::Texture2DCreate(Texture2D& tex, void* data, size_t size, int width, int height, TextureDataType dataType, Texture2DSetting settings){
    return false;
}

void WebGPUGraphicsDevice::Texture2DDestroy(Texture2D& tex){
    if(tex.wgData.texture != nullptr){
        wgpuTextureDestroy(tex.wgData.texture);
        wgpuTextureRelease(tex.wgData.texture);
    }

    tex.wgData.texture = nullptr;
    tex.wgData.textureView = nullptr;
    tex.wgData.sampler = nullptr;
}

bool WebGPUGraphicsDevice::Texture2DIsValid(Texture2D& tex){
    return tex.wgData.texture != nullptr;
}

void* WebGPUGraphicsDevice::Texture2DRenderId(Texture2D& tex){
    return nullptr;
}

bool WebGPUGraphicsDevice::Texture2DArrayCreate(Texture2DArray& tex, const std::vector<std::string>& filePaths){
    return false;
}

void WebGPUGraphicsDevice::Texture2DArrayDestroy(Texture2DArray& tex){

}

bool WebGPUGraphicsDevice::Texture2DArrayIsValid(Texture2DArray& tex){
    return false;
}

bool WebGPUGraphicsDevice::CubemapCreateFromFile(
    Cubemap& cubemap,
    const char* right, const char* left, const char* top,
    const char* bottom, const char* front, const char* back
){
    return false;
}

void WebGPUGraphicsDevice::CubemapDestroy(Cubemap& cubemap){

}

bool WebGPUGraphicsDevice::CubemapIsValid(Cubemap& tex){
    return false;
}

using ShaderSpiv = std::vector<uint32_t>;

bool CompileShader(std::string& baseSource, glslang_stage_t Stage, ShaderSpiv& spiv){
	glslang_input_t input = {};
	input.language = GLSLANG_SOURCE_GLSL;
	input.stage = Stage;
	input.client = GLSLANG_CLIENT_VULKAN;
	input.client_version = GLSLANG_TARGET_VULKAN_1_1;
	input.target_language = GLSLANG_TARGET_SPV;
	input.target_language_version = GLSLANG_TARGET_SPV_1_0;
	input.code = baseSource.c_str();
	input.default_version = 450;
	input.default_profile = GLSLANG_NO_PROFILE;
	input.force_default_version_and_profile = false;
	input.forward_compatible = false;
	input.messages = (glslang_messages_t)(GLSLANG_MSG_SPV_RULES_BIT);
	input.resource = glslang_default_resource();

	glslang_shader_t* shader = glslang_shader_create(&input);

	if(!glslang_shader_preprocess(shader, &input)){
		fprintf(stderr, "GLSL preprocessing failed\n");
		fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
		fprintf(stderr, "\n%s", glslang_shader_get_info_debug_log(shader));
        
		//PrintShaderSource(input.code);
		return 0;
	}

	if(!glslang_shader_parse(shader, &input)){
		fprintf(stderr, "GLSL parsing failed\n");
		fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
		fprintf(stderr, "\n%s", glslang_shader_get_info_debug_log(shader));
		//PrintShaderSource(glslang_shader_get_preprocessed_code(shader));
		return 0;
	}

	glslang_program_t* program = glslang_program_create();
	glslang_program_add_shader(program, shader);

	if(!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
		fprintf(stderr, "GLSL linking failed\n");
		fprintf(stderr, "\n%s", glslang_program_get_info_log(program));
		fprintf(stderr, "\n%s", glslang_program_get_info_debug_log(program));
		return 0;
	}

	glslang_program_SPIRV_generate(program, Stage);
    size_t program_size = glslang_program_SPIRV_get_size(program);
    spiv.resize(program_size);
    glslang_program_SPIRV_get(program, spiv.data());

	const char* spirv_messages = glslang_program_SPIRV_get_messages(program);

	if(spirv_messages){
		fprintf(stderr, "SPIR-V message: '%s'", spirv_messages);
	}

	glslang_program_delete(program);
	glslang_shader_delete(shader);

	bool ret = spiv.size() > 0;
	return ret;
}

WGPURenderPipeline WebGPUGraphicsDevice::CreatePipeline(SubShader& shader, std::vector<std::string>& keyworlds, ShaderPipeline pipeline, int targetRenderPass){
    auto ContainKey = [&](std::string key){
        return std::find(keyworlds.begin(), keyworlds.end(), key) != keyworlds.end(); 
    };

    WGPURenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.nextInChain = nullptr;

    std::vector<WGPUVertexBufferLayout> vertexBufferLayouts(5);

    WGPUVertexAttribute positionAttrib = {};
    positionAttrib.shaderLocation = 0;
    positionAttrib.format = WGPUVertexFormat_Float32x3;// Means vec3f in the shader
    positionAttrib.offset = 0;// Index of the first element
    vertexBufferLayouts[0].arrayStride = 3 * sizeof(float);
    vertexBufferLayouts[0].stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayouts[0].attributeCount = 1;
    vertexBufferLayouts[0].attributes = &positionAttrib;

    WGPUVertexAttribute uvAttrib = {};
    uvAttrib.shaderLocation = 1;
    uvAttrib.format = WGPUVertexFormat_Float32x3;// Means vec3f in the shader
    uvAttrib.offset = 0;// Index of the first element
    vertexBufferLayouts[1].arrayStride = 3 * sizeof(float);
    vertexBufferLayouts[1].stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayouts[1].attributeCount = 1;
    vertexBufferLayouts[1].attributes = &uvAttrib;

    WGPUVertexAttribute normalAttrib = {};
    normalAttrib.shaderLocation = 2;
    normalAttrib.format = WGPUVertexFormat_Float32x3;// Means vec3f in the shader
    normalAttrib.offset = 0;// Index of the first element
    vertexBufferLayouts[2].arrayStride = 3 * sizeof(float);
    vertexBufferLayouts[2].stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayouts[2].attributeCount = 1;
    vertexBufferLayouts[2].attributes = &normalAttrib;

    WGPUVertexAttribute colorAttrib = {};
    colorAttrib.shaderLocation = 3;
    colorAttrib.format = WGPUVertexFormat_Float32x4;// Means vec3f in the shader
    colorAttrib.offset = 0;// Index of the first element
    vertexBufferLayouts[3].arrayStride = 4 * sizeof(float);
    vertexBufferLayouts[3].stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayouts[3].attributeCount = 1;
    vertexBufferLayouts[3].attributes = &colorAttrib;

    WGPUVertexAttribute tangentAttrib = {};
    tangentAttrib.shaderLocation = 4;
    tangentAttrib.format = WGPUVertexFormat_Float32x3;// Means vec3f in the shader
    tangentAttrib.offset = 0;// Index of the first element
    vertexBufferLayouts[4].arrayStride = 3 * sizeof(float);
    vertexBufferLayouts[4].stepMode = WGPUVertexStepMode_Vertex;
    vertexBufferLayouts[4].attributeCount = 1;
    vertexBufferLayouts[4].attributes = &tangentAttrib;

    WGPUVertexAttribute bondeIdsAttrib = {};
    bondeIdsAttrib.shaderLocation = 5;
    bondeIdsAttrib.format = WGPUVertexFormat_Sint32x4;// Means vec3f in the shader
    bondeIdsAttrib.offset = 0;// Index of the first element
    WGPUVertexAttribute weightsAttrib;
    weightsAttrib.shaderLocation = 6;
    weightsAttrib.format = WGPUVertexFormat_Float32x4;// Means vec3f in the shader
    weightsAttrib.offset = 0;// Index of the first element

    std::vector<WGPUVertexAttribute> instancingAttrib(4);
    instancingAttrib[0].shaderLocation = 5;
    instancingAttrib[0].format = WGPUVertexFormat_Float32x4;
    instancingAttrib[0].offset = 0;
    instancingAttrib[1].shaderLocation = 6;
    instancingAttrib[1].format = WGPUVertexFormat_Float32x4;
    instancingAttrib[1].offset = 16;
    instancingAttrib[2].shaderLocation = 7;
    instancingAttrib[2].format = WGPUVertexFormat_Float32x4;
    instancingAttrib[2].offset = 32;
    instancingAttrib[3].shaderLocation = 8;
    instancingAttrib[3].format = WGPUVertexFormat_Float32x4;
    instancingAttrib[3].offset = 48;

    if(ContainKey("INSTANCING")){
        WGPUVertexBufferLayout instancinglayout = {};
        instancinglayout.arrayStride = sizeof(float) * 16;
        instancinglayout.stepMode = WGPUVertexStepMode_Instance; //WGPUVertexStepMode_Vertex;
        instancinglayout.attributeCount = 4;
        instancinglayout.attributes = instancingAttrib.data();
        vertexBufferLayouts.emplace_back(instancinglayout);
    }

    if(ContainKey("SKINNED")){
        WGPUVertexBufferLayout boneIdsLayout = {};
        boneIdsLayout.arrayStride = 4 * sizeof(int);
        boneIdsLayout.stepMode = WGPUVertexStepMode_Vertex;
        boneIdsLayout.attributeCount = 1;
        boneIdsLayout.attributes = &bondeIdsAttrib;
        vertexBufferLayouts.emplace_back(boneIdsLayout);

        WGPUVertexBufferLayout weightsLayout = {};
        weightsLayout.arrayStride = 4 * sizeof(float);
        weightsLayout.stepMode = WGPUVertexStepMode_Vertex;
        weightsLayout.attributeCount = 1;
        weightsLayout.attributes = &weightsAttrib;
        vertexBufferLayouts.emplace_back(weightsLayout);
    }

    pipelineDesc.vertex.nextInChain = nullptr;
    pipelineDesc.vertex.bufferCount = vertexBufferLayouts.size(); //pipelineDesc.vertex.bufferCount = 0;
    pipelineDesc.vertex.buffers = vertexBufferLayouts.data();// pipelineDesc.vertex.buffers = nullptr;
    pipelineDesc.vertex.module = shader.wgData.shaderModuleVertex;
    pipelineDesc.vertex.entryPoint = "main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;
    // Each sequence of 3 vertices is considered as a triangle
    pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    // We'll see later how to specify the order in which vertices should be
    // connected. When not specified, vertices are considered sequentially.
    pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
    // The face orientation is defined by assuming that when looking
    // from the front of the face, its corner vertices are enumerated
    // in the counter-clockwise (CCW) order.
    pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
    // But the face orientation does not matter much because we do not
    // cull (i.e. "hide") the faces pointing away from us (which is often
    // used for optimization).
    pipelineDesc.primitive.cullMode = WGPUCullMode_None;

    WGPUFragmentState fragmentState = {};
    fragmentState.nextInChain = nullptr;
    fragmentState.module = shader.wgData.shaderModuleFrag;
    fragmentState.entryPoint = "main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;

    WGPUBlendState blendState = {};
    blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blendState.color.operation = WGPUBlendOperation_Add;
    blendState.alpha.srcFactor = WGPUBlendFactor_Zero;
    blendState.alpha.dstFactor = WGPUBlendFactor_One;
    blendState.alpha.operation = WGPUBlendOperation_Add;

    std::vector<WGPUColorTargetState> colorTarget(renderPasses[targetRenderPass].colorAttachments.size());
    for(int i = 0; i < renderPasses[targetRenderPass].colorAttachments.size(); i++){
        colorTarget[i] = {};
        colorTarget[i].nextInChain = nullptr;
        colorTarget[i].format = renderPasses[targetRenderPass].colorAttachments[i].format;
        colorTarget[i].blend = &blendState;
        colorTarget[i].writeMask = WGPUColorWriteMask_All; // We could write to only some of the color channels.
    }

    WGPUDepthStencilState depthStencilState = {};
    setDefault(depthStencilState);
    depthStencilState.depthCompare = WGPUCompareFunction_Less;
    depthStencilState.depthWriteEnabled = true;
    depthStencilState.format = renderPasses[targetRenderPass].depthStencilAttachments.format;
    depthStencilState.stencilReadMask = 0;
    depthStencilState.stencilWriteMask = 0;
    
    fragmentState.targetCount = colorTarget.size();
    fragmentState.targets = colorTarget.data();

    pipelineDesc.fragment = &fragmentState;
    pipelineDesc.depthStencil = &depthStencilState;

    /*WGPUFragmentState fragmentState{};
    fragmentState.module = shader.wgData.shaderModuleFrag;
    fragmentState.entryPoint = "main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;

    WGPUBlendState blendState{};
    blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blendState.color.operation = WGPUBlendOperation_Add;
    blendState.alpha.srcFactor = WGPUBlendFactor_Zero;
    blendState.alpha.dstFactor = WGPUBlendFactor_One;
    blendState.alpha.operation = WGPUBlendOperation_Add;

    WGPUColorTargetState colorTarget{};
    colorTarget.format = surfaceFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = WGPUColorWriteMask_All; // We could write to only some of the color channels.

    // We have only one target because our render pass has only one output color
    // attachment.
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    pipelineDesc.fragment = &fragmentState;

    WGPUDepthStencilState depthStencilState;
    setDefault(depthStencilState);
    depthStencilState.depthCompare = WGPUCompareFunction_Less;
    depthStencilState.depthWriteEnabled = true;
    depthStencilState.format = depthTextureFormat;
    depthStencilState.stencilReadMask = 0;
    depthStencilState.stencilWriteMask = 0;

    pipelineDesc.depthStencil = &depthStencilState;*/

    pipelineDesc.multisample.count = 1;
    pipelineDesc.multisample.mask = ~0u;// Default value for the mask, meaning "all bits on"
    pipelineDesc.multisample.alphaToCoverageEnabled = false;// Default value as well (irrelevant for count = 1 anyways)

    std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(3);

    // Define binding layout
    bindingLayoutEntries[0] = {};
    setDefault(bindingLayoutEntries[0]);
    bindingLayoutEntries[0].binding = 0;// The binding index as used in the @binding attribute in the shader
    bindingLayoutEntries[0].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;// The stage that needs to access this resource
    bindingLayoutEntries[0].buffer.type = WGPUBufferBindingType_Uniform;
    bindingLayoutEntries[0].buffer.minBindingSize = shader.wgData.materialMainSetDef.bufferSize; //4 * sizeof(float);

    bindingLayoutEntries.resize(maxTexSlots * 2 + 1);
    for(int i = 1; i < bindingLayoutEntries.size(); i+=2){
        bindingLayoutEntries[i] = {};
        setDefault(bindingLayoutEntries[i]);
        bindingLayoutEntries[i].binding = i;
        bindingLayoutEntries[i].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
        bindingLayoutEntries[i].texture.sampleType = WGPUTextureSampleType_Float;
        bindingLayoutEntries[i].texture.viewDimension = WGPUTextureViewDimension_2D;
        bindingLayoutEntries[i].texture.multisampled = false;
        
        bindingLayoutEntries[i+1] = {};
        setDefault(bindingLayoutEntries[i+1]);
        bindingLayoutEntries[i+1].binding = i+1;
        bindingLayoutEntries[i+1].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
        bindingLayoutEntries[i+1].sampler.type = WGPUSamplerBindingType_Filtering;
    }

    /*setDefault(bindingLayoutEntries[1]);
    bindingLayoutEntries[1].binding = 1;
    bindingLayoutEntries[1].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[1].texture.sampleType = WGPUTextureSampleType_Float;
    bindingLayoutEntries[1].texture.viewDimension = WGPUTextureViewDimension_2D;
    bindingLayoutEntries[1].texture.multisampled = false;
    setDefault(bindingLayoutEntries[2]);
    bindingLayoutEntries[2].binding = 2;
    bindingLayoutEntries[2].visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindingLayoutEntries[2].sampler.type = WGPUSamplerBindingType_Filtering;*/

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
    bindGroupLayoutDesc.nextInChain = nullptr;
    bindGroupLayoutDesc.entryCount = bindingLayoutEntries.size();// 1;
    bindGroupLayoutDesc.entries = bindingLayoutEntries.data();// &bindingLayout;
    shader.wgData.bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);

    std::vector<WGPUBindGroupLayout> bindGroupLayouts{
        shader.wgData.bindGroupLayout,
        perDrawBindGroupLayout,
        cameraBindGroupLayout
    };

    if(ContainKey("SKINNED")){
        bindGroupLayouts[1] = perDrawSkinnedBindGroupLayout; 
    }

    // Create the pipeline layout
    WGPUPipelineLayoutDescriptor layoutDesc = {};
    layoutDesc.nextInChain = nullptr;
    layoutDesc.bindGroupLayoutCount = bindGroupLayouts.size(); //1;
    layoutDesc.bindGroupLayouts = bindGroupLayouts.data();// &shader.wgData.bindGroupLayout;
    shader.wgData.layout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);

    pipelineDesc.layout = shader.wgData.layout; //nullptr;

    return wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
}

bool WebGPUGraphicsDevice::SubShaderCreateFromBaseSource(
    SubShader& shader,
    std::string& source, 
    std::vector<std::string>& keyworlds,
    ShaderPipeline pipeline, 
    std::vector<std::string>& errors
){
    glslang_initialize_process();

    #define Header "#version 450"
    std::string vertexToInsert = Header "\n#define VERTEX\n#define WebGPU_API\n";
    std::string fragToInsert = Header "\n#define FRAGMENT\n#define WebGPU_API\n";

    ShaderSpiv spivVertex;
    ShaderSpiv spivFrag;

    source.insert(0, vertexToInsert);
    if(CompileShader(source,  GLSLANG_STAGE_VERTEX, spivVertex) == false) Assert(false);

    /*WGPUShaderModuleWGSLDescriptor shaderCodeDesc2{};
    shaderCodeDesc2.chain.next = nullptr;// Set the chained struct's header
    shaderCodeDesc2.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    shaderCodeDesc2.code = source.c_str();
    WGPUShaderModuleDescriptor shaderDesc2{};
    shaderDesc2.nextInChain = &shaderCodeDesc2.chain;// Connect the chain
    wgpuDeviceCreateShaderModule(device, &shaderDesc2);*/

    source.erase(0, vertexToInsert.size());

    source.insert(0, fragToInsert);
    if(CompileShader(source, GLSLANG_STAGE_FRAGMENT, spivFrag) == false) Assert(false);
    source.erase(0, fragToInsert.size());

    Assert(SpirvReflectMainSet(spivVertex.data(), spivVertex.size() * sizeof(unsigned int), shader.wgData.materialMainSetDef) == true);

    WGPUShaderModuleDescriptor shaderDesc{};
    #ifdef WEBGPU_BACKEND_WGPU
    shaderDesc.hintCount = 0;
    shaderDesc.hints = nullptr;
    #endif

    WGPUShaderModuleSPIRVDescriptor shaderCodeDesc{};
    shaderCodeDesc.chain.next = nullptr;// Set the chained struct's header
    shaderCodeDesc.chain.sType = WGPUSType_ShaderModuleSPIRVDescriptor;
    shaderCodeDesc.code = spivVertex.data();
    shaderCodeDesc.codeSize = spivVertex.size();
    shaderDesc.nextInChain = &shaderCodeDesc.chain;// Connect the chain
    shader.wgData.shaderModuleVertex = wgpuDeviceCreateShaderModule(device, &shaderDesc);

    shaderCodeDesc.code = spivFrag.data();
    shaderCodeDesc.codeSize = spivFrag.size();
    shader.wgData.shaderModuleFrag = wgpuDeviceCreateShaderModule(device, &shaderDesc);
    glslang_finalize_process();

    //shader.wgData.pipeline = CreatePipeline(shader, keyworlds, pipeline);
    shader.wgData.pipelines.resize(2);
    for(int i = 0; i < shader.wgData.pipelines.size(); i++){
        shader.wgData.pipelines[i] = CreatePipeline(shader, keyworlds, pipeline, i);
    }

    return true;
}

void WebGPUGraphicsDevice::SubShaderDestroy(SubShader& shader){
    wgpuShaderModuleRelease(shader.wgData.shaderModuleVertex);
    wgpuShaderModuleRelease(shader.wgData.shaderModuleFrag);
    //wgpuRenderPipelineRelease(shader.wgData.pipeline);
    for(auto i: shader.wgData.pipelines) wgpuRenderPipelineRelease(i);
    wgpuPipelineLayoutRelease(shader.wgData.layout);
    wgpuBindGroupLayoutRelease(shader.wgData.bindGroupLayout);

    shader.wgData.shaderModuleVertex = nullptr;
    shader.wgData.shaderModuleFrag = nullptr;
    //shader.wgData.pipeline = nullptr;
    shader.wgData.pipelines.clear();
    shader.wgData.layout = nullptr;
    shader.wgData.bindGroupLayout = nullptr;
}

bool WebGPUGraphicsDevice::SubShaderIsValid(SubShader& shader){
    if(shader.wgData.pipelines.size() <= 0) return false;
    //if(shader.wgData.pipeline == nullptr) return false;
    return true;
}

void WebGPUGraphicsDevice::SubShaderBind(SubShader& shader){
    //wgpuRenderPassEncoderSetPipeline(renderPass, shader.wgData.pipeline);
    wgpuRenderPassEncoderSetPipeline(renderPass, shader.wgData.pipelines[currentRendePassTarget]);
}

bool WebGPUGraphicsDevice::ShaderCreate(Shader& shader, std::string inPath){
    //LogInfo("Create Shader: %s", inPath.c_str());
    return shader.Create(inPath);
}

void WebGPUGraphicsDevice::ShaderDestroy(Shader& shader){

}

bool WebGPUGraphicsDevice::MaterialCreate(Material& shader){
    return false;
}

void WebGPUGraphicsDevice::MaterialDestroy(Material& shader){

}

void WebGPUGraphicsDevice::MaterialOnSetShader(Material& mat){
    auto CreateUniformBuffer = [&](size_t size, const char* label){
        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.nextInChain = nullptr;
        bufferDesc.label = label;
        bufferDesc.size = size;
        bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
        bufferDesc.mappedAtCreation = false;
        WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
        return buffer;
    };

    mat.wgData.materialMainSetDef = mat.currentShader->wgData.materialMainSetDef;
    mat.wgData.mainUniformData = malloc(mat.wgData.materialMainSetDef.bufferSize);
    memset( mat.wgData.mainUniformData, 0, mat.wgData.materialMainSetDef.bufferSize);
    mat.wgData.mainUniformBuffer = CreateUniformBuffer(mat.wgData.materialMainSetDef.bufferSize, "UniformBuffer");

    //UpdateMaterialMainSet(mat);

    /*std::vector<WGPUBindGroupEntry> bindings(3);
    bindings[0] = {};
    bindings[0].nextInChain = nullptr;
    bindings[0].binding = 0;// The index of the binding (the entries in bindGroupDesc can be in any order)
    bindings[0].buffer = mat.wgData.mainUniformBuffer;// The buffer it is actually bound to
    bindings[0].size = mat.wgData.materialMainSetDef.bufferSize;// And we specify again the size of the buffer.
    bindings[0].offset = 0;

    bindings.resize(maxTexSlots * 2 + 1);
    for(int i = 1; i < bindings.size(); i+=2){
        bindings[i] = {};
        bindings[i].nextInChain = nullptr;
        bindings[i].binding = i;
        bindings[i].textureView = defaultTex.wgData.textureView;
        bindings[i+1] = {};
        bindings[i+1].nextInChain = nullptr;
        bindings[i+1].binding = i+1;
        bindings[i+1].sampler = defaultTex.wgData.sampler;
    }

    // A bind group contains one or multiple bindings
    WGPUBindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.nextInChain = nullptr;
    bindGroupDesc.layout = mat.currentShader->wgData.bindGroupLayout;// bindGroupLayout;
    bindGroupDesc.entryCount = bindings.size();// 1; // There must be as many bindings as declared in the layout!
    bindGroupDesc.entries = bindings.data();// &binding;
    mat.wgData.mainBindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);*/
}

void WebGPUGraphicsDevice::UpdateMaterialMainSet(Material& mat, std::vector<TexTarget>& texs){
    std::vector<WGPUBindGroupEntry> bindings(3);
    bindings[0] = {};
    bindings[0].nextInChain = nullptr;
    bindings[0].binding = 0;// The index of the binding (the entries in bindGroupDesc can be in any order)
    bindings[0].buffer = mat.wgData.mainUniformBuffer;// The buffer it is actually bound to
    bindings[0].size = mat.wgData.materialMainSetDef.bufferSize;// And we specify again the size of the buffer.
    bindings[0].offset = 0;

    bindings.resize(maxTexSlots * 2 + 1);
    int _i = 0;
    Assert(texs.size() == maxTexSlots);
    for(int i = 1; i < bindings.size(); i+=2){
        WGPUTextureView targetTextureView = defaultTex.wgData.textureView;
        WGPUSampler targetSampler = defaultTex.wgData.sampler;

        if(texs[_i].textureView != nullptr){
            targetTextureView =texs[_i].textureView;
            targetSampler = texs[_i].sampler;
        }
        
        bindings[i] = {};
        bindings[i].nextInChain = nullptr;
        bindings[i].binding = i;
        bindings[i].textureView = targetTextureView;
        bindings[i+1] = {};
        bindings[i+1].nextInChain = nullptr;
        bindings[i+1].binding = i+1;
        bindings[i+1].sampler = targetSampler;
        _i += 1;
    }

    // A bind group contains one or multiple bindings
    WGPUBindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.nextInChain = nullptr;
    bindGroupDesc.layout = mat.currentShader->wgData.bindGroupLayout;// bindGroupLayout;
    bindGroupDesc.entryCount = bindings.size();// 1; // There must be as many bindings as declared in the layout!
    bindGroupDesc.entries = bindings.data();// &binding;
    mat.wgData.mainBindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);
}

void WebGPUGraphicsDevice::MaterialOnUnsetShader(Material& mat){
    
}

bool WebGPUGraphicsDevice::ImGuiSupport(){
    return true;
}

void WebGPUGraphicsDevice::ImGuiNewFrame(){
    ImGui_ImplWGPU_NewFrame();
}

void WebGPUGraphicsDevice::ImGuiRenderDrawData(unsigned int x, unsigned int y, unsigned int w, unsigned int h){
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}

}

#endif