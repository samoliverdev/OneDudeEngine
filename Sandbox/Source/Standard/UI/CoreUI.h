#pragma once
#include <OD/Core/Math.h>
#include <OD/Graphics/Texture.h>
#include <OD/Graphics/Font.h>
#include <OD/Graphics/Camera.h>

using namespace OD;

namespace Standard{

#define UIGlobalScale(x) roundf(x*UI::ComputeScale()) 

namespace UI{

Vector2& BaseResolution();
const Vector2& CurrentResolution();
float ComputeScale();
void SetLookInput(bool lockInputs);

void Init();
void Shotdown();

void Begin(Camera& camera);
void End();

int Width();
int Height();

void DrawPanel(const Ref<Texture2D>& tex, Vector2 pos, Vector2 size, float anchorX = 0.0f, float anchorY = 0.0f, Vector4 color = Vector4(1));
void DrawText(const char* text, const Ref<Font>& font, Vector2 pos, float size, float anchorX = 0, float anchorY = 0, Vector4 color = Vector4(1));
bool DrawButtom(const char* text, const Ref<Font>& font, const Ref<Texture2D>& tex, Vector2 pos, Vector2 size);

};

}