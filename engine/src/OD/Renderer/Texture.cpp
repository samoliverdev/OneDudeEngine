#include "Texture.h"
#include "OD/Core/ImGui.h"
#include <stb/stb_image.h>
#include "OD/Serialization/Serialization.h"

namespace OD{

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

void Texture2D::Create(const char* path, Texture2DSetting settings){
    _path = std::string(path);
    _settings = settings;

    _internalFormat = GL_RGB;
    _imageFormat = GL_RGB;
    _wrapS = GL_REPEAT;
    _wrapT = GL_REPEAT;
    _filterMin = settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    _filterMax = settings.filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
    _mipmap = settings.mipmap;

    stbi_set_flip_vertically_on_load(1);

    int width;
    int height;
    int nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    //LogInfo("Chennels %d", nrChannels);

    if(!data){
        LogError("Cannot load file image %s\nSTB Reason: %s\n", path, stbi_failure_reason());
    }

    bool alpha = false;
    if(nrChannels > 3) alpha = true;

    if(alpha){
        _internalFormat = GL_RGBA;
        _imageFormat = GL_RGBA;
    }
    
    texture2DGenerate(width, height, data);
    stbi_image_free(data);
}

void Texture2D::texture2DGenerate(unsigned int width, unsigned int height, unsigned char* data){
    _width = width;
    _height = height;
    
    glGenTextures(1, &_id);
    glCheckError();

    glBindTexture(GL_TEXTURE_2D, _id);
    glTexImage2D(GL_TEXTURE_2D, 0, _internalFormat, width, height, 0, _imageFormat, GL_UNSIGNED_BYTE, data);
    glCheckError();
    
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

void Texture2D::OnGui(){
    bool save = false;

    //ImGui::Text("--------Texture2D--------");
    //ImGui::Text("Path: %s", path().c_str());

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

    float aspect = _width / _height;
    ImGui::Image((void*)(uint64_t)_id, ImVec2(viewportPanelSize.x, viewportPanelSize.x * aspect), ImVec2(0, 0), ImVec2(1, -1));

    ImGui::Spacing();

    const char* optionsString[] = {"Nearest", "Linear"};
    const char* curOptionString = optionsString[(int)_settings.filter];

    if(ImGui::BeginCombo("filter", curOptionString)){
        for(int i = 0; i < 2; i++){
            bool isSelected = curOptionString == optionsString[i];
            if(ImGui::Selectable(optionsString[i], isSelected)){
                curOptionString = optionsString[i];
                _settings.filter = (TextureFilter)i;
                save = true;
            }

            if(isSelected) ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    if(ImGui::Checkbox("mipmap", &_settings.mipmap)){
        save = true;
    }

    ImGui::Spacing();

    ImGui::Text("Path: %s", path().c_str());
    ImGui::Text("Width: %d Height: %d", _width, _height);

    if(save){
        SaveSettings();
    }
}

void Texture2D::SaveSettings(){
    YAML::Emitter out;

    out << YAML::BeginMap;
    out << YAML::Key << "Texture2DSetting_filter" << YAML::Value << (int)_settings.filter;
    out << YAML::Key << "Texture2DSetting_mipmap" << YAML::Value << _settings.mipmap;
    out << YAML::EndMap;

    std::ofstream fout(std::string(_path) + ".meta");
    fout << out.c_str();

    Destroy();
    Create(_path.c_str(), _settings);
}

void LoadSettings(const char* filePath, Texture2DSetting& settings){
    std::ifstream stream(std::string(filePath) + ".meta");
    if(stream.fail()) return;

    YAML::Node data = YAML::Load(stream);
    LogInfo("Test %zd", data.size());

    if(data["Texture2DSetting_filter"]) settings.filter = (TextureFilter)data["Texture2DSetting_filter"].as<int>();
    if(data["Texture2DSetting_mipmap"]) settings.mipmap = data["Texture2DSetting_mipmap"].as<bool>();
}

Ref<Texture2D> Texture2D::CreateFromFile(const char* filePath, Texture2DSetting settings){
    Ref<Texture2D> texture = CreateRef<Texture2D>();
    texture->Create(filePath, settings);
    return texture;
}

Ref<Texture2D> Texture2D::CreateFromFile(const char* filePath){
    Texture2DSetting settings;
    LoadSettings(filePath, settings);
    return CreateFromFile(filePath, settings);
}

}