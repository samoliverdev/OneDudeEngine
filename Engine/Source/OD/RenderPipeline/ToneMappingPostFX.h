#pragma once
#include "RendererFeature.h"
#include "OD/Base.h"
#include "OD/Serialization/Serialization.h"

namespace OD{

class Material;

class OD_API ToneMappingFeature: public RendererFeatureBase<ToneMappingFeature>, RenderPass{
public:
    enum class Mode{ None = -1, ACES, Neutral, Reinhard };

    Mode mode = Mode::None;
    float exposure = 1;

    ToneMappingFeature();
    void AddRenderPasses(IRenderer& renderer, RenderContext& context) override;
    void Execute(Scene& scene, RenderContext& context, RenderFrameData& data) override;
    void OnGui() override;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(enable));
        ArchiveDump(ar, CEREAL_NVP(mode));
        ArchiveDump(ar, CEREAL_NVP(exposure));
    }

private:
    Ref<Material> copyPass;
    Ref<Material> toneMappingReinhardPass;
    Ref<Material> toneMappingNeutralPass;
    Ref<Material> toneMappingACESPass;
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