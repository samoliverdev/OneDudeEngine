#include "Framebuffer.h"
#include "OD/Platform/GL.h"
#include "OD/Defines.h"
#include "Shader.h"

namespace OD{

GLenum GetGLColorFormart(FramebufferTextureFormat format){
    if(format == FramebufferTextureFormat::RGB) return GL_RGB;
    if(format == FramebufferTextureFormat::RGBA8) return GL_RGBA8;
    if(format == FramebufferTextureFormat::RED_INTEGER) return GL_RED_INTEGER;
    if(format == FramebufferTextureFormat::DEPTH4STENCIL8) return GL_DEPTH24_STENCIL8;

    Assert(false);
    return 0;
}

void Framebuffer::GenColorAttachment(int index){
    if(_specification.colorAttachments[index].writeOnly){
        Assert(false);
    } else {
        GLenum internalFormat = GL_RGB;
        GLenum format = GL_RGB;

        if(_specification.colorAttachments[index].colorFormat == FramebufferTextureFormat::RGBA8){
            internalFormat = GL_RGB8;
            format = GL_RGBA;
        }

        if(_specification.colorAttachments[index].colorFormat == FramebufferTextureFormat::RGB16F){
            internalFormat = GL_RGB16F;
            format = GL_RGB;
        }

        if(_specification.colorAttachments[index].colorFormat == FramebufferTextureFormat::RGBA16F){
            internalFormat = GL_RGBA16F;
            format = GL_RGBA;
        }

        if(_specification.colorAttachments[index].colorFormat == FramebufferTextureFormat::RGB32F){
            internalFormat = GL_RGB32F;
            format = GL_RGB;
        }

        if(_specification.colorAttachments[index].colorFormat == FramebufferTextureFormat::RGBA32F){
            internalFormat = GL_RGBA32F;
            format = GL_RGBA;
        }
        
        if(_specification.colorAttachments[index].colorFormat == FramebufferTextureFormat::RED_INTEGER){
            internalFormat = GL_R32I;
            format = GL_RED_INTEGER;
        }

        bool multisample = _specification.sample > 1;

        unsigned int colorAttachment;
        glGenTextures(1, &colorAttachment);
        glCheckError();

        if(multisample){
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorAttachment);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, _specification.sample, internalFormat, _specification.width, _specification.height, GL_TRUE);
            glCheckError();
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D_MULTISAMPLE, colorAttachment, 0);
            glCheckError();
        } else {
            glBindTexture(GL_TEXTURE_2D, colorAttachment);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, _specification.width, _specification.height, 0, format, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glCheckError();
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, GL_TEXTURE_2D, colorAttachment, 0);
            glCheckError();
        }

        _colorAttachments.push_back(colorAttachment);
    }
}

void Framebuffer::GenDepthAttachment(){
    if(_specification.depthAttachment.colorFormat == FramebufferTextureFormat::None) return;

    GLenum internalFormat = GL_DEPTH24_STENCIL8;
    GLenum format = GL_DEPTH_STENCIL;
    GLenum type = GL_UNSIGNED_INT_24_8;
    GLenum attachment = GL_DEPTH_STENCIL_ATTACHMENT;

    if(_specification.depthAttachment.colorFormat == FramebufferTextureFormat::DEPTH4STENCIL8){
        internalFormat = GL_DEPTH24_STENCIL8;
        format = GL_DEPTH_STENCIL;
        type = GL_UNSIGNED_INT_24_8;
        attachment = GL_DEPTH_STENCIL_ATTACHMENT;
    }
    
    if(_specification.depthAttachment.colorFormat == FramebufferTextureFormat::DEPTH_COMPONENT){
        internalFormat = GL_DEPTH_COMPONENT;
        format = GL_DEPTH_COMPONENT;
        type = GL_FLOAT;
        attachment = GL_DEPTH_ATTACHMENT;
    }

    bool multisample = _specification.sample > 1;

    if(_specification.depthAttachment.writeOnly){
        glGenRenderbuffers(1, &_depthAttachment);
        glBindRenderbuffer(GL_RENDERBUFFER, _depthAttachment);
        if(multisample){
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, _specification.sample, internalFormat, _specification.width, _specification.height); // use a single renderbuffer object for both a depth AND stencil buffer.
        } else {    
            glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, _specification.width, _specification.height); // use a single renderbuffer object for both a depth AND stencil buffer.
        }
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, _depthAttachment); // now actually attach it
        glCheckError();
    } else {
        glGenTextures(1, &_depthAttachment);
        if(multisample){
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, _depthAttachment);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, _specification.sample, internalFormat, _specification.width, _specification.height, GL_TRUE);
            glCheckError();
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D_MULTISAMPLE, _depthAttachment, 0);
            glCheckError();
        } else {
            glBindTexture(GL_TEXTURE_2D, _depthAttachment);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, _specification.width, _specification.height, 0, format, type, NULL);
            glCheckError();
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);  
            glCheckError();

            glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, _depthAttachment, 0);
            glCheckError();
        }
    }
}

