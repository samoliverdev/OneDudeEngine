#pragma once
#include "OD/RenderPipeline/RendererFeature.h"
#include <memory>

namespace OD{
// Backend boundary: consumes only public data/settings and produces RenderData.
// GPU state belongs to the rendering context, never to physics/navigation/data.
class OD_API TerrainQuadtreeRenderer final : public RendererFeatureBase<TerrainQuadtreeRenderer>,
                                             public CameraRenderDataFeature{
public:
    TerrainQuadtreeRenderer();
    ~TerrainQuadtreeRenderer() override;
    void OnCollectCameraRenderData(Scene& scene,
                                   RenderContext& context,
                                   const Camera& camera,
                                   std::vector<RenderData>& out) override;

private:
    struct State;
    std::unique_ptr<State> state;
};
}
