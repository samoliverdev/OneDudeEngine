#pragma once

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletDynamics/Dynamics/btDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

namespace OD{

inline Vector3 FromBullet(btVector3 v){ return Vector3(v.x(), v.y(), v.z()); }
inline btVector3 ToBullet(Vector3 v){ return btVector3(v.x, v.y, v.z); }

inline btQuaternion ToBullet(Quaternion v){ return btQuaternion(v.x, v.y, v.z, v.w); }

inline btTransform ToBullet(Transform v){
    btTransform t;
    t.setIdentity();
    t.setOrigin(btVector3(v.LocalPosition().x, v.LocalPosition().y, v.LocalPosition().z));
    t.setRotation(btQuaternion(v.LocalRotation().x, v.LocalRotation().y, v.LocalRotation().z, v.LocalRotation().w));
    return t;
}

}