Framebuffer::Framebuffer(FrameBufferSpecification specification){
    _specification = specification;
    Invalidate();
}

Framebuffer::~Framebuffer(){
    //Destroy();
}

void Framebuffer::Reload(FrameBufferSpecification specification){
    _specification = specification;
    Invalidate();
}

bool Framebuffer::IsValid(){
    return _framebuffer > 0;
}

void Framebuffer::Destroy(){
    Assert(_framebuffer > 0);
    //LogInfo("Framebuffer::Destroy %d", _framebuffer);
    
    glDeleteFramebuffers(1, &_framebuffer);
    glCheckError();

    for(int i = 0; i < _specification.colorAttachments.size(); i++){
        //if(_specification.colorAttachments[i].writeOnly){
        //    glDeleteRenderbuffers(1, &_colorAttachments[i]);
        //} else {
            glDeleteTextures(1, &_colorAttachments[i]);
        //}
    }
    glCheckError();

    if(_specification.depthAttachment.writeOnly){
        glDeleteRenderbuffers(1, &_depthAttachment);
    } else {
        glDeleteTextures(1, &_depthAttachment);
    }
    glCheckError();

    _framebuffer = 0;
    _colorAttachments.clear();
}

void Framebuffer::Resize(int width, int height){
    if(_specification.width == width && _specification.height == height) return;

    _specification.width = width;
    _specification.height = height;
    
    Invalidate();
}

void Framebuffer::Invalidate(){
    //Clean Framebuffer
    if(_framebuffer) Destroy();
    
    //Gen Framebuffer
    glGenFramebuffers(1, &_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glCheckError();

    //Gen Attachments
    for(int i = 0; i < _specification.colorAttachments.size(); i++){
        GenColorAttachment(i);
    }
    GenDepthAttachment();

    if(_specification.colorAttachments.size() == 0){
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glCheckError();
    } else if(_specification.colorAttachments.size() > 1){
        Assert(_specification.colorAttachments.size() <= 4);
		
        std::vector<GLenum> buffers;
        for(int i = 0; i < _specification.colorAttachments.size(); i++){
            buffers.push_back(GL_COLOR_ATTACHMENT0+i);
        }
		
        glDrawBuffers(_specification.colorAttachments.size(), &buffers[0]);
        glCheckError();
    }

    //Check Erros
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        LogError("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
        Assert(false);
    }
    
    //Unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();
}

void Framebuffer::Bind(){
    Assert(_framebuffer > 0);
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    glCheckError();
    //glViewport(0, 0, _specification.width, _specification.height);
}
    
void Framebuffer::Unbind(){
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();
}

/*
void Framebuffer::BindColorAttachmentTexture(Shader& shader, int index){
    Assert(colorAttachments.size() != 0);
    shader.Bind();
    glBindTexture(GL_TEXTURE_2D, colorAttachments[0]);
}
*/

unsigned int Framebuffer::ColorAttachmentId(int index){ 
    Assert(index < _colorAttachments.size());
    return _colorAttachments[index]; 
}

unsigned int Framebuffer::DepthAttachmentId(){
    return _depthAttachment;
}

int Framebuffer::ReadPixel(int attachmentIndex, int x, int y){
    Assert(attachmentIndex < _colorAttachments.size());

    glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
    glCheckError();

    int pixelData;
    glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
    glCheckError();
    
    return pixelData;
}

}