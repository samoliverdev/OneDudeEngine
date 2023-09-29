#include "Texture.h"
#include "OD/Core/ImGui.h"
#include <stb/stb_image.h>

namespace OD{

void Texture2D::texture2DGenerate(unsigned int width, unsigned int height, unsigned char* data){
    _width = width;
    _height = height;
    
    glGenTextures(1, &_id);
    glCheckError();

    glBindTexture(GL_TEXTURE_2D, _id);
    glTexImage2D(GL_TEXTURE_2D, 0, _internalFormat, width, height, 0, _imageFormat, GL_UNSIGNED_BYTE, data);
    if(_mipmap == true) glGenerateMipmap(GL_TEXTURE_2D);
    glCheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _wrapT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _filterMin);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _filterMax);
    glCheckError();

    glBindTexture(GL_TEXTURE_2D, 0);
    glCheckError();
}

bool Texture2D::IsValid(){
    return _id != 0;
}

void Texture2D::Destroy(){
    if(_id != 0) glDeleteTextures(1, &_id);
    _id = 0;
    glCheckError();
}

void Texture2D::Bind(int index){
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D, _id);
    glCheckError();
}

/*
void Texture2D::Bind(int index, const char* name, Shader& shader){
    Bind(index);
    shader.Bind();
    shader.SetInt(name, index);
}
*/

void Texture2D::OnGui(){
    ImGui::Text("--------Texture2D--------");
    ImGui::Text("Path: %s", path().c_str());

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

    float aspect = _width / _height;
    ImGui::Image((void*)(uint64_t)_id, ImVec2(viewportPanelSize.x, viewportPanelSize.x * aspect), ImVec2(0, 0), ImVec2(1, -1));
}

Ref<Texture2D> Texture2D::CreateFromFile(const char* filePath, TextureFilter filter, bool mipmap){
    Ref<Texture2D> texture = CreateRef<Texture2D>();

    texture->_internalFormat = GL_RGB;
    texture->_imageFormat = GL_RGB;
    texture->_wrapS = GL_REPEAT;
    texture->_wrapT = GL_REPEAT;
    texture->_filterMin = filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    texture->_filterMax = filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    texture->_mipmap = mipmap;
    //texture->filterMin = (unsigned int)filter;
    //texture->filterMin = (unsigned int)filter;

    stbi_set_flip_vertically_on_load(1);

    int width;
    int height;
    int nrChannels;
    unsigned char* data = stbi_load(filePath, &width, &height, &nrChannels, 0);

    //LogInfo("Chennels %d", nrChannels);

    if(!data){
        LogError("Cannot load file image %s\nSTB Reason: %s\n", filePath, stbi_failure_reason());
    }

    bool alpha = false;
    if(nrChannels > 3) alpha = true;

    if(alpha){
        texture->_internalFormat = GL_RGBA;
        texture->_imageFormat = GL_RGBA;
    }
    
    texture->texture2DGenerate(width, height, data);

    stbi_image_free(data);

    texture->_path = std::string(filePath);
    return texture;
}

Ref<Texture2D> Texture2D::CreateFromFile(const char* filePath){
    return CreateFromFile(filePath, TextureFilter::Linear, true);
}

}