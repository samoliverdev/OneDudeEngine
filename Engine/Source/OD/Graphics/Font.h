#pragma once
#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"
#include "Texture.h"
#include "OD/Serialization/Serialization.h"
#include <map>
#include <msdf-atlas-gen.h>

namespace sol{ class state; }

namespace OD{
    
class Graphics;

struct MSDFData{
    std::vector<msdf_atlas::GlyphGeometry> glyphs;
    msdf_atlas::FontGeometry fontGeometry;
};

struct OD_API TextMetrics {
    Vector2 size;
    int lineCount;
};

class OD_API Font: public Asset{
    friend class OpenGLGraphicsDevice;
public:
    //Fixme opengl texture memory leak
    struct OD_API Character {
        //unsigned int textureID;  // ID handle of the glyph texture
        glm::ivec2 textureCoords;
        glm::ivec2   size;       // Size of glyph
        glm::ivec2   bearing;    // Offset from baseline to left/top of glyph
        unsigned int advance;    // Offset to advance to next glyph
    };

    //Font() = default;
    //Font(const std::string& inPath);

    void OnGui() override;
    
    static Ref<Font> CreateFromFile(const std::string& filepath);
    
    bool LoadFromFile(const std::string& path) override;

    inline std::vector<std::string> GetFileAssociations() override { 
        return std::vector<std::string>{
            ".ttf"
        }; 
    }

    static void CreateLuaBind(sol::state& lua);

    TextMetrics CalculateTextMetrics(const std::string& text);

    /*template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(path));
    }*/

private:
    struct MSDFData* data;
    std::map<char, Character> characters; //Fixme opengl texture memory leak
    Ref<Texture2D> fontAtlas;
};

}
