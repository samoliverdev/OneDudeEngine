#pragma once
#include "OD/Defines.h"
#include "OD/Scene/Scene.h"
#include "OD/Graphics/Texture.h"
#include "StandRenderPipeline.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"

#include <vector>
#include <algorithm>

namespace OD{

struct OD_API SpriteComponent{
    Ref<Texture2D> texture = nullptr;

    static void OnGui(Entity& e);
};

};