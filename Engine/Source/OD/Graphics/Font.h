#pragma once
#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"
#include "OD/Serialization/Serialization.h"
#include <map>

namespace sol{ class state; }

namespace OD{

class Graphics;

//Fixme opengl texture memory leak
struct OD_API Character {
    unsigned int textureID;  // ID handle of the glyph texture
    glm::ivec2   size;       // Size of glyph
    glm::ivec2   bearing;    // Offset from baseline to left/top of glyph
    unsigned int advance;    // Offset to advance to next glyph
};
    
struct OD_API Font: public Asset{
    friend class Graphics;

    //Font() = default;
    //Font(const std::string& inPath);
    
    static Ref<Font> CreateFromFile(const std::string& filepath);
    
    bool LoadFromFile(const std::string& path) override;

    inline std::vector<std::string> GetFileAssociations() override { 
        return std::vector<std::string>{
            ".ttf"
        }; 
    }

    static void CreateLuaBind(sol::state& lua);

    /*template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(path));
    }*/

private:
    std::map<char, Character> characters; //Fixme opengl texture memory leak
};

}
