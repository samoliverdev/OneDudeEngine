#include "Font.h"
#include "OD/Core/Application.h"
#include "OD/Platform/GL.h"

#include <ft2build.h>
#include FT_FREETYPE_H 

namespace OD{

bool freeFontHasInited = false;
FT_Library ft;

void InitFreeFont(){
    if(freeFontHasInited == true) return;

    if (FT_Init_FreeType(&ft)){
        LogError("ERROR::FREETYPE: Could not init FreeType Library");
        Application::Quit();
        return;
    }

    freeFontHasInited = true;
}

Ref<Font> Font::CreateFromFile(const char* path){
    InitFreeFont();

    FT_Face face;
    if(FT_New_Face(ft, path, 0, &face)){
        LogError("ERROR::FREETYPE: Failed to load font");  
        Application::Quit();
        return nullptr;
    }

    FT_Set_Pixel_Sizes(face, 0, 48); 

    Ref<Font> font = CreateRef<Font>();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
    glCheckError();
  
    for(unsigned char c = 0; c < 128; c++){
        // load character glyph 
        if(FT_Load_Char(face, c, FT_LOAD_RENDER)){
            LogError("ERROR::FREETYTPE: Failed to load Glyph");
            continue;
        }
        // generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glCheckError();
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        glCheckError();
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glCheckError();
        // now store character for later use
        Character character = {
            texture, 
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            (unsigned int)face->glyph->advance.x
        };

        font->_characters.insert(std::pair<char, Character>(c, character));
    }

    FT_Done_Face(face);
    //FT_Done_FreeType(ft);

    font->_path = std::string(path);

    return font;
}

}