#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Serialization/CerealImGui.h"
#include "PostFX.h"

namespace OD{

//TODO: Add blur
class OD_API SSAOPostFX: public PostFX{
public:
    float intensity = 0.5f;
    float radius = 0.5f;
    float bias = 0.025f;

    SSAOPostFX();
    void OnRenderImage(Framebuffer* src, Framebuffer* dst, RenderContext* context) override;

    inline void OnGui() override {
        cereal::ImGuiArchive gui;
        gui(*this);
    }

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(enable));
        ArchiveDump(ar, CEREAL_NVP(intensity));
        ArchiveDump(ar, CEREAL_NVP(radius));
        ArchiveDump(ar, CEREAL_NVP(bias));
    }

private:
    Ref<Material> aoPass;
    Ref<Texture2D> noise;
    std::vector<glm::vec4> ssaoKernel;
};

};