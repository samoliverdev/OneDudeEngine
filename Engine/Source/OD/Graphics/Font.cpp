#include "Font.h"
#include "OD/Core/Application.h"
#include "OD/Core/Lua.h"
#include "OD/Core/ImGui.h"
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

Ref<Font> Font::CreateFromFile(const std::string& filepath){
    Ref<Font> font = CreateRef<Font>();
    if(font->LoadFromFile(filepath) == false) return nullptr;
    return font;
}

void Font::OnGui(){
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

    float aspect = fontAtlas->Width() / fontAtlas->Height();
    ImGui::Image(fontAtlas->RenderId(), ImVec2(viewportPanelSize.x, viewportPanelSize.x * aspect), ImVec2(0, 1), ImVec2(1, 0));
}

/*
Ref<Font> Font::CreateFromFile(const char* path){
    Ref<Font> font = CreateRef<Font>();
    //font->LoadFromFile(path);
    //return font;

    InitFreeFont();

    FT_Face face;
    if(FT_New_Face(ft, path, 0, &face)){
        LogError("ERROR::FREETYPE: Failed to load font");  
        Application::Quit();
        return nullptr;
    }

    FT_Set_Pixel_Sizes(face, 0, 48); 

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

        font->characters.insert(std::pair<char, Character>(c, character));
    }

    FT_Done_Face(face);
    //FT_Done_FreeType(ft);

    font->path = std::string(path);

    return font;
}
*/

template<typename T, typename S, int N, msdf_atlas::GeneratorFunction<S, N> GenFunc>
static Ref<Texture2D> CreateAndCacheAtlas(
    const std::string& fontName, float fontSize, const std::vector<msdf_atlas::GlyphGeometry>& glyphs,
    const msdf_atlas::FontGeometry& fontGeometry, uint32_t width, uint32_t height)
{
    msdf_atlas::GeneratorAttributes attributes;
    attributes.config.overlapSupport = true;
    attributes.scanlinePass = true;

    msdf_atlas::ImmediateAtlasGenerator<S, N, GenFunc, msdf_atlas::BitmapAtlasStorage<T, N>> generator(width, height);
    generator.setAttributes(attributes);
    generator.setThreadCount(8);
    generator.generate(glyphs.data(), (int)glyphs.size());

    msdfgen::BitmapConstRef<T, N> bitmap = (msdfgen::BitmapConstRef<T, N>)generator.atlasStorage();

    /*TextureSpecification spec;
    spec.Width = bitmap.width;
    spec.Height = bitmap.height;
    spec.Format = ImageFormat::RGB8;
    spec.GenerateMips = false;

    Ref<Texture2D> texture = Texture2D::Create(spec);
    texture->SetData((void*)bitmap.pixels, bitmap.width * bitmap.height * 3);
    return texture;*/

    Texture2DSetting setting;
    setting.mipmap = false;
    setting.textureFormat = TextureFormat::RGB8;
    return Texture2D::CreateFromRaw(
        (void*)bitmap.pixels, 
        (bitmap.width * bitmap.height * 3), 
        bitmap.width, bitmap.height, 
        TextureDataType::UnsignedByte, setting
    );
}

