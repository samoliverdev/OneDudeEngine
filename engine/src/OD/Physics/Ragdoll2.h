#pragma once
#include <btBulletDynamicsCommon.h>
#include "RagdollBodyPart.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"
#include "OD/Scene/Scene.h"

namespace OD{

void ProsseBodyParts(std::vector<RagdollBodyPart>& inBodyParts, SkinnedModelRendererComponent& skinnedComponent, TransformComponent& transformComponent);

class Ragdoll2{
public:
    Ragdoll2(btDynamicsWorld* inWorld, std::vector<RagdollBodyPart> inBodyParts);
    ~Ragdoll2(){}
private:
    std::vector<RagdollBodyPart> bodyParts;

    btDynamicsWorld* world;

	std::vector<btCollisionShape*> shapes;
	std::vector<btRigidBody*> bodies;
	std::vector<btTypedConstraint*> joints;

    btRigidBody* localCreateRigidBody(btScalar mass, const btTransform& startTransform, btCollisionShape* shape);
};

}