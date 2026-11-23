#pragma once
#include "RendererFeature.h"
#include "OD/Serialization/SerializationCore.h"

namespace OD{

class Framebuffer;
class Material;
class Texture2D;

class OD_API SSAOFeature: public RendererFeatureBase<SSAOFeature>, RenderPass{
public:
    float intensity = 0.5f;
    float radius = 0.5f;
    float bias = 0.025f;
    bool publishGlobalAO = false;
    bool compose = true;
    bool useBentNormals = false;

    enum class ResolutionMode{Full, Half, Quarter};
    ResolutionMode resolutionMode = ResolutionMode::Full;

    enum class UpsampleMode{Bilateral, Guided, SurfacePlane};
    UpsampleMode upsampleMode = UpsampleMode::Bilateral;

    enum class DebugView { Off, AmbientOcclusion, BentNormals };
    DebugView debugView = DebugView::Off;

    SSAOFeature();
    void AddRenderPasses(IRenderer& renderer, RenderContext& context) override;
    void Execute(Scene& scene, RenderContext& context, RenderFrameData& data) override;

    Ref<Framebuffer> GenerateAO(RenderContext& context, bool debugBentNormals = false);
    void DrawDebug(const Ref<Framebuffer>& ao);
    void OnGui() override;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDump(ar, CEREAL_NVP(enable));
        ArchiveDump(ar, CEREAL_NVP(intensity));
        ArchiveDump(ar, CEREAL_NVP(radius));
        ArchiveDump(ar, CEREAL_NVP(bias));
        ArchiveDump(ar, CEREAL_NVP(publishGlobalAO));
        ArchiveDump(ar, CEREAL_NVP(compose));
        ArchiveDump(ar, CEREAL_NVP(useBentNormals));
        ArchiveDump(ar, CEREAL_NVP(resolutionMode));
        ArchiveDump(ar, CEREAL_NVP(upsampleMode));
    }

private:
    Ref<Material> aoPass;
    Ref<Texture2D> noise;
    std::vector<glm::vec4> ssaoKernel;
};

};