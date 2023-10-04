#pragma once

#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Platform/GL.h"
//#include "Shader.h"

namespace OD {

enum class TextureFilter {
    Nearest,
    Linear
};

class Renderer;

struct Texture2DSetting{
    TextureFilter filter = TextureFilter::Linear;
    bool mipmap = true;
};

class Texture2D: public Asset{
    friend class Renderer;
public:
    bool IsValid();
    void Destroy();
    void Bind(int index);
    //void Bind(int index, const char* name, Shader& shader);

    void OnGui() override;

    inline unsigned int width(){ return _width; }
    inline unsigned int height(){ return _height; }
    inline unsigned int renderId(){ return _id; }

    static Ref<Texture2D> CreateFromFile(const char* filePath, Texture2DSetting settings); 
    static Ref<Texture2D> CreateFromFile(const char* filePath); 

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
    Texture2DSetting _settings;

    void Create(const char* path, Texture2DSetting settings);
    void texture2DGenerate(unsigned int width, unsigned int height, unsigned char* data);

    void SaveSettings();
};

}