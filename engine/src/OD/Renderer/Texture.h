#pragma once

#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Platform/GL.h"
//#include "Shader.h"

namespace OD {

enum class TextureFilter {
    Nearest = GL_NEAREST,
    Linear = GL_LINEAR
};

class Renderer;

class Texture2D: public Asset{
    friend class Renderer;
public:
    bool IsValid();
    void Destroy();
    void Bind(int index);
    //void Bind(int index, const char* name, Shader& shader);

    static Ref<Texture2D> CreateFromFile(const char* filePath, TextureFilter filter, bool mipmap); 

private:
    unsigned int _id;
    unsigned int _width;
    unsigned int _height;
    unsigned int _internalFormat;
    unsigned int _imageFormat;
    unsigned int _wrapS;
    unsigned int _wrapT;
    unsigned int _filterMin;
    unsigned int _filterMax;
    bool _mipmap;

    void texture2DGenerate(unsigned int width, unsigned int height, unsigned char* data);
};

}