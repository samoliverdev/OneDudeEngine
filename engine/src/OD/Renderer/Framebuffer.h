#pragma once

#include <vector>

namespace OD{

class Shader;

enum class FramebufferTextureFormat{
    None, RGB, RGBA8, RGB16F, RGBA16F, RGB32F, RGBA32F, RED_INTEGER, DEPTH4STENCIL8, DEPTH_COMPONENT
};

enum class FramebufferAttachmentType{
    TEXTURE_2D,
    TEXTURE_2D_MULTISAMPLE,
    TEXTURE_2D_ARRAY
};

struct FramebufferAttachment{
    FramebufferTextureFormat colorFormat;
};

struct FrameBufferSpecification{
    int width;
    int height;
    unsigned int sample = 1;
    FramebufferAttachmentType type = FramebufferAttachmentType::TEXTURE_2D;

    std::vector<FramebufferAttachment> colorAttachments;
    FramebufferAttachment depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8};

    bool swapChainTarget = false;
};

class Framebuffer{
public:
    Framebuffer(FrameBufferSpecification specification);
    ~Framebuffer();

    void Reload(FrameBufferSpecification specification);
    void Resize(int width, int height);
    bool IsValid();
    void Destroy();
    void Invalidate();
    void Bind();
    void Unbind();
    //void BindColorAttachmentTexture(Shader& shader, int index);
    unsigned int ColorAttachmentId(int index);
    unsigned int DepthAttachmentId();

    int ReadPixel(int attachmentIndex, int x, int y);

    inline int width(){ return _specification.width; }
    inline int height(){ return _specification.height; }

    inline unsigned int renderId(){ return _framebuffer; }
    inline FrameBufferSpecification specification(){ return _specification; }

private:
    FrameBufferSpecification _specification;
    unsigned int _framebuffer = 0;

    //unsigned int colorAttachment;
    unsigned int _depthAttachment;
    std::vector<unsigned int> _colorAttachments;

    void GenColorAttachment(int index);
    void GenDepthAttachment();
};

}