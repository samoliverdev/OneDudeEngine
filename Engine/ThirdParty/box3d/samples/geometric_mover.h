// SPDX-FileCopyrightText: 2026 Erin Catto
// SPDX-License-Identifier: MIT

#pragma once

#include "box3d/collision.h"
#include "box3d/types.h"

struct GeometricMoverDef
{
	b3Pos position = {};
	b3Capsule capsule = { { 0.0f, -0.5f, 0.0f }, { 0.0f, 0.5f, 0.0f }, 0.3f };
	float jumpSpeed = 5.0f;
	float maxSpeed = 6.0f;
	float minSpeed = 0.01f;
	float stopSpeed = 1.0f;
	float accelerate = 30.0f;
	float friction = 4.0f;
	float gravity = 15.0f;
	float pogoRestLength = 0.9f;
	float pogoHertz = 4.0f;
	float pogoDampingRatio = 0.7f;
};

// Optional shape user data read by GeometricMover when it gathers collision planes.
// Shapes without user data are treated as rigid, fully clipping, and pushable.
struct MoverShapeUserData
{
	// Distance limit on the push this surface may apply to the mover. FLT_MAX is rigid.
	float maxPush;

	// False keeps the velocity along this surface, which is what soft collision needs.
	bool clipVelocity;

	// False stops the mover from shoving this body.
	bool canMoverPush;
};

// Extra data kept alongside each collision plane so the mover can push rigid bodies.
struct PlaneExtra
{
	b3Pos point;
	b3ShapeId shapeId;
};

// Geometric character mover. The capsule is moved by casting and by solving collision
// planes, so the game keeps full control of the motion.
//
// A pogo spring holds the capsule above the ground so it can move up steps.
class GeometricMover
{
public:
	GeometricMover();

	void Create( b3WorldId worldId, const GeometricMoverDef* def );

	// Advance the mover. Throttle is the input along the forward and right directions on [-1, 1].
	void Update( float timeStep, b3Vec3 forward, b3Vec3 right, b3Vec2 throttle, float speedScale );

	// Jump if grounded. Returns false when airborne.
	bool Jump();

	void Draw() const;

	static constexpr int m_planeCapacity = 8;

	float m_jumpSpeed;
	float m_maxSpeed;
	float m_minSpeed;
	float m_stopSpeed;
	float m_accelerate;

	// Friction in units of 1/time.
	float m_friction;

	// The mover is not simulated so it needs its own gravity.
	float m_gravity;

	float m_pogoRestLength;
	float m_pogoHertz;
	float m_pogoDampingRatio;

	// The clipper avoids picking up velocity from soft collision depenetration. Otherwise the
	// velocity comes from the position delta, which is more intuitive in some cases.
	bool m_clipVelocity;

	// Shapes the mover passes through. Set before calling Update.
	const b3ShapeId* m_ignoreShapeIds;
	int m_ignoreCount;

	// The capsule is relative to the mover position.
	b3Capsule m_capsule;

	b3WorldId m_worldId;

	b3Pos m_position;
	b3Vec3 m_velocity;
	float m_pogoVelocity;
	bool m_onGround;

	// Planes from the last solve, kept for debug drawing.
	b3CollisionPlane m_planes[m_planeCapacity] = {};
	PlaneExtra m_planeExtras[m_planeCapacity] = {};
	int m_planeCount;
	int m_totalIterations;

	b3Pos m_pogoOrigin;
	b3Vec3 m_pogoTranslation;
	b3Pos m_pogoPoint;
	bool m_pogoHit;

private:
	void PushBodies();
};
