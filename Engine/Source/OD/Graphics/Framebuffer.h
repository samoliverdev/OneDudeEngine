#pragma once
#include "OD/Defines.h"
#include <vector>

#include "OD/Platform/OpenGL/GL.h"
#include "OD/Platform/WebGPU/WebGPU.h"

namespace sol{ class state; }

namespace OD{

class SubShader;

enum class OD_API_IMPORT FramebufferTextureFormat{
    None, RGB, RGBA8, RGB16F, RGBA16F, RGB32F, RGBA32F, RED_INTEGER, DEPTH4STENCIL8, DEPTH_COMPONENT
};

enum class OD_API_IMPORT FramebufferAttachmentType{
    TEXTURE_2D,
    TEXTURE_2D_MULTISAMPLE,
    TEXTURE_2D_ARRAY
};

struct OD_API FramebufferAttachment{
    FramebufferTextureFormat colorFormat;
};

struct OD_API FrameBufferSpecification{
    int width;
    int height;
    unsigned int sample = 1;
    FramebufferAttachmentType type = FramebufferAttachmentType::TEXTURE_2D;

    std::vector<FramebufferAttachment> colorAttachments;
    FramebufferAttachment depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8};

    bool swapChainTarget = false;
};

enum class OD_API_IMPORT FramebufferType{
    Stand = 0, 
    Deffered,
    Shadowmap,
    //Dynamic
};

class OD_API Framebuffer{
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
    void Invalidate();
    int ReadPixel(int attachmentIndex, int x, int y);
    bool IsValid();
    void* ColorAttachmentId(int index);
    void* DepthAttachmentId();
    
    inline int Width(){ return specification.width; }
    inline int Height(){ return specification.height; }
    inline FrameBufferSpecification Specification(){ return specification; }

    static void CreateLuaBind(sol::state& lua);

private:
    FramebufferType type;
    FrameBufferSpecification specification;
    FramebufferDataGL;
    FramebufferDataWG;
};

}