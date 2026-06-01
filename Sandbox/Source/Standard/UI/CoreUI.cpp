#include "CoreUI.h"
#include <OD/Graphics/Font.h>
#include <OD/Graphics/Material.h>
#include <OD/Graphics/Mesh.h>
#include <OD/Graphics/Graphics.h>
#include <OD/Core/Input.h>
#include <OD/Core/Application.h>
#include <OD/Platform/Platform.h>

//using namespace OD;
namespace Standard{

Ref<Material> fontMat;
Ref<Mesh> spriteMesh;
Ref<Material> spriteMat;
Camera cam;

Ref<OD::Font> font;
Ref<OD::Texture2D> whiteTex;

Vector2 baseResolution = {1920, 1080}; //{1280, 720};
Vector2 currentResolution = {1280, 720};

float fontSize = 25;

static char *temp_render_buffer = NULL;
static int temp_render_buffer_len = 0;

bool lockInputs = false;

inline Matrix4 MakeUITransform(Vector2 pos, Vector2 size, float anchorX, float anchorY){
    // Convert from top-left (0,0) to bottom-left
    Vector2 p = pos;
    p.y = (float)cam.height - p.y;

    // UI anchor (0=left/top, 0.5=center, 1=right/bottom)
    float correctedAnchorY = 1.0f - anchorY;

    p.x -= size.x * (anchorX - 0.5f);
    p.y -= size.y * (correctedAnchorY - 0.5f);

    return math::translate(Vector3(p, 0.0f)) * math::scale(Vector3(size, 1.0f));
}

namespace UI{

Vector2& BaseResolution(){
    return baseResolution;
}

const Vector2& CurrentResolution(){
    return currentResolution;
}

float ComputeScale(){
    float sx = currentResolution.x / baseResolution.x;
    float sy = currentResolution.y / baseResolution.y;
    return (sx + sy) * 0.5f;

    //float scale = std::min(sx, sy); Use the smaller value (preserve fit)
    //float scale = std::max(sx, sy); Use the bigger value (preserve readability)
}

void SetLookInput(bool inlockInputs){
    lockInputs = inlockInputs;
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
    currentResolution = {incamera.width, incamera.height};

    //currentGuiScale = GUI_ComputeImGuiScale({incamera.width, incamera.height});
}

void End(){
}

int Width(){
    return cam.width;
}

int Height(){
    return cam.height;
}

void DrawPanel(const Ref<Texture2D>& tex, Vector2 pos, Vector2 size, float anchorX, float anchorY, Vector4 color){
    spriteMat->SetVector4("color", color);
    spriteMat->SetTexture("mainTex", tex);

    Matrix4 model = MakeUITransform(pos, size, anchorX, anchorY);
    Graphics::DrawMesh(*spriteMesh, *spriteMat, model);
}

void DrawText(const char* text, const Ref<Font>& font, Vector2 pos, float size, float anchorX, float anchorY, Vector4 color){
    Vector2 p = pos;
    p.y = (float)cam.height - p.y;

    // Get relative text size (in font normalized space) and convert to pixels by * size
    Vector2 textSize = font->CalculateTextMetrics(text).size * size;

    // Apply anchor
    p.x -= textSize.x * anchorX;
    p.y += textSize.y * anchorY;

    /*auto metrics = font->CalculateTextMetrics(text);
    Vector2 textSize = metrics.size * size;

    // Apply anchor
    p.x -= textSize.x * anchorX;

    // Fix Y: Text metrics size includes ascender and descender, 
    // but the draw baseline is aligned to ascender by default
    float baseline = metrics.ascenderY * size;
    p.y -= baseline; // Shift text baseline to top (like panel)

    // Apply anchorY from top
    p.y -= textSize.y * anchorY;*/

    // Build model matrix with translation and scale (uniform scale = font height in pixels)
    Matrix4 model = math::translate(Vector3(p, 0.0f)) * math::scale(Vector3(size));

    fontMat->SetVector4("color", color);
    Graphics::DrawText(*font, *fontMat, text, model, true, {});
}

bool DrawButtom(const char* text, const Ref<Font>& font, const Ref<Texture2D>& tex, Vector2 pos, Vector2 size){
    // Draw panel background with top-left pivot
    DrawPanel(tex, pos, size, 0, 0); 

    // ----- Text -----
    // Scale: the text height will match around 50% of button height (adjust to taste)
    float textScale = size.y * 0.9f;

    // Measure text size in pixels
    Vector2 textSize = font->CalculateTextMetrics(text).size * textScale;

    /*// Compute text position to center it inside the button
    Vector2 textPos = {
        pos.x + (size.x - textSize.x) * 0.5f, // center X
        pos.y + (size.y - textSize.y) * 0.5f  // center Y
    };
    // Draw text — top-left pivot (0,0) as you specified
    DrawText(text, font, textPos, textScale, 0.0f, 0.0f);*/

    // Compute text position to center it inside the button
    Vector2 textPos = {
        pos.x + size.x * 0.5f, // center X
        pos.y + size.y * 0.5f  // center Y
    };
    // Draw text — center pivot (0.5, 0.5)
    DrawText(text, font, textPos, textScale, 0.5f, 0.5f);

    // ----- Mouse Handling -----
    double mouseX, mouseY;
    Input::GetMousePosition(&mouseX, &mouseY);

    // Convert mouse Y (because UI Y axis is top-down)
    //mouseY = (double)cam.height - mouseY;

    //LogWarning("Mouse x: %f y:%f", mouseX, mouseY);

    // Hit test: simple AABB
    bool hovered = mouseX >= pos.x && mouseX <= (pos.x + size.x) &&
                   mouseY >= pos.y && mouseY <= (pos.y + size.y);

    // Click detection
    bool clicked = hovered && Input::IsMouseButtonDown(MouseButton::Left);

    return clicked;
}

};

}