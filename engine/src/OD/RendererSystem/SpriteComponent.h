#pragma once

#include "OD/Scene/Scene.h"
#include "OD/Renderer/Texture.h"
#include "StandRendererSystem.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"

#include <vector>
#include <algorithm>

namespace OD{

struct SpriteComponent{
    Ref<Texture2D> texture = nullptr;

    static void OnGui(Entity& e);
};

};