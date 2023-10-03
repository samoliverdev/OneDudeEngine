#pragma once

#include "OD/Scene/Scene.h"
#include "OD/Renderer/Animations.h"
#include "OD/RendererSystem/MeshRendererComponent.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"

namespace OD{
struct AnimatorComponent{
    friend struct AnimatorSystem;

    static void Serialize(YAML::Emitter& out, Entity& e);
    static void Deserialize(YAML::Node& in, Entity& e);
    static void OnGui(Entity& e);

    void Play(Animation* animation);

private:
    Animator* anim = nullptr;
};

struct AnimatorSystem: public System{
    virtual SystemType Type() override { return SystemType::Stand; }

    virtual void Init(Scene* scene) override;
    virtual void Update() override;
};

}
