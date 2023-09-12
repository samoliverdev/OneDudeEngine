#pragma once

#include <vector>

namespace OD{

class Shader;

enum class FramebufferTextureFormat{
    None, RGB, RGBA8,RED_INTEGER, DEPTH4STENCIL8, DEPTH_COMPONENT
};

struct FramebufferAttachment{
    FramebufferTextureFormat colorFormat;
    bool writeOnly = false;
};

struct FrameBufferSpecification{
    int width;
    int height;
    unsigned sample = 1;

    std::vector<FramebufferAttachment> colorAttachments;
    FramebufferAttachment depthAttachment = {FramebufferTextureFormat::DEPTH4STENCIL8, true};

    bool swapChainTarget = false;
};

class Framebuffer{
public:
    Framebuffer(FrameBufferSpecification specification);
    void Resize(int width, int height);
    bool IsValid();
    void Destroy();
    void Invalidate();
    void Bind();
    void Unbind();
    //void BindColorAttachmentTexture(Shader& shader, int index);
    unsigned int ColorAttachmentId(int index);
    unsigned int DepthAttachmentId();

    inline int width(){ return _specification.width; }
    inline int height(){ return _specification.height; }

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