#include <gtest/gtest.h>
#include <OD/RenderPipeline/RenderData.h>

namespace OD {
namespace {

Frustum MakeRejectedFrustum(){
    Frustum frustum;
    const Plane reject(glm::vec4(1.0f, 0.0f, 0.0f, -10.0f));
    frustum.leftFace = reject;
    frustum.rightFace = reject;
    frustum.topFace = reject;
    frustum.bottomFace = reject;
    frustum.nearFace = reject;
    frustum.farFace = reject;
    return frustum;
}

TEST(RenderDataCullingTest, BuildsIndependentVisibilityBits){
    AABB bounds(Vector3(0.0f), 1.0f, 1.0f, 1.0f);
    const std::vector<RenderDataCullingView> views = {
        {Frustum{}, 1u << 0},
        {MakeRejectedFrustum(), 1u << 2},
        {Frustum{}, 1u << 20}
    };

    EXPECT_EQ(ComputeRenderDataCullingMask(bounds, views, false), (1u << 0) | (1u << 20));
}

TEST(RenderDataCullingTest, AlwaysDrawSetsEveryActiveVisibilityBit){
    AABB bounds(Vector3(0.0f), 1.0f, 1.0f, 1.0f);
    const std::vector<RenderDataCullingView> views = {
        {MakeRejectedFrustum(), 1u << 0},
        {Frustum{}, 1u << 4},
        {MakeRejectedFrustum(), 1u << 20}
    };

    EXPECT_EQ(ComputeRenderDataCullingMask(bounds, views, true), (1u << 0) | (1u << 4) | (1u << 20));
}

TEST(RenderDataCullingTest, EmptyViewsProduceAnEmptyMask){
    AABB bounds(Vector3(0.0f), 1.0f, 1.0f, 1.0f);
    const std::vector<RenderDataCullingView> views;

    EXPECT_EQ(ComputeRenderDataCullingMask(bounds, views, false), 0u);
    EXPECT_EQ(ComputeRenderDataCullingMask(bounds, views, true), 0u);
}

} // namespace
} // namespace OD
