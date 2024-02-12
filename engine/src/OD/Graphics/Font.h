#pragma once

#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"

namespace OD{

class Graphics;

struct Character {
    unsigned int textureID;  // ID handle of the glyph texture
    glm::ivec2   size;       // Size of glyph
    glm::ivec2   bearing;    // Offset from baseline to left/top of glyph
    unsigned int advance;    // Offset to advance to next glyph
};
    
struct Font: public Asset{
    friend class Graphics;

    static Ref<Font> CreateFromFile(const char* path);

private:
    std::map<char, Character> characters;
};


}