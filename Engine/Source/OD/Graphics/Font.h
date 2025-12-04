#pragma once
#include "OD/Defines.h"
#include "OD/Core/Asset.h"
#include "OD/Core/Math.h"
#include "OD/Serialization/Serialization.h"
#include <map>
#include <msdf-atlas-gen.h>

namespace sol{ class state; }

namespace OD{
    class Texture2D;
};

namespace OD{
    
class Graphics;

struct MSDFData{
    std::vector<msdf_atlas::GlyphGeometry> glyphs;
    msdf_atlas::FontGeometry fontGeometry;
};

struct OD_API TextMetrics {
    Vector2 size;
    int lineCount;
    double ascenderY;
    double descenderY;
};

struct OD_API TextParams{
    float kerning = 0.0f;
    float lineSpacing = 0.0f;
};

enum class FontType { 
    Raster, SDF, MSDF 
};

struct OD_API FontSettings{
    float pixelSize = 16.0f;
    FontType type = FontType::MSDF;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, pixelSize);
        ArchiveDumpNVP(ar, type);
    }
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
    
    static Ref<Font> CreateFromFile(const std::string& filepath, const FontSettings& settings = {});
    
    bool LoadFromFile(const std::string& path) override;

    inline std::vector<std::string> GetFileAssociations() override { 
        return std::vector<std::string>{
            ".ttf"
        }; 
    }

    static void CreateLuaBind(sol::state& lua);

    TextMetrics CalculateTextMetrics(const std::string& text, const TextParams& params = {});
    TextMetrics CalculateTextMetrics(const std::string& text, float pixelSize, const TextParams& params = {});

    /*template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(path));
    }*/

    inline const FontSettings Settings(){ return settings; }
    inline void Settings(const FontSettings& insettings){ settings = insettings; }

    inline MSDFData* Data(){ return data; }
    inline float MsdfPxRange(){ return msdfPxRange; }

private:
    MSDFData* data;
    std::map<char, Character> characters; //Fixme opengl texture memory leak
    Ref<Texture2D> fontAtlas;
    FontSettings settings;

    float msdfPxRange = 0;
};

}
