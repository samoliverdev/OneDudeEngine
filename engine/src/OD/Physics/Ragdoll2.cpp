#include "Ragdoll2.h"
#include "Phys.h"

namespace OD{

void ProsseBodyParts(std::vector<RagdollBodyPart>& inBodyParts, SkinnedModelRendererComponent& skinnedComponent, TransformComponent& transformComponent){
    Transform globalTransform = Transform(
        transformComponent.GlobalModelMatrix() * 
        skinnedComponent.localTransform.GetLocalModelMatrix() * 
        skinnedComponent.GetModel()->skeleton.GetBindPose().GetGlobalMatrix(0)
    );
    Pose& pose = skinnedComponent.GetModel()->skeleton.GetBindPose(); 

    for(auto& bodyPart: inBodyParts){
        std::vector<int> child = pose.GetChildrens(bodyPart.animatorBone);

        if(bodyPart.animatorBone < 0) continue;
        //if(pose.GetParent(bodyPart.animatorBone) < 0) continue;
        if(child.size() < 1) continue;
        
        Vector3 p0 = globalTransform.TransformPoint( pose.GetGlobalTransform(bodyPart.animatorBone).LocalPosition() );
        //Vector3 p1 = globalTransform.TransformPoint( pose.GetGlobalTransform(pose.GetParent(bodyPart.animatorBone)).LocalPosition() );
        Vector3 p1 = globalTransform.TransformPoint( pose.GetGlobalTransform(child[0]).LocalPosition() );

        bodyPart.trans = Transform( globalTransform.GetLocalModelMatrix() * pose.GetGlobalTransform(bodyPart.animatorBone).GetLocalModelMatrix() );
        bodyPart.pivot = p0;

        bodyPart.center = p0;
        bodyPart.rot = pose.GetGlobalTransform(bodyPart.animatorBone).LocalRotation();
        //bodyPart.rot = QuaternionIdentity;

        bodyPart.frameA = Vector3(0);
        bodyPart.frameB = Vector3(0);
        bodyPart.frameARot = pose.GetLocalTransform(bodyPart.animatorBone).LocalRotation();
        bodyPart.frameBRot = pose.GetLocalTransform(bodyPart.animatorBone).LocalRotation();

        if(bodyPart.collisionType == RagdollBodyPart::CollisionType::Capsule){
            float distance = math::distance(p0, p1);
            float distance2 = math::distance(p0, p1) - (bodyPart.radius*2);
            bodyPart.height = distance2;

            Vector3 dir = math::normalize(p1 - p0) * (distance*0.5f);
            bodyPart.center = p0 + dir;

            bodyPart.frameA = Vector3(0, distance/2, 0);
            bodyPart.frameB = Vector3(0, -distance/2, 0);

            //bodyPart.rot = Quaternion(Vector3(math::radians(math::angle(Vector3Down, p1-p0)), 0, math::radians(math::angle(Vector3Down, p1-p0))));
        } else {
            bodyPart.frameA = Vector3(0, bodyPart.size.y/2, 0);
            bodyPart.frameB = Vector3(0, -bodyPart.size.y/2, 0);

            //bodyPart.pivot += Vector3(0, bodyPart.size.y/2, 0);
        }

        bodyPart.trans = Transform(bodyPart.center, bodyPart.rot, Vector3One);
    }
}

Ragdoll2::Ragdoll2(btDynamicsWorld* inWorld, std::vector<RagdollBodyPart> inBodyParts):world(inWorld), bodyParts(inBodyParts){
    for(auto& i: bodyParts){
        if(i.collisionType == RagdollBodyPart::CollisionType::Box){
            shapes.push_back(new btBoxShape(ToBullet(i.size)));
        } else if(i.collisionType == RagdollBodyPart::CollisionType::Capsule){
            shapes.push_back(new btCapsuleShape(btScalar(i.radius), btScalar(i.height)));
        }

        btTransform trans;
        trans.setIdentity();
        trans.setOrigin(ToBullet(i.center));
        trans.setRotation(ToBullet(i.rot));
        
        bodies.push_back( localCreateRigidBody(btScalar(1.), trans, shapes[shapes.size()-1]) );
    }

    /*
    for(int i = 0; i < bodies.size(); ++i){
        bodies[i]->setLinearFactor(btVector3(0.0f, 0.0f, 0.0f));
        bodies[i]->setActivationState(0);
        bodies[i]->setCollisionFlags(bodies[i]->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
    }

    for(int i = 0; i < bodies.size(); ++i){
        bodies[i]->setGravity(btVector3(0, -20.0f, 0));
        bodies[i]->setFriction(btScalar(0.8));
        bodies[i]->setDamping(btScalar(0.05), btScalar(0.85));
        bodies[i]->setDeactivationTime(btScalar(0.8));
        bodies[i]->setSleepingThresholds(btScalar(1.6), btScalar(2.5));
    }
    */

    ///*
    bodies[0]->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
    for(int i = 0; i < bodies.size(); ++i){
		bodies[i]->setDamping(0.05f, 0.85f);
        bodies[i]->setActivationState(DISABLE_DEACTIVATION);
		continue;
		bodies[i]->setDeactivationTime(0.8f);
		bodies[i]->setSleepingThresholds(1.6f, 2.5f);
	}

    bool useLinearReferenceFrameA = true;

    for(int i = 0; i < bodyParts.size(); i++){
        if(bodyParts[i].parent < 0) continue;

        btVector3 worldPoint = ToBullet(bodyParts[ i ].pivot);
        btTransform inverse_a = bodies[bodyParts[i].parent]->getWorldTransform().inverse();
        btTransform inverse_b = bodies[i]->getWorldTransform().inverse();

        btTransform localA, localB;
        localA.setIdentity();
        localA.setOrigin(inverse_a * worldPoint);
        localB.setIdentity();
        localB.setOrigin(inverse_b * worldPoint);

        btGeneric6DofConstraint* joint6DOF = new btGeneric6DofConstraint(
            *bodies[bodyParts[i].parent],
            *bodies[i], 
            localA, 
            localB,
            useLinearReferenceFrameA
        );
        
        joints.push_back(joint6DOF);
        //joint6DOF->setAngularLowerLimit(btVector3(-SIMD_PI*0.3f,-SIMD_EPSILON,-SIMD_PI*0.3f));
		//joint6DOF->setAngularUpperLimit(btVector3(SIMD_PI*0.5f,SIMD_EPSILON,SIMD_PI*0.3f));
        //joint6DOF->setAngularLowerLimit(btVector3(-SIMD_PI*0.1f, -SIMD_PI*0.05f, -SIMD_PI*0.1f));
		//joint6DOF->setAngularUpperLimit(btVector3(SIMD_PI*0.1f, SIMD_PI*0.05f, SIMD_PI*0.1f));
		world->addConstraint(joints[joints.size()-1], true);
    }
    //*/
}

btRigidBody* Ragdoll2::localCreateRigidBody(btScalar mass, const btTransform& startTransform, btCollisionShape* shape){
	bool isDynamic = (mass != 0.f);

	btVector3 localInertia(0,0,0);
	if(isDynamic) shape->calculateLocalInertia(mass,localInertia);

	btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,shape,localInertia);
	rbInfo.m_additionalDamping = true;
	btRigidBody* body = new btRigidBody(rbInfo);

	world->addRigidBody(body);

	return body;
}

}