bool Font::LoadFromFile(const std::string& inPath){
    /*InitFreeFont();

    const int fontSize = 48;

    FT_Face face;
    if(FT_New_Face(ft, inPath.c_str(), 0, &face)){
        LogError("ERROR::FREETYPE: Failed to load font: %s", inPath.c_str());  
        Application::Quit();
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, fontSize);*/ 

    /*glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
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
            //texture, 
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            (unsigned int)face->glyph->advance.x
        };

        characters.insert(std::pair<char, Character>(c, character));
    }*/

    /*int padding = 2;
    int row = 0;
    int col = padding;

    const int textureWidth = 512;
    char* textureBuffer = new char[textureWidth  * textureWidth];

    //for(char glyphIdx = 0; glyphIdx < 128; ++glyphIdx){
    for(char glyphIdx = 32; glyphIdx < 127; ++glyphIdx){
        FT_UInt glyphIndex = FT_Get_Char_Index(face, glyphIdx);
        FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT);
        FT_Error error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

        if(col + face->glyph->bitmap.width + padding >= 512){
            col = padding;
            row += fontSize;
        }

        for(unsigned int y = 0; y < face->glyph->bitmap.rows; ++y){
            for(unsigned int x = 0; x < face->glyph->bitmap.width; ++x){
                textureBuffer[(row + y) * textureWidth + col + x] = face->glyph->bitmap.buffer[y * face->glyph->bitmap.width + x];
            }
        }

        Character character = {
            //texture, 
            {col, row},
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            (unsigned int)face->glyph->advance.x
        };
        characters.insert(std::pair<char, Character>(glyphIdx, character));

        col += face->glyph->bitmap.width + padding;
    }

    FT_Done_Face(face);
    //FT_Done_FreeType(ft);

    Texture2DSetting setting;
    setting.mipmap = false;
    setting.textureFormat = TextureFormat::RED8;
    fontAtlas = Texture2D::CreateFromRaw(
        textureBuffer, 
        sizeof(char) * (textureWidth * textureWidth), 
        textureWidth, textureWidth, 
        TextureDataType::UnsignedByte, setting
    );
    Assert(fontAtlas != nullptr);

    delete textureBuffer;
    path = inPath;
    return true;*/

    path = inPath;

    using namespace msdf_atlas;
    
    data = new MSDFData();

    bool success = false;
    // Initialize instance of FreeType library
    if(msdfgen::FreetypeHandle *ft = msdfgen::initializeFreetype()){
        // Load font file
        if(msdfgen::FontHandle *font = msdfgen::loadFont(ft, path.c_str())){
            data->fontGeometry = FontGeometry(&data->glyphs);

            // Storage for glyph geometry and their coordinates in the atlas
            //std::vector<GlyphGeometry> glyphs;
            // FontGeometry is a helper class that loads a set of glyphs from a single font.
            // It can also be used to get additional font metrics, kerning information, etc.
            //FontGeometry fontGeometry(&glyphs);
            // Load a set of character glyphs:
            // The second argument can be ignored unless you mix different font sizes in one atlas.
            // In the last argument, you can specify a charset other than ASCII.
            // To load specific glyph indices, use loadGlyphs instead.
            data->fontGeometry.loadCharset(font, 1.0f, Charset::ASCII);
            // Apply MSDF edge coloring. See edge-coloring.h for other coloring strategies.
            const double maxCornerAngle = 3.0;
            for(GlyphGeometry &glyph : data->glyphs){
                glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);
            }
            // TightAtlasPacker class computes the layout of the atlas.
            TightAtlasPacker packer;
            // Set atlas parameters:
            // setDimensions or setDimensionsConstraint to find the best value
            packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
            // setScale for a fixed size or setMinimumScale to use the largest that fits
            packer.setMinimumScale(40.0);
            // setPixelRange or setUnitRange
            packer.setPixelRange(2.0);
            packer.setMiterLimit(1.0);
            // Compute atlas layout - pack glyphs
            packer.pack(data->glyphs.data(), data->glyphs.size());
            //packer.setDimensions(2048, 2048);
            // Get final atlas dimensions
            int width = 0, height = 0;
            packer.getDimensions(width, height);
            // The ImmediateAtlasGenerator class facilitates the generation of the atlas bitmap.
            ImmediateAtlasGenerator<
                float, // pixel type of buffer for individual glyphs depends on generator function
                3, // number of atlas color channels
                msdfGenerator, // function to generate bitmaps for individual glyphs
                BitmapAtlasStorage<byte, 3> // class that stores the atlas bitmap
                // For example, a custom atlas storage class that stores it in VRAM can be used.
            > generator(width, height);
            // GeneratorAttributes can be modified to change the generator's default settings.
            GeneratorAttributes attributes;
            generator.setAttributes(attributes);
            generator.setThreadCount(4);
            // Generate atlas bitmap
            generator.generate(data->glyphs.data(), data->glyphs.size());
            // The atlas bitmap can now be retrieved via atlasStorage as a BitmapConstRef.
            // The glyphs array (or fontGeometry) contains positioning data for typesetting text.
            //success = my_project::submitAtlasBitmapAndLayout(generator.atlasStorage(), glyphs);

            auto bitmap = (msdfgen::BitmapConstRef<byte, 3>)generator.atlasStorage();

            Texture2DSetting setting;
            setting.mipmap = false;
            setting.textureFormat = TextureFormat::RGB8;
            fontAtlas = Texture2D::CreateFromRaw(
                (void*)bitmap.pixels, 
                (bitmap.width * bitmap.height * 3), 
                bitmap.width, bitmap.height, 
                TextureDataType::UnsignedByte, setting
            );
            success = true;

            // Cleanup
            msdfgen::destroyFont(font);
        }
        msdfgen::deinitializeFreetype(ft);
    }
    return success;
}

