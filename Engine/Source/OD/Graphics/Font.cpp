#include "OD/pch.h"
#include "Font.h"
#include "Texture.h"
#include "OD/Core/Application.h"
#include "OD/Core/Lua.h"
#include "OD/Core/ImGui.h"
#include <ft2build.h>
#include FT_FREETYPE_H 

#include <glm/gtx/integer.hpp>

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

/*Ref<Font> Font::CreateFromFile(const std::string& filepath, const FontSettings& settings){
    Ref<Font> font = CreateRef<Font>();
    font->Settings(settings);
    if(font->LoadFromFile(filepath) == false) return nullptr;
    return font;
}*/

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

uint32_t NextPow2(uint32_t x) {
    if (x <= 1) return 1;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;

    //return glm::ceilPowerOfTwo(x);
}

/* 
//INFO: Possible New API Desing
enum class FontUnicodePreset{
    ASCII,
    Latin,
    LatinCyrillic,
    European,
    CJK,
    AllCommon
};

void BuildCharset(msdf_atlas::Charset& charset, FontUnicodePreset preset){
    auto AddRange = [&](uint32_t from, uint32_t to){
        for(uint32_t c = from; c <= to; ++c)
            charset.add(c);
    };

    switch(preset){
        case FontUnicodePreset::ASCII:
            AddRange(0x20, 0x7E);
            break;

        case FontUnicodePreset::Latin:
            AddRange(0x20, 0x017F);
            break;

        case FontUnicodePreset::LatinCyrillic:
            AddRange(0x20, 0x017F);
            AddRange(0x0400, 0x04FF);
            break;

        case FontUnicodePreset::European:
            AddRange(0x20, 0x017F);
            AddRange(0x0370, 0x03FF);
            AddRange(0x0400, 0x04FF);
            break;

        case FontUnicodePreset::CJK:
            AddRange(0x20, 0x017F);
            AddRange(0x3040, 0x30FF);
            AddRange(0x4E00, 0x9FFF);
            break;

        case FontUnicodePreset::AllCommon:
            AddRange(0x20, 0x017F);
            AddRange(0x0370, 0x03FF);
            AddRange(0x0400, 0x04FF);
            AddRange(0x3040, 0x30FF);
            AddRange(0x4E00, 0x9FFF);
            break;
    }
}*/

