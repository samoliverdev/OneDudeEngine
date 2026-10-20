#include <gtest/gtest.h>
#include <OD/Base.h>
#include <OD/Core/Math.h>
namespace OD {
OD_API Vector3 ComputeRagdollAnchorAcceleration(Vector3, Vector3, float, float, float = 1.0f);
OD_API Quaternion RagdollAnchorLockedRotation(Quaternion, Quaternion, float);
OD_API Vector3 RagdollAnchorAngularVelocity(Quaternion, Quaternion, float, float);
OD_API bool IsRagdollOwnerContact(uint32_t, int, uint32_t, int);
}

using namespace OD;

TEST(StandingRagdollTest, DynamicHipsReturnAfterAnImpulseAtDifferentPhysicsRates){
    for(float dt: {1.0f / 30, 1.0f / 60, 1.0f / 120}){
        Vector3 p(0), v(3, 2, -1), target(0, 1, 0);
        float largestError = 0;
        for(int step = 0; step < int(2 / dt); ++step){
            v += ComputeRagdollAnchorAcceleration(target - p, -v, 1, dt) * dt;
            p += v * dt;
            largestError = glm::max(largestError, glm::length(target - p));
        }
        EXPECT_LT(glm::length(p - target), 0.001f);
        EXPECT_LT(glm::length(v), 0.001f);
        EXPECT_LT(largestError, 2.0f);
    }
}

TEST(StandingRagdollTest, MovingTargetRemainsBoundedWithRA2DisplacementDamping){
    const float dt = 1.0f / 60;
    Vector3 p(0), v(0), target(0), targetVelocity(4, 1, 0);
    Vector3 previousPosition = p;
    for(int step = 0; step < 180; ++step){
        Vector3 physicalDelta = p - previousPosition;
        previousPosition = p;
        v += ComputeRagdollAnchorAcceleration(target - p, physicalDelta - v, 1, dt) * dt;
        p += v * dt;
        target += targetVelocity * dt;
    }
    EXPECT_LT(glm::length(p - target), 0.2f);
    EXPECT_LT(glm::length((p - previousPosition) / dt - targetVelocity), 0.002f);
}

TEST(StandingRagdollTest, ZeroAttachmentReleasesHipsAndInvalidTimestepIsSafe){
    EXPECT_EQ(ComputeRagdollAnchorAcceleration(Vector3(1), Vector3(2), 0, 1.0f / 60), Vector3(0));
    EXPECT_EQ(ComputeRagdollAnchorAcceleration(Vector3(1), Vector3(2), 1, 0), Vector3(0));
}

TEST(StandingRagdollTest, OwnerCapsuleDoesNotCollideWithItsOwnDynamicRagdoll){
    EXPECT_TRUE(IsRagdollOwnerContact(42, -1, 42, 0));
    EXPECT_TRUE(IsRagdollOwnerContact(42, 7, 42, -1));
    EXPECT_FALSE(IsRagdollOwnerContact(42, 0, 42, 3));
    EXPECT_FALSE(IsRagdollOwnerContact(42, -1, 43, 0));
    EXPECT_FALSE(IsRagdollOwnerContact(42, -1, 42, -1));
}

TEST(StandingRagdollTest, LowerAttachmentLeavesMorePhysicalFreedom){
    const float dt = 1.0f / 60;
    float loose = ComputeRagdollAnchorAcceleration(Vector3(1,0,0), Vector3(0), 0.25f, dt).x;
    float medium = ComputeRagdollAnchorAcceleration(Vector3(1,0,0), Vector3(0), 0.75f, dt).x;
    float tight = ComputeRagdollAnchorAcceleration(Vector3(1,0,0), Vector3(0), 1, dt).x;
    EXPECT_LT(loose, medium);
    EXPECT_LT(medium, tight);
}

TEST(StandingRagdollTest, OnePhysicsStepPullsCloseWithoutOvershooting){
    for(float dt: {1.0f / 30, 1.0f / 60, 1.0f / 120}){
        Vector3 velocity = ComputeRagdollAnchorAcceleration(Vector3(1,0,0), Vector3(0), 1, dt) * dt;
        float position = velocity.x * dt;
        EXPECT_GT(position, 0.85f);
        EXPECT_LT(position, 1.0f);
    }
}
TEST(StandingRagdollTest, LockedRotationUsesRA2FixedDeltaSlerp){
    Quaternion target = math::angleAxis(math::radians(90.0f), Vector3(0,1,0));
    Quaternion result = RagdollAnchorLockedRotation(QuaternionIdentity, target, 1.0f / 120);
    EXPECT_NEAR(std::abs(math::dot(result, math::slerp(QuaternionIdentity, target, 0.5f))), 1.0f, 0.00001f);
    EXPECT_NEAR(std::abs(math::dot(RagdollAnchorLockedRotation(QuaternionIdentity, target, 1.0f / 60), target)), 1.0f, 0.00001f);
}

TEST(StandingRagdollTest, UnlockedRotationMatchesRA2CrossAxisCorrection){
    Quaternion target = math::angleAxis(math::radians(60.0f), Vector3(0,1,0));
    Vector3 velocity = RagdollAnchorAngularVelocity(QuaternionIdentity, target, 0.5f, 1.0f / 60);
    EXPECT_LT(glm::length(velocity - Vector3(0, math::radians(60.0f) * 0.75f * 60.0f, 0)), 0.0001f);
    EXPECT_EQ(RagdollAnchorAngularVelocity(QuaternionIdentity, QuaternionIdentity, 1, 1.0f / 60), Vector3(0));
}