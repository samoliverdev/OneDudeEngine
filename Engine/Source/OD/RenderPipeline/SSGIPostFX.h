#pragma once
#include "OD/Defines.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Serialization/CerealImGui.h"
#include "PostFX.h"

namespace OD{

class OD_API SSGIPostFX: public PostFX{
public:
    SSGIPostFX();
    void OnRenderImage(Framebuffer* src, Framebuffer* dst, RenderContext* context) override;

    inline void OnGui() override {
        cereal::ImGuiArchive gui;
        gui(*this);
    }

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(enable));
        
        ArchiveDump(ar, CEREAL_NVP(sampleRadius));
        ArchiveDump(ar, CEREAL_NVP(hitThickness));
        ArchiveDump(ar, CEREAL_NVP(sampleCount));
        ArchiveDump(ar, CEREAL_NVP(sliceCount));
    }

private:
    Ref<Material> giPass;

    float sampleRadius = 1;
    float hitThickness = 0.5f;
    float sampleCount = 4;
    float sliceCount = 4;
    
};

};