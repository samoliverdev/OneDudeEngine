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

#include <GLFW/glfw3.h>

#include "glslang/Include/glslang_c_interface.h"
#include "glslang/Public/resource_limits_c.h"
//#include <spirv_cross_c.h>

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
	surfaceFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);
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
	/*WGPUTextureView*/ targetView = GetNextSurfaceTextureView();
	if(!targetView) return;

	// Create a command encoder for the draw call
	WGPUCommandEncoderDescriptor encoderDesc = {};
	encoderDesc.nextInChain = nullptr;
	encoderDesc.label = "My command encoder";
	/*WGPUCommandEncoder*/ encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

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
	/*WGPURenderPassEncoder*/ renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
}

void WebGPUGraphicsDevice::_End(){
    if(!targetView) return;

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
    /*auto ContainUniformName = [&](SubShader shader, const std::string& name){ 
        return std::find(shader.glData._uniforms.begin(), shader.glData._uniforms.end(), name) != shader.glData._uniforms.end(); 
    };

    auto ApplyUniformTo = [&](Material& material, SubShader& shader, std::unordered_map<std::string, MaterialMap>& maps){
        for(auto& i: maps){
            MaterialMap& map = i.second;

            if(ContainUniformName(shader, i.first) == false) continue;

            if(map.type == MaterialMap::Type::Int){
                SubShaderSetInt(shader, i.first.c_str(), map.valueInt);
            }
            if(map.type == MaterialMap::Type::Float){
                SubShaderSetFloat(shader, i.first.c_str(), map.valueFloat);
            }
            if(map.type == MaterialMap::Type::Vector2){
                SubShaderSetVector2(shader, i.first.c_str(), Vector2(map.vec.vector.x, map.vec.vector.y));
            }
            if(map.type == MaterialMap::Type::Vector3){
                SubShaderSetVector3(shader, i.first.c_str(), Vector3(map.vec.vector.x, map.vec.vector.y, map.vec.vector.z));
            }
            if(map.type == MaterialMap::Type::Vector4){
                SubShaderSetVector4(shader, i.first.c_str(), map.vec.vector);
            }
            if(map.type == MaterialMap::Type::Matrix4){
                SubShaderSetMatrix4(shader, i.first.c_str(), i.second.matrix);
            }
            if(map.type == MaterialMap::Type::Texture){
                Assert(i.second.texture != nullptr);
                SubShaderSetTexture2D(shader, i.first.c_str(), *i.second.texture, material.currentTextureSlot);
                material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::TextureArray){
                SubShaderSetTexture2DArray(shader, i.first.c_str(), *i.second.textureArray, material.currentTextureSlot);
                material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::Framebuffer){
                SubShaderSetFramebuffer(shader, i.first.c_str(), *i.second.framebuffer, material.currentTextureSlot, map.framebufferAttachment);
                material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::Cubemap){
                SubShaderSetCubemap(shader, i.first.c_str(), *i.second.cubemap, material.currentTextureSlot);
                material.currentTextureSlot += 1;
            }
            if(map.type == MaterialMap::Type::FloatList){
                SubShaderSetFloat(shader, i.first.c_str(), static_cast<float*>(map.list), map.listCount);
            }
            if(map.type == MaterialMap::Type::Vector4List){
                SubShaderSetVector4(shader, i.first.c_str(), static_cast<Vector4*>(map.list), map.listCount);
            }
            if(map.type == MaterialMap::Type::Matrix4List){
                SubShaderSetMatrix4(shader, i.first.c_str(), static_cast<Matrix4*>(map.list), map.listCount);
            }
        }
    };*/

    auto SubmitGraphicDatas = [&](Material& material){
        stats.materialSubmitDatas += 1;
        material.currentTextureSlot = 0;
        material.UpdateCurrentShader();

        Assert(material.GetShader() != nullptr);
        if(material.GetShader() == nullptr) return;

        /*SetColorMask(mat.currentShader->pipeline.colorMask);
        SetCullFace(mat.currentShader->GetCullFace());
        SetDepthTest(mat.currentShader->GetDepthTest());
        SetDepthMask(mat.currentShader->IsDepthMask());
        if(mat.currentShader->IsBlend()){
            SetBlend(true);
            SetBlendFunc(mat.currentShader->GetSrcBlend(), mat.currentShader->GetDstBlend());
        } else {
            SetBlend(false);
        }*/

        SubShaderBind(*material.currentShader);
        //ApplyUniformTo(material, *material.currentShader, material.maps);
        //ApplyUniformTo(material, *material.currentShader, Material::globalMaps);
        Assert(material.currentTextureSlot < 32);
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

void WebGPUGraphicsDevice::DrawMesh(Mesh& mesh, Matrix4 modelMatrix){
    if(!targetView) return;
    wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);
}

void WebGPUGraphicsDevice::DrawMeshSkinned(Mesh& mesh, Matrix4 model, Matrix4* animMatrix, int count){

}

void WebGPUGraphicsDevice::DrawMeshInstancing(Mesh& mesh, Matrix4* modelMatrixs, int count){

}

void WebGPUGraphicsDevice::DrawMesh(Mesh& mesh, Material& mat, Matrix4 modelMatrix){
    BindMaterial(mat);
    DrawMesh(mesh, modelMatrix);
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
    source.erase(0, vertexToInsert.size());

    source.insert(0, fragToInsert);
    if(CompileShader(source, GLSLANG_STAGE_FRAGMENT, spivFrag) == false) Assert(false);
    source.erase(0, fragToInsert.size());

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

    //-----------------------------------------------

    WGPURenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.nextInChain = nullptr;

    pipelineDesc.vertex.bufferCount = 0;
    pipelineDesc.vertex.buffers = nullptr;
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

    WGPUFragmentState fragmentState{};
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
    pipelineDesc.depthStencil = nullptr;

    pipelineDesc.multisample.count = 1;
    pipelineDesc.multisample.mask = ~0u;// Default value for the mask, meaning "all bits on"
    pipelineDesc.multisample.alphaToCoverageEnabled = false;// Default value as well (irrelevant for count = 1 anyways)

    pipelineDesc.layout = nullptr;

    shader.wgData.pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
    return true;
}

void WebGPUGraphicsDevice::SubShaderDestroy(SubShader& shader){
    wgpuShaderModuleRelease(shader.wgData.shaderModuleVertex);
    wgpuShaderModuleRelease(shader.wgData.shaderModuleFrag);
    wgpuRenderPipelineRelease(shader.wgData.pipeline);

    shader.wgData.shaderModuleVertex = nullptr;
    shader.wgData.shaderModuleFrag = nullptr;
    shader.wgData.pipeline = nullptr;
}

bool WebGPUGraphicsDevice::SubShaderIsValid(SubShader& shader){
    if(shader.wgData.pipeline == nullptr) return false;
    return true;
}

void WebGPUGraphicsDevice::SubShaderBind(SubShader& shader){
    if(!targetView) return;
    wgpuRenderPassEncoderSetPipeline(renderPass, shader.wgData.pipeline);
}

bool WebGPUGraphicsDevice::ShaderCreate(Shader& shader, std::string inPath){
    LogInfo("Create Shader: %s", inPath.c_str());
    return shader.Create(inPath);
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