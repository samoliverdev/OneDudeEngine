#include <gtest/gtest.h>
#include <OD/Base.h>
#include <OD/Animation/Pose.h>
namespace OD {
OD_API void BlendRagdollLocalPoseInPlace(Pose&, Pose&, float);
}
using namespace OD;

static Pose MakePose(float rootX,float legLength){
    Pose pose(3);
    pose.SetParent(0,-1); pose.SetParent(1,0); pose.SetParent(2,1);
    pose.SetLocalTransform(0,Transform(Vector3(rootX,0,0),QuaternionIdentity,Vector3One));
    pose.SetLocalTransform(1,Transform(Vector3(0,legLength,0),QuaternionIdentity,Vector3One));
    pose.SetLocalTransform(2,Transform(Vector3(0,-legLength,0),QuaternionIdentity,Vector3One));
    return pose;
}

TEST(RagdollInPlacePoseTest, ZeroLeavesCurrentFinalPoseUntouched){
    auto current=MakePose(2,1),physical=MakePose(10,2);
    BlendRagdollLocalPoseInPlace(current,physical,0);
    EXPECT_EQ(current.GetLocalTransform(0).Position(),Vector3(2,0,0));
    EXPECT_EQ(current.GetLocalTransform(1).Position(),Vector3(0,1,0));
}
TEST(RagdollInPlacePoseTest, OneReplacesCurrentFinalPoseWithPhysics){
    auto current=MakePose(2,1),physical=MakePose(10,2);
    BlendRagdollLocalPoseInPlace(current,physical,1);
    EXPECT_EQ(current.GetLocalTransform(0).Position(),Vector3(10,0,0));
    EXPECT_EQ(current.GetLocalTransform(1).Position(),Vector3(0,2,0));
}
TEST(RagdollInPlacePoseTest, HalfBlendsLocalBonesWithoutCompoundingParentTranslation){
    auto current=MakePose(2,1),physical=MakePose(10,2);
    BlendRagdollLocalPoseInPlace(current,physical,0.5f);
    EXPECT_EQ(current.GetLocalTransform(0).Position(),Vector3(6,0,0));
    EXPECT_EQ(current.GetLocalTransform(1).Position(),Vector3(0,1.5f,0));
    EXPECT_EQ(current.GetLocalTransform(2).Position(),Vector3(0,-1.5f,0));
    EXPECT_LT(glm::length(current.GetGlobalTransform(2).Position()-Vector3(6,0,0)),0.0001f);
    EXPECT_EQ(physical.GetLocalTransform(0).Position(),Vector3(10,0,0));
}
TEST(RagdollInPlacePoseTest, ReadsTheCurrentInputInsteadOfAPreviousAnimationSnapshot){
    auto current=MakePose(2,1),physical=MakePose(10,2);
    current.SetLocalTransform(0,Transform(Vector3(6,0,0),QuaternionIdentity,Vector3One));
    BlendRagdollLocalPoseInPlace(current,physical,0.5f);
    EXPECT_EQ(current.GetLocalTransform(0).Position(),Vector3(8,0,0));
}
