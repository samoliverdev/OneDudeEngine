#include "Cubemap.h"
#include "OD/Platform/GL.h"
#include <stb/stb_image.h>
#include <vector>

namespace OD{

bool Cubemap::CreateFromFile(Cubemap& out, const char* right, const char* left, const char* top, const char* bottom, const char* front, const char* back){
    std::vector<const char*> faces;
    faces.push_back(right);
    faces.push_back(left);
    faces.push_back(top);
    faces.push_back(bottom);
    faces.push_back(front);
    faces.push_back(back);

    glGenTextures(1, &out.renderId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, out.renderId);
    glCheckError();

    stbi_set_flip_vertically_on_load(0);

    int width, height, nrChannels;
    for(unsigned int i = 0; i < faces.size(); i++){
        unsigned char* data = stbi_load(faces[i], &width, &height, &nrChannels, 0);
        if(data){
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            glCheckError();
            stbi_image_free(data);
        } else {
            LogError("Cubemap tex failed to load at path: %s", faces[i]);
            //std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }

        LogInfo("Loading Cubemap: %s", faces[i]);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glCheckError();

    return true;
}

void Cubemap::Destroy(Cubemap& cubemap){
    if(cubemap.renderId != 0) glDeleteTextures(1, &cubemap.renderId);
    cubemap.renderId = 0;
    glCheckError();
}

void Cubemap::Bind(Cubemap& cubemap, int index){
    //glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap.renderId);
    glCheckError();
}

bool Cubemap::IsValid(){
    return renderId != 0;
}

}