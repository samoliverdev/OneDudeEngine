#pragma once
#include "OD/Defines.h"
#include "OD/Platform/OpenGL/GL.h"
#include "OD/Platform/WebGPU/WebGPU.h"
#include "OD/Core/Resource.h"
#include <vector>

namespace sol{ class state; }

namespace OD{

class SubShader;

enum class OD_API_IMPORT FramebufferTextureFormat{
    None = 0, RGB, RGBA8, RGB11B10F, RGB16F, RGBA16F, RGB32F, RGBA32F, RED_INTEGER, DEPTH24_STENCIL8, DEPTH32F_STENCIL8, DEPTH_COMPONENT16, DEPTH_COMPONENT24, DEPTH_COMPONENT32, DEPTH_COMPONENT32F
};

enum class OD_API_IMPORT FramebufferAttachmentType{
    TEXTURE_2D,
    TEXTURE_2D_MULTISAMPLE,
    TEXTURE_2D_ARRAY,
    CUBEMAP
};

struct OD_API FramebufferAttachment{
    FramebufferTextureFormat colorFormat;
    bool genMip = false;
    int mipLevels = 1;
};

struct OD_API FrameBufferSpecification{
    int width;
    int height;
    unsigned int sample = 1;
    FramebufferAttachmentType type = FramebufferAttachmentType::TEXTURE_2D;

    std::vector<FramebufferAttachment> colorAttachments;
    FramebufferAttachment depthAttachment = {FramebufferTextureFormat::DEPTH24_STENCIL8};
    bool createDepth = true;

    bool swapChainTarget = false;
};

enum class OD_API_IMPORT FramebufferType{
    Stand = 0, 
    Deffered,
    Shadowmap,
    Dynamic
};

class OD_API Framebuffer: public Resource{
    friend class OpenGLGraphicsDevice;
    friend class WebGPUGraphicsDevice;
public:
    Framebuffer(FramebufferType type, int width, int height, int layers = 1);
    Framebuffer(FrameBufferSpecification specification);
    ~Framebuffer();
    
    Framebuffer& operator=(const Framebuffer& other) = delete;
    Framebuffer(const Framebuffer& other) = delete;

    void Reload(FrameBufferSpecification specification);
    void Resize(int width, int height);
    void GenMipmap();
    void Invalidate();
    
    int ReadPixel(int attachmentIndex, int x, int y);

    bool IsValid();
    void* ColorAttachmentId(int index);
    void* DepthAttachmentId();
    
    inline int Width(){ return specification.width; }
    inline int Height(){ return specification.height; }
    inline FrameBufferSpecification Specification(){ return specification; }

    static void CreateLuaBind(sol::state& lua);

    size_t VRamUsage(){ return vramUsage; }

    std::string name = "NoName";

private:
    size_t vramUsage = 0;
    FramebufferType type;
    FrameBufferSpecification specification;
    FramebufferDataGL;
    FramebufferDataWG;
};

}