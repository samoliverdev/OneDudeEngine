#include <gtest/gtest.h>
#include <OD/Base.h>
#include <OD/Core/Math.h>
namespace OD {
struct RagdollPoseTransform {
    Vector3 position;
    Quaternion rotation;
    Vector3 scale;
};
OD_API float ClampRagdollPoseWeight(float);
OD_API RagdollPoseTransform BlendRagdollPoseTransform(const RagdollPoseTransform&, const RagdollPoseTransform&, float);
}
using namespace OD;

static RagdollPoseTransform Animation(){ return {Vector3(1,2,3),glm::angleAxis(0.2f,Vector3(0,1,0)),Vector3(1,2,1)}; }
static RagdollPoseTransform Physics(){ return {Vector3(5,6,7),glm::angleAxis(1.2f,Vector3(0,1,0)),Vector3(2,1,3)}; }

TEST(RagdollPoseBlendTest, ZeroIsExactlyAnimationAndOneIsExactlyPhysics){
    auto a=Animation(),p=Physics();
    auto zero=BlendRagdollPoseTransform(a,p,0);
    auto one=BlendRagdollPoseTransform(a,p,1);
    EXPECT_EQ(zero.position,a.position); EXPECT_EQ(zero.rotation,a.rotation); EXPECT_EQ(zero.scale,a.scale);
    EXPECT_EQ(one.position,p.position); EXPECT_EQ(one.rotation,p.rotation); EXPECT_EQ(one.scale,p.scale);
}
TEST(RagdollPoseBlendTest, HalfBlendsPositionRotationAndScale){
    auto half=BlendRagdollPoseTransform(Animation(),Physics(),0.5f);
    EXPECT_LT(glm::length(half.position-Vector3(3,4,5)),0.0001f);
    EXPECT_NEAR(glm::abs(glm::dot(half.rotation,glm::angleAxis(0.7f,Vector3(0,1,0)))),1,0.0001f);
    EXPECT_LT(glm::length(half.scale-Vector3(1.5f,1.5f,2)),0.0001f);
}
TEST(RagdollPoseBlendTest, OutOfRangeWeightsClampToEndpoints){
    EXPECT_EQ(ClampRagdollPoseWeight(-1),0);
    EXPECT_EQ(ClampRagdollPoseWeight(2),1);
    EXPECT_EQ(BlendRagdollPoseTransform(Animation(),Physics(),-1).position,Animation().position);
}
TEST(RagdollPoseBlendTest, PreviousRenderedBlendCannotAffectNextBlend){
    auto a=Animation(),p=Physics();
    auto previous=BlendRagdollPoseTransform(a,p,0.75f);
    auto next=BlendRagdollPoseTransform(a,p,0.25f);
    EXPECT_LT(glm::length(next.position-Vector3(2,3,4)),0.0001f);
    EXPECT_EQ(a.position,Animation().position);
    EXPECT_EQ(p.position,Physics().position);
}
