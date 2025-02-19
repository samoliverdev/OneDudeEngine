#ifdef WEBGPU_SUPPORT
#include "WebGPUGraphicsDevice.h"
#include "OD/Platform/Platform.h"
#include <GLFW/glfw3.h>

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
};

using namespace Ultis;

WebGPUGraphicsDevice::WebGPUGraphicsDevice(){

}

void WebGPUGraphicsDevice::LoadContext(void* data){

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
	deviceDesc.requiredLimits = nullptr;
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
	config.width = 640;
	config.height = 480;
	config.usage = WGPUTextureUsage_RenderAttachment;
	WGPUTextureFormat surfaceFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);
	config.format = surfaceFormat;

	// And we do not need any particular view format:
	config.viewFormatCount = 0;
	config.viewFormats = nullptr;
	config.device = device;
	config.presentMode = WGPUPresentMode_Fifo;
	config.alphaMode = WGPUCompositeAlphaMode_Auto;

	wgpuSurfaceConfigure(surface, &config);

	// Release the adapter only after it has been fully utilized
	wgpuAdapterRelease(adapter);
}

void WebGPUGraphicsDevice::Shutdown(){
	wgpuSurfaceUnconfigure(surface);
	wgpuQueueRelease(queue);
	wgpuSurfaceRelease(surface);
	wgpuDeviceRelease(device);
}

WGPUTextureView WebGPUGraphicsDevice::GetNextSurfaceTextureView(){
	// Get the surface texture
	WGPUSurfaceTexture surfaceTexture;
	wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);
	if(surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success){
		return nullptr;
	}

	// Create a view for this surface texture
	WGPUTextureViewDescriptor viewDescriptor;
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
	WGPUTextureView targetView = GetNextSurfaceTextureView();
	if (!targetView) return;

	// Create a command encoder for the draw call
	WGPUCommandEncoderDescriptor encoderDesc = {};
	encoderDesc.nextInChain = nullptr;
	encoderDesc.label = "My command encoder";
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

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

	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;
	renderPassDesc.depthStencilAttachment = nullptr;
	renderPassDesc.timestampWrites = nullptr;

	// Create the render pass and end it immediately (we only clear the screen but do not draw anything)
	WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
	wgpuRenderPassEncoderEnd(renderPass);
	wgpuRenderPassEncoderRelease(renderPass);

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

void WebGPUGraphicsDevice::_End(){

}

GraphicsStats& WebGPUGraphicsDevice::GetStats(){
    return stats;
}

GraphicsDeviceInfo WebGPUGraphicsDevice::GetInfo(){
    return info;
}

void WebGPUGraphicsDevice::Begin(){

}

void WebGPUGraphicsDevice::End(){

}

bool WebGPUGraphicsDevice::HasBegin(){
    return false;
}

void WebGPUGraphicsDevice::SetCamera(Camera& camera){

}

Camera WebGPUGraphicsDevice::GetCamera(){
    return camera;
}

void WebGPUGraphicsDevice::Clean(float r, float g, float b, float a){
    
}

void WebGPUGraphicsDevice::SetViewport(unsigned int x, unsigned int y, unsigned int w, unsigned int h){

}

void WebGPUGraphicsDevice::GetViewport(unsigned int*x, unsigned int* y, unsigned int* w, unsigned int* h){

}

void WebGPUGraphicsDevice::BindMaterial(Material& mat){

}

void WebGPUGraphicsDevice::DrawMesh(Mesh& mesh, Matrix4 modelMatrix){
    
}

void WebGPUGraphicsDevice::DrawMeshSkinned(Mesh& mesh, Matrix4 model, Matrix4* animMatrix, int count){

}

void WebGPUGraphicsDevice::DrawMeshInstancing(Mesh& mesh, Matrix4* modelMatrixs, int count){

}

void WebGPUGraphicsDevice::DrawMesh(Mesh& mesh, Material& shader, Matrix4 modelMatrix){

}

void WebGPUGraphicsDevice::DrawMeshSkinned(Mesh& mesh, Material& shader, Matrix4 model, Matrix4* animMatrix, int count){

}

void WebGPUGraphicsDevice::DrawMeshInstancing(Mesh& mesh, Material& shader, Matrix4* animMatrixs, int count){

}

void WebGPUGraphicsDevice::DrawModel(Model& model, Matrix4 modelMatrix){
    
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
    return false;
}

void WebGPUGraphicsDevice::MeshSubmitInstancingModelMatrixs(Mesh& mesh){

}

void WebGPUGraphicsDevice::MeshSubmitInstancingCustomModelMatrixs(Mesh& mesh, Matrix4* modelMatrixs, int count){

}

void WebGPUGraphicsDevice::MeshDestroy(Mesh& mesh){

}

bool WebGPUGraphicsDevice::MeshIsValid(Mesh& mesh){
    return false;
}

void WebGPUGraphicsDevice::BeginFramebuffer(Framebuffer& frambuffer, int layer){

}

void WebGPUGraphicsDevice::EndFramebuffer(){

}

bool WebGPUGraphicsDevice::FramebufferCreate(Framebuffer& frambuffer, FrameBufferSpecification specification){
    return false;
}

void WebGPUGraphicsDevice::FramebufferDestroy(Framebuffer& frambuffer){

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

bool WebGPUGraphicsDevice::Texture2DCreate(Texture2D& tex, const std::string path, Texture2DSetting settings){
    return false;
}

bool WebGPUGraphicsDevice::Texture2DCreate(Texture2D& tex, void* data, size_t size, Texture2DSetting settings){
    return false;
}

bool WebGPUGraphicsDevice::Texture2DCreate(Texture2D& tex, void* data, size_t size, int width, int height, TextureDataType dataType, Texture2DSetting settings){
    return false;
}

void WebGPUGraphicsDevice::Texture2DDestroy(Texture2D& tex){

}

bool WebGPUGraphicsDevice::Texture2DIsValid(Texture2D& tex){
    return false;
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

bool WebGPUGraphicsDevice::SubShaderCreateFromBaseSource(
    SubShader& shader,
    std::string& source, 
    std::vector<std::string>& keyworlds,
    ShaderPipeline pipeline, 
    std::vector<std::string>& errors
){
    return false;
}

void WebGPUGraphicsDevice::SubShaderDestroy(SubShader& shader){

}

bool WebGPUGraphicsDevice::SubShaderIsValid(SubShader& shader){
    return false;
}

void WebGPUGraphicsDevice::SubShaderBind(SubShader& shader){

}

bool WebGPUGraphicsDevice::ShaderCreate(Shader& shader, std::string path){
    return false;
}

void WebGPUGraphicsDevice::ShaderDestroy(Shader& shader){

}

bool WebGPUGraphicsDevice::MaterialCreate(Material& shader){
    return false;
}

void WebGPUGraphicsDevice::MaterialDestroy(Material& shader){

}

bool WebGPUGraphicsDevice::ImGuiSupport(){
    return false;
}

void WebGPUGraphicsDevice::ImGuiNewFrame(){

}

void WebGPUGraphicsDevice::ImGuiRenderDrawData(unsigned int x, unsigned int y, unsigned int w, unsigned int h){

}

}

#endif