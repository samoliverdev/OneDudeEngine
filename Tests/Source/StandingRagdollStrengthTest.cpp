#include <gtest/gtest.h>
#include <OD/Base.h>
#include <OD/Core/Math.h>
namespace OD {
OD_API Vector3 ComputeRagdollAnchorAcceleration(Vector3, Vector3, float, float, float = 1.0f);
}

using namespace OD;


TEST(StandingRagdollStrengthTest, StrongerHipsReduceDisplacementUnderTheSameLoad){
    const float dt=1.0f/60;
    auto errorUnderLoad=[&](float strength){
        Vector3 position(0), velocity(0);
        for(int i=0;i<600;++i){
            auto a=ComputeRagdollAnchorAcceleration(-position,-velocity,1,dt,strength);
            velocity+=(a+Vector3(0,-40,0))*dt;
            position+=velocity*dt;
        }
        return glm::length(position);
    };
    EXPECT_LT(errorUnderLoad(4),errorUnderLoad(1));
}

TEST(StandingRagdollStrengthTest, IncreasedStrengthSettlesWithoutExplodingAtDifferentRates){
    for(float dt:{1.0f/30,1.0f/60,1.0f/120}) for(float strength:{1,2,4,8,16}){
        Vector3 p(1,2,3),v(-4,1,2);
        for(int i=0;i<int(3/dt);++i){
            v+=ComputeRagdollAnchorAcceleration(-p,-v,1,dt,strength)*dt;
            p+=v*dt;
        }
        EXPECT_LT(glm::length(p),0.001f);
        EXPECT_LT(glm::length(v),0.001f);
    }
}
