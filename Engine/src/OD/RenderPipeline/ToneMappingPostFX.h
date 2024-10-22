#pragma once
#include "OD/Defines.h"
#include "RenderContext.h"
#include "OD/Serialization/Serialization.h"
#include <magic_enum/magic_enum.hpp>

namespace OD{

class OD_API ToneMappingPostFX: public PostFX{
public:
    enum class Mode{ None = -1, ACES, Neutral, Reinhard };

    Mode mode = Mode::None;
    float exposure = 1;

    ToneMappingPostFX();
    void OnRenderImage(Framebuffer* src, Framebuffer* dst) override;

    inline void OnGui() override {
        cereal::ImGuiArchive colorGradring;
        colorGradring(*this);
    }

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(enable));
        ArchiveDump(ar, CEREAL_NVP(mode));
        ArchiveDump(ar, CEREAL_NVP(exposure));
    }

private:
    Ref<Shader> copyPass;
    Ref<Shader> toneMappingReinhardPass;
    Ref<Shader> toneMappingNeutralPass;
    Ref<Shader> toneMappingACESPass;
};

}

/*namespace cereal{
    template <class Archive> inline
    std::string save_minimal(const Archive&, const OD::ToneMappingPostFX::Mode& t){
        return std::string(magic_enum::enum_name(t));
    }

    template <class Archive> inline
    void load_minimal(const Archive&, OD::ToneMappingPostFX::Mode& t, const std::string& value){
        t = magic_enum::enum_cast<OD::ToneMappingPostFX::Mode>(value).value_or(OD::ToneMappingPostFX::Mode::None);
    }
}*/