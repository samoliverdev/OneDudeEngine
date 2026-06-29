#include "Renderer2D.h"
#include <OD/Graphics/Font.h>
#include <OD/Graphics/Material.h>
#include <OD/Graphics/Mesh.h>
#include <OD/Graphics/Graphics.h>
#include <OD/Core/Input.h>
#include <OD/Core/Application.h>
#include <OD/Platform/Platform.h>

//using namespace OD;
namespace Standard{

namespace Renderer2D{

Ref<Material> fontMat;
Ref<Mesh> spriteMesh;
Ref<Material> spriteMat;
Camera cam;

Ref<OD::Font> font;
Ref<OD::Texture2D> whiteTex;

float fontSize = 25;

static char *temp_render_buffer = NULL;
static int temp_render_buffer_len = 0;

inline Matrix4 MakeUITransform(Vector2 pos, Vector2 size, Vector2 pivot){
    // Convert from top-left (0,0) to bottom-left
    Vector2 p = pos;
    p.y = (float)cam.height - p.y;

    // UI anchor (0=left/top, 0.5=center, 1=right/bottom)
    float correctedAnchorY = 1.0f - pivot.y;

    p.x -= size.x * (pivot.x - 0.5f);
    p.y -= size.y * (correctedAnchorY - 0.5f);

    return math::translate(Vector3(p, 0.0f)) * math::scale(Vector3(size, 1.0f));
}

void Init(){
    //font = OD::Font::CreateFromFile("Engine/Fonts/OpenSans/static/OpenSans-Regular.ttf", {8*1, FontType::MSDF});
    font =  AssetManager::Get().LoadAsset<Font>("Engine/Fonts/OpenSans/static/OpenSans-Regular.ttf", FontSettings{8*3, FontType::MSDF});// OD::Font::CreateFromFile("SandboxGame/Fonts/Coolvetica/Coolvetica Rg Cond.otf", {8*3, FontType::MSDF});
    fontMat = OD::CreateRef<OD::Material>(OD::Shader::CreateFromFile("Engine/Shaders/FontMSDF.glsl"));
    fontMat->SetFloat("pxRange", font->MsdfPxRange());
    
    spriteMat = OD::CreateRef<OD::Material>(OD::Shader::CreateFromFile("Engine/Shaders/Sprite.glsl"));
    whiteTex = OD::Texture2D::CreateFromFile("Engine/Textures/White.jpg", {});

    spriteMesh = CreateRef<Mesh>();
    spriteMesh->vertices = {
        {-0.5f, -0.5f, 0},
        {-0.5f,  0.5f, 0},
        { 0.5f, -0.5f, 0},
        { 0.5f,  0.5f, 0},
    };
    spriteMesh->uv = {
        {0, 0, 0},
        {0, 1, 0},
        {1, 0, 0},
        {1, 1, 0},
    };
    spriteMesh->drawMode = MeshDrawMode::TRIANGLES_STRIP;
    spriteMesh->Submit();

}

void Shotdown(){
    fontMat = nullptr;
    spriteMesh = nullptr;
    spriteMat = nullptr;
    font = nullptr;
    whiteTex = nullptr;
}

void Begin(Camera& incamera){
    cam = incamera;
    Graphics::SetCamera(cam);
}

void End(){
}

void DrawPanel(const Ref<Texture2D>& tex, Vector2 pos, Vector2 size, Vector2 pivot, Vector4 color){
    spriteMat->SetVector4("color", color);
    spriteMat->SetTexture("mainTex", tex);

    Matrix4 model = MakeUITransform(pos, size, pivot);
    Graphics::DrawMesh(*spriteMesh, *spriteMat, model);
}

void DrawText(const char* text, const Ref<Font>& font, Vector2 pos, float size, Vector2 pivot, Vector4 color){
    Vector2 p = pos;
    p.y = (float)cam.height - p.y;

    // Get relative text size (in font normalized space) and convert to pixels by * size
    Vector2 textSize = font->CalculateTextMetrics(text).size * size;

    // Apply anchor
    p.x -= textSize.x * pivot.x;
    p.y += textSize.y * pivot.y;

    // Build model matrix with translation and scale (uniform scale = font height in pixels)
    Matrix4 model = math::translate(Vector3(p, 0.0f)) * math::scale(Vector3(size));

    fontMat->SetVector4("color", color);
    Graphics::DrawText(*font, *fontMat, text, model, true, {});
}

};

}