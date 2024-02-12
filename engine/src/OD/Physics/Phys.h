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

}