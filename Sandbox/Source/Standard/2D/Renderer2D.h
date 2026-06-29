#pragma once
#include <OD/Core/Math.h>
#include <OD/Graphics/Texture.h>
#include <OD/Graphics/Font.h>
#include <OD/Graphics/Camera.h>

using namespace OD;

namespace Standard{

namespace Renderer2D{
    
void Init();
void Shotdown();

void Begin(Camera& camera);
void End();

void DrawPanel(const Ref<Texture2D>& tex, Vector2 pos, Vector2 size, Vector2 pivot = {0, 0}, Vector4 color = Vector4(1));
void DrawText(const char* text, const Ref<Font>& font, Vector2 pos, float size, Vector2 pivot = {0, 0}, Vector4 color = Vector4(1));

};

}