TextMetrics Font::CalculateTextMetrics(const std::string& text, const TextParams& textParams){
    /*
    const auto& fontGeometry = data->fontGeometry;
    const auto& metrics = fontGeometry.getMetrics();
    double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);

    double width = 0.0;
    double maxWidth = 0.0;
    double height = fsScale * metrics.lineHeight;
    bool firstLine = true;

    const float spaceGlyphAdvance = fontGeometry.getGlyph(' ')->getAdvance();

    for (size_t i = 0; i < text.size(); i++) {
        char character = text[i];
        if (character == '\r') continue;

        if (character == '\n') {
            maxWidth = std::max(maxWidth, width);
            width = 0;
            height += fsScale * metrics.lineHeight;
            firstLine = false;
            continue;
        }

        if (character == ' ') {
            double advance = spaceGlyphAdvance;
            if (i < text.size() - 1) {
                char next = text[i + 1];
                double dAdvance;
                fontGeometry.getAdvance(dAdvance, character, next);
                advance = dAdvance;
            }
            width += fsScale * advance;
            continue;
        }

        if (character == '\t') {
            width += 4.0 * (fsScale * spaceGlyphAdvance);
            continue;
        }

        auto glyph = fontGeometry.getGlyph(character);
        if (!glyph)
            glyph = fontGeometry.getGlyph('?');
        if (!glyph)
            continue;

        double advance = glyph->getAdvance();
        if (i < text.size() - 1) {
            char next = text[i + 1];
            fontGeometry.getAdvance(advance, character, next);
        }
        width += fsScale * advance;
    }

    maxWidth = std::max(maxWidth, width);
    return OD::Vector2((float)maxWidth, (float)height);
    */

    /*const auto& fontGeometry = data->fontGeometry;
    const auto& metrics = fontGeometry.getMetrics();
    double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);

    double x = 0.0;
    double y = 0.0;
    double maxX = 0.0;
    int lineCount = 1; // Start with one line

    for (size_t i = 0; i < text.size(); i++) {
        char c = text[i];

        if (c == '\r') continue;

        if (c == '\n') {
            maxX = std::max(maxX, x);
            x = 0;
            y += metrics.lineHeight * fsScale;
            lineCount++;
            continue;
        }

        if (c == ' ') {
            double advance;
            if (i < text.size() - 1) {
                char next = text[i + 1];
                fontGeometry.getAdvance(advance, c, next);
            } else {
                advance = fontGeometry.getGlyph(' ')->getAdvance();
            }
            x += advance * fsScale;
            continue;
        }

        if (c == '\t') {
            double advance = fontGeometry.getGlyph(' ')->getAdvance();
            x += 4 * advance * fsScale;
            continue;
        }

        const auto* glyph = fontGeometry.getGlyph(c);
        if (!glyph) {
            glyph = fontGeometry.getGlyph('?');
            if (!glyph) continue;
        }

        double advance;
        if (i < text.size() - 1) {
            char next = text[i + 1];
            fontGeometry.getAdvance(advance, c, next);
        } else {
            advance = glyph->getAdvance();
        }

        x += advance * fsScale;
        maxX = std::max(maxX, x);
    }

    double totalHeight = metrics.lineHeight * fsScale * lineCount;

    return {
        Vector2((float)maxX, (float)totalHeight),
        lineCount
    };*/

    const auto& fontGeometry = data->fontGeometry;
    const auto& metrics = fontGeometry.getMetrics();
    double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);

    double x = 0.0;
    double y = 0.0;
    double maxX = 0.0;
    int lineCount = 1;

    const float spaceAdvance = fontGeometry.getGlyph(' ')->getAdvance();

    for (size_t i = 0; i < text.size(); i++) {
        char character = text[i];
        if (character == '\r') continue;

        if (character == '\n') {
            maxX = std::max(maxX, x);
            x = 0;
            y += fsScale * metrics.lineHeight + textParams.lineSpacing;
            lineCount++;
            continue;
        }

        if (character == ' ') {
            float advance = spaceAdvance;
            if (i < text.size() - 1) {
                char nextCharacter = text[i + 1];
                double dAdvance;
                fontGeometry.getAdvance(dAdvance, character, nextCharacter);
                advance = (float)dAdvance;
            }
            x += fsScale * advance + textParams.kerning;
            continue;
        }

        if(character == '\t'){
            // NOTE(Yan): is this right?
            x += 4.0f * (fsScale * spaceAdvance + textParams.kerning);
            continue;
        }

        auto glyph = fontGeometry.getGlyph(character);
        if (!glyph) glyph = fontGeometry.getGlyph('?');
        if (!glyph) continue;

        double pl, pb, pr, pt;
        glyph->getQuadPlaneBounds(pl, pb, pr, pt);
        double width = (pr - pl) * fsScale;

        double advance = glyph->getAdvance();
        if (i < text.size() - 1) {
            char nextCharacter = text[i + 1];
            fontGeometry.getAdvance(advance, character, nextCharacter);
        }

        x += fsScale * advance;
    }

    maxX = std::max(maxX, x);
    double height = lineCount * fsScale * metrics.lineHeight;

    return { Vector2((float)maxX, (float)height), lineCount, metrics.ascenderY, metrics.descenderY };
}

void Font::CreateLuaBind(sol::state& lua){
    lua.new_usertype<Font>(
        "Font",
        "LoadFromFile", &Font::LoadFromFile,
        "GetFileAssociations", &Font::GetFileAssociations
    );
}

}