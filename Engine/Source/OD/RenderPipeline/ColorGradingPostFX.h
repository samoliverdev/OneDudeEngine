#pragma once
#include "PostFX.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Serialization/CerealImGui.h"
#include <type_traits>

namespace OD{

class Material;

class OD_API ColorGradingPostFX: public PostFX{
public:

    float postExposure = 0;
    float contrast = 0;
    Vector4 colorFilter = Vector4(1,1,1,1);
    float hueShift = 0;
    float saturation = 0;

    ColorGradingPostFX();
    void OnRenderImage(class Framebuffer* src, class Framebuffer* dst, class RenderContext* context) override;

    inline void OnGui() override {
        cereal::ImGuiArchive colorGradring;
        colorGradring(*this);
    }

    template <class Archive>
    void serialize(Archive& ar){
        if(std::is_same<Archive, cereal::ImGuiArchive>::value){
            cereal::ImGuiArchive& ar2 = (cereal::ImGuiArchive&)ar;
            ar2.setOption("contrast", cereal::ImGuiArchive::Options(-100, 100, 1));
            cereal::ImGuiArchive::Options c;
            c.isColor = true;
            ar2.setOption("colorFilter", c);
        }

        ArchiveDump(ar, CEREAL_NVP(enable));
        ArchiveDump(ar, CEREAL_NVP(postExposure));
        ArchiveDump(ar, CEREAL_NVP(contrast));
        ArchiveDump(ar, CEREAL_NVP(colorFilter));
        ArchiveDump(ar, CEREAL_NVP(hueShift));
        ArchiveDump(ar, CEREAL_NVP(saturation));

        contrast = math::clamp<float>(contrast, -100, 100);
        hueShift = math::clamp<float>(hueShift, -180, 180);
        saturation = math::clamp<float>(saturation, -100, 100);

        if(std::is_same<Archive, cereal::ImGuiArchive>::value){
            cereal::ImGuiArchive& ar2 = (cereal::ImGuiArchive&)ar;
            ar2.CleanOptions();
        }
    }

private:
    Ref<Material> colorGradingPass;
};

}