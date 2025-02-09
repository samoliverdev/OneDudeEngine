#pragma once
#include "OD/Defines.h"
#include <vector>

#include "OD/Platform/OpenGL/GL.h"

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

class OD_API Framebuffer{
    friend class OpenGLGraphicsDevice;
public:
    Framebuffer(FrameBufferSpecification specification);
    ~Framebuffer();

    void Reload(FrameBufferSpecification specification);
    void Resize(int width, int height);
    bool IsValid();
    void Invalidate();
    
    //void BindColorAttachmentTexture(Shader& shader, int index);
    unsigned int ColorAttachmentId(int index);
    unsigned int DepthAttachmentId();

    int ReadPixel(int attachmentIndex, int x, int y);

    inline int Width(){ return specification.width; }
    inline int Height(){ return specification.height; }

    inline unsigned int RenderId(){ return 0; /*renderId;*/ }
    inline FrameBufferSpecification Specification(){ return specification; }

    static void CreateLuaBind(sol::state& lua);

private:
    FrameBufferSpecification specification;
    bool isComplete = false;
    FramebufferDataGL;
};

}