bool Font::LoadFromFile(const std::string& inPath){
    /*path = inPath;

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
            //packer.setScale(2);
            // Set atlas parameters:
            // setDimensions or setDimensionsConstraint to find the best value
            
            // === Derived from fontSize ===
            float fontSize = 32;
            float scale      = fontSize;
            float pixelRange = std::clamp(fontSize * 0.20f, 4.0f, 16.0f);
            int atlasSize    = NextPow2((int)(fontSize * 64));
            packer.setDimensions(atlasSize, atlasSize);
            packer.setMinimumScale(scale);
            packer.setPixelRange(pixelRange);
            packer.setMiterLimit(1.0);

            //packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
            //packer.setMinimumScale(40.0);
            //packer.setPixelRange(2.0*1);
            //packer.setMiterLimit(1.0);

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
    return success;*/

    path = inPath;
    using namespace msdf_atlas;

    data = new MSDFData();
    bool success = false;

    auto BuildCharset = [&](msdfgen::FontHandle* font){
        msdf_atlas::Charset charset;

        auto AddRange = [&](uint32_t from, uint32_t to){
            for(uint32_t c = from; c <= to; ++c)
                charset.add(c);
        };

        AddRange(0x0020, 0x007E); // Basic Latin
        AddRange(0x00A0, 0x00FF); // Latin-1
        AddRange(0x0100, 0x017F); // Latin Extended
        AddRange(0x0400, 0x04FF); // Cyrillic
        //AddRange(0x0370, 0x03FF); // Greek
        //AddRange(0x3040, 0x30FF); // Japanese kana
        //AddRange(0x4E00, 0x9FFF); // Chinese (BIG)

        /*for(uint32_t c = 0x20; c <= 0x7E; ++c){
            charset.add(c);
        }
        for(uint32_t c = 0xA0; c <= 0x017F; ++c){ // Extended Latin
            charset.add(c);
        }*/

        data->fontGeometry.loadCharset(font, 1.0f, charset, true, true);
    };

    if(msdfgen::FreetypeHandle *ft = msdfgen::initializeFreetype()){
        if(msdfgen::FontHandle *font = msdfgen::loadFont(ft, path.c_str())){
            data->fontGeometry = FontGeometry(&data->glyphs);
            
            //data->fontGeometry.loadCharset(font, 1.0f, Charset::ASCII);
            BuildCharset(font);

            // Edge coloring only needed for MSDF/SDF
            if(settings.type == FontType::SDF || settings.type == FontType::MSDF){
                const double maxCornerAngle = 3.0;
                for(GlyphGeometry &glyph : data->glyphs){
                    glyph.edgeColoring(
                        &msdfgen::edgeColoringInkTrap,
                        maxCornerAngle, 0
                    );
                }
            }

            // === Derived from font size ===
            float fontSize = settings.pixelSize;
            float scale = fontSize;
            float pixelRange = std::clamp(fontSize * 0.20f, 4.0f, 16.0f);
            int atlasSize = NextPow2((int)(fontSize * (64/2)));

            msdfPxRange = pixelRange;

            TightAtlasPacker packer;

            packer.setDimensions(atlasSize, atlasSize);
            //packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
            packer.setMinimumScale(scale);
            packer.setPixelRange(pixelRange);
            packer.setMiterLimit(1.0);

            /*packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
            packer.setMinimumScale(40.0);
            packer.setPixelRange(2.0*1);
            packer.setMiterLimit(1.0);*/

            // Compute atlas layout
            packer.pack(data->glyphs.data(), data->glyphs.size());

            int width = 0, height = 0;
            packer.getDimensions(width, height);

            TextureFormat texFormat;
            int channelCount = 1;

            // --------------------------
            // SELECT GENERATOR BY TYPE
            // --------------------------
            if(settings.type == FontType::Raster){
                // RASTER MODE
                ImmediateAtlasGenerator<
                    float,
                    1,
                    scanlineGenerator,
                    BitmapAtlasStorage<byte,1>
                > generator(width, height);

                generator.setThreadCount(4);
                generator.generate(data->glyphs.data(), data->glyphs.size());

                auto bitmap = (msdfgen::BitmapConstRef<byte,1>)generator.atlasStorage();
                texFormat = TextureFormat::RED8;
                channelCount = 1;

                Texture2DSetting setting;
                setting.mipmap = false;
                setting.textureFormat = texFormat;
                setting.filter = TextureFilter::Linear;
                setting.wrap = TextureWrapping::ClampToEdge;

                fontAtlas = Texture2D::CreateFromRaw(
                    (void*)bitmap.pixels,
                    //bitmap.width * bitmap.height * channelCount,
                    bitmap.width, bitmap.height,
                    TextureDataType::UnsignedByte,
                    setting, "RASTER_ATLAS"
                );
            } else if(settings.type == FontType::SDF){
                // SDF MODE
                ImmediateAtlasGenerator<
                    float,
                    1,
                    sdfGenerator,
                    BitmapAtlasStorage<byte,1>
                > generator(width, height);

                generator.setThreadCount(4);
                generator.generate(data->glyphs.data(), data->glyphs.size());

                auto bitmap = (msdfgen::BitmapConstRef<byte,1>)generator.atlasStorage();
                texFormat = TextureFormat::RED8;
                channelCount = 1;

                Texture2DSetting setting;
                setting.mipmap = false;
                setting.textureFormat = texFormat;
                setting.filter = TextureFilter::Linear;
                setting.wrap = TextureWrapping::ClampToEdge;

                fontAtlas = Texture2D::CreateFromRaw(
                    (void*)bitmap.pixels,
                    //bitmap.width * bitmap.height * channelCount,
                    bitmap.width, bitmap.height,
                    TextureDataType::UnsignedByte,
                    setting, "SDF_ATLAS"
                );
            } else if(settings.type == FontType::MSDF){
                // MSDF MODE
                ImmediateAtlasGenerator<
                    float,
                    3,
                    msdfGenerator,
                    BitmapAtlasStorage<byte,3>
                > generator(width, height);

                generator.setThreadCount(4);
                generator.generate(data->glyphs.data(), data->glyphs.size());

                auto bitmap = (msdfgen::BitmapConstRef<byte,3>)generator.atlasStorage();
                texFormat = TextureFormat::RGB8;
                channelCount = 3;

                Texture2DSetting setting;
                setting.mipmap = false;
                setting.textureFormat = texFormat;
                setting.filter = TextureFilter::Linear;
                setting.wrap = TextureWrapping::ClampToEdge;

                fontAtlas = Texture2D::CreateFromRaw(
                    (void*)bitmap.pixels,
                    //bitmap.width * bitmap.height * channelCount,
                    bitmap.width, bitmap.height,
                    TextureDataType::UnsignedByte,
                    setting, "MSDF_ATLAS"
                );
            }

            success = true;
            msdfgen::destroyFont(font);
        }

        msdfgen::deinitializeFreetype(ft);
    }

    return success;
}

bool Font::LoadFromPackage(const std::string& inPath, Package& package){
    void* _data = nullptr;
    size_t size;
    if(package.ReadFileData(inPath.c_str(), _data, size) == false){
        package.FreeFileData(_data);
        return false;
    }
    
    path = inPath;
    using namespace msdf_atlas;

    data = new MSDFData();
    bool success = false;

    auto BuildCharset = [&](msdfgen::FontHandle* font){
        msdf_atlas::Charset charset;

        auto AddRange = [&](uint32_t from, uint32_t to){
            for(uint32_t c = from; c <= to; ++c)
                charset.add(c);
        };

        AddRange(0x0020, 0x007E); // Basic Latin
        AddRange(0x00A0, 0x00FF); // Latin-1
        AddRange(0x0100, 0x017F); // Latin Extended
        AddRange(0x0400, 0x04FF); // Cyrillic
        //AddRange(0x0370, 0x03FF); // Greek
        //AddRange(0x3040, 0x30FF); // Japanese kana
        //AddRange(0x4E00, 0x9FFF); // Chinese (BIG)

        /*for(uint32_t c = 0x20; c <= 0x7E; ++c){
            charset.add(c);
        }
        for(uint32_t c = 0xA0; c <= 0x017F; ++c){ // Extended Latin
            charset.add(c);
        }*/

        data->fontGeometry.loadCharset(font, 1.0f, charset, true, true);
    };

    if(msdfgen::FreetypeHandle *ft = msdfgen::initializeFreetype()){
        if(msdfgen::FontHandle *font = msdfgen::loadFontData(ft, (const msdfgen::byte*)_data, size)){
            data->fontGeometry = FontGeometry(&data->glyphs);
            
            //data->fontGeometry.loadCharset(font, 1.0f, Charset::ASCII);
            BuildCharset(font);

            // Edge coloring only needed for MSDF/SDF
            if(settings.type == FontType::SDF || settings.type == FontType::MSDF){
                const double maxCornerAngle = 3.0;
                for(GlyphGeometry &glyph : data->glyphs){
                    glyph.edgeColoring(
                        &msdfgen::edgeColoringInkTrap,
                        maxCornerAngle, 0
                    );
                }
            }

            // === Derived from font size ===
            float fontSize = settings.pixelSize;
            float scale = fontSize;
            float pixelRange = std::clamp(fontSize * 0.20f, 4.0f, 16.0f);
            int atlasSize = NextPow2((int)(fontSize * (64/2)));

            msdfPxRange = pixelRange;

            TightAtlasPacker packer;

            packer.setDimensions(atlasSize, atlasSize);
            //packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
            packer.setMinimumScale(scale);
            packer.setPixelRange(pixelRange);
            packer.setMiterLimit(1.0);

            /*packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
            packer.setMinimumScale(40.0);
            packer.setPixelRange(2.0*1);
            packer.setMiterLimit(1.0);*/

            // Compute atlas layout
            packer.pack(data->glyphs.data(), data->glyphs.size());

            int width = 0, height = 0;
            packer.getDimensions(width, height);

            TextureFormat texFormat;
            int channelCount = 1;

            // --------------------------
            // SELECT GENERATOR BY TYPE
            // --------------------------
            if(settings.type == FontType::Raster){
                // RASTER MODE
                ImmediateAtlasGenerator<
                    float,
                    1,
                    scanlineGenerator,
                    BitmapAtlasStorage<byte,1>
                > generator(width, height);

                generator.setThreadCount(4);
                generator.generate(data->glyphs.data(), data->glyphs.size());

                auto bitmap = (msdfgen::BitmapConstRef<byte,1>)generator.atlasStorage();
                texFormat = TextureFormat::RED8;
                channelCount = 1;

                Texture2DSetting setting;
                setting.mipmap = false;
                setting.textureFormat = texFormat;
                setting.filter = TextureFilter::Linear;
                setting.wrap = TextureWrapping::ClampToEdge;

                fontAtlas = Texture2D::CreateFromRaw(
                    (void*)bitmap.pixels,
                    //bitmap.width * bitmap.height * channelCount,
                    bitmap.width, bitmap.height,
                    TextureDataType::UnsignedByte,
                    setting, "RASTER_ATLAS"
                );
            } else if(settings.type == FontType::SDF){
                // SDF MODE
                ImmediateAtlasGenerator<
                    float,
                    1,
                    sdfGenerator,
                    BitmapAtlasStorage<byte,1>
                > generator(width, height);

                generator.setThreadCount(4);
                generator.generate(data->glyphs.data(), data->glyphs.size());

                auto bitmap = (msdfgen::BitmapConstRef<byte,1>)generator.atlasStorage();
                texFormat = TextureFormat::RED8;
                channelCount = 1;

                Texture2DSetting setting;
                setting.mipmap = false;
                setting.textureFormat = texFormat;
                setting.filter = TextureFilter::Linear;
                setting.wrap = TextureWrapping::ClampToEdge;

                fontAtlas = Texture2D::CreateFromRaw(
                    (void*)bitmap.pixels,
                    //bitmap.width * bitmap.height * channelCount,
                    bitmap.width, bitmap.height,
                    TextureDataType::UnsignedByte,
                    setting, "SDF_ATLAS"
                );
            } else if(settings.type == FontType::MSDF){
                // MSDF MODE
                ImmediateAtlasGenerator<
                    float,
                    3,
                    msdfGenerator,
                    BitmapAtlasStorage<byte,3>
                > generator(width, height);

                generator.setThreadCount(4);
                generator.generate(data->glyphs.data(), data->glyphs.size());

                auto bitmap = (msdfgen::BitmapConstRef<byte,3>)generator.atlasStorage();
                texFormat = TextureFormat::RGB8;
                channelCount = 3;

                Texture2DSetting setting;
                setting.mipmap = false;
                setting.textureFormat = texFormat;
                setting.filter = TextureFilter::Linear;
                setting.wrap = TextureWrapping::ClampToEdge;

                fontAtlas = Texture2D::CreateFromRaw(
                    (void*)bitmap.pixels,
                    //bitmap.width * bitmap.height * channelCount,
                    bitmap.width, bitmap.height,
                    TextureDataType::UnsignedByte,
                    setting, "MSDF_ATLAS"
                );
            }

            success = true;
            msdfgen::destroyFont(font);
        }

        msdfgen::deinitializeFreetype(ft);
    }

    package.FreeFileData(_data);
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

    for(size_t i = 0; i < text.size(); i++){
        char character = text[i];
        if(character == '\r') continue;

        if(character == '\n'){
            maxX = std::max(maxX, x);
            x = 0;
            y += fsScale * metrics.lineHeight + textParams.lineSpacing;
            lineCount++;
            continue;
        }

        if(character == ' '){
            float advance = spaceAdvance;
            if(i < text.size() - 1){
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
        if(!glyph) glyph = fontGeometry.getGlyph('?');
        if(!glyph) continue;

        double pl, pb, pr, pt;
        glyph->getQuadPlaneBounds(pl, pb, pr, pt);
        double width = (pr - pl) * fsScale;

        double advance = glyph->getAdvance();
        if(i < text.size() - 1){
            char nextCharacter = text[i + 1];
            fontGeometry.getAdvance(advance, character, nextCharacter);
        }

        x += fsScale * advance;
    }

    maxX = std::max(maxX, x);
    double height = lineCount * fsScale * metrics.lineHeight;

    return { Vector2((float)maxX, (float)height), lineCount, metrics.ascenderY, metrics.descenderY };
}

TextMetrics Font::CalculateTextMetrics(const std::string& text, float pixelSize, const TextParams& params){
    const auto& fontGeometry = data->fontGeometry;
    const auto& metrics = fontGeometry.getMetrics();

    // EM → Pixel conversion scale
    const double pxScale = (double)pixelSize / (metrics.ascenderY - metrics.descenderY);

    double x = 0.0;
    double maxX = 0.0;
    int lineCount = 1;

    // Get space width (in EM units)
    const double spaceAdvanceEM = fontGeometry.getGlyph(' ')->getAdvance();

    for(size_t i = 0; i < text.size(); i++){

        char c = text[i];
        if (c == '\r') continue;

        // New line
        if (c == '\n') {
            maxX = std::max(maxX, x);
            x = 0.0;
            lineCount++;
            continue;
        }

        // Space
        if (c == ' ') {
            double adv = spaceAdvanceEM;
            if (i < text.size() - 1) {
                double kerned;
                fontGeometry.getAdvance(kerned, c, text[i+1]);
                adv = kerned;
            }
            x += adv;
            continue;
        }

        // Tab = 4 spaces
        if (c == '\t') {
            x += 4.0 * spaceAdvanceEM;
            continue;
        }

        // Glyph
        const auto* glyph = fontGeometry.getGlyph(c);
        if (!glyph) glyph = fontGeometry.getGlyph('?');
        if (!glyph) continue;

        double adv = glyph->getAdvance();
        if (i < text.size() - 1) {
            fontGeometry.getAdvance(adv, c, text[i + 1]);
        }

        x += adv;
    }

    maxX = std::max(maxX, x);

    // Convert EM → pixels
    double widthPx  = maxX * pxScale;
    double heightPx = (metrics.lineHeight * lineCount) * pxScale +
                      (lineCount - 1) * params.lineSpacing;

    return {
        Vector2((float)widthPx, (float)heightPx),
        lineCount,
        metrics.ascenderY,
        metrics.descenderY
    };
}

void Font::CreateLuaBind(sol::state& lua){
    lua.new_usertype<Font>(
        "Font",
        "LoadFromFile", &Font::LoadFromFile,
        "GetFileAssociations", &Font::GetFileAssociations
    );
}

}