// SPDX-FileCopyrightText: 2026 Erin Catto
// SPDX-License-Identifier: MIT

#pragma once

#include "box3d/types.h"

struct DynamicMoverDef
{
	b3Pos position = {};
	b3Capsule capsule = { { 0.0f, -0.5f, 0.0f }, { 0.0f, 0.5f, 0.0f }, 0.3f };

	float density = 200.0f;

	b3Filter filter = b3DefaultFilter();
	float jumpSpeed = 7.0f;
	float maxSpeed = 6.0f;
	float minSpeed = 0.1f;
	float stopSpeed = 3.0f;
	float accelerate = 20.0f;
	float airSteer = 0.25f;
	float friction = 8.0f;
	float gravityScale = 1.5f;
	float maxGroundForce = 6000.0f;
	float maxAirForce = 1500.0f;
	float pogoRestLength = 0.9f;
	float pogoHertz = 3.0f;
	float pogoDampingRatio = 0.8f;
	float pogoCompressionScale = 100.0f;
	float pogoTensionScale = 100.0f;
	uint64_t platformMaterialId = 0;
	bool enableSkirtPogo = true;
	bool enablePreSolveEvents = false;
	bool enableSensorEvents = false;
};

struct DynamicMoverCastResult
{
	b3Pos point;
	b3Vec3 normal;
	b3BodyId bodyId;
	uint64_t userMaterialId;
	float fraction;
	bool hit;
};

struct DynamicMoverProbe
{
	b3Pos origin;
	b3Vec3 translation;
	float fraction;
	bool hit;
};

// Dynamic character mover. The capsule is a rigid body with locked rotation, so it
// collides and is pushed like everything else in the world. A mover joint drives it to
// the target velocity and a pogo joint holds it above the ground so it can climb
// steps without touching the capsule.
//
// The dynamic mover should be vendored into your project so you can alter it to meet
// your game's specific needs.
//
// This system is inspired by Tom Waterson's presentation:
// https://www.youtube.com/watch?v=_jRLlTDqoGI
class DynamicMover
{
public:
	DynamicMover();

	void Create( b3WorldId worldId, const DynamicMoverDef* def );
	void Destroy();

	// Advance the mover. Throttle is the input along the forward and right directions on [-1, 1].
	void Update( float timeStep, b3Vec3 forward, b3Vec3 right, b3Vec2 throttle, float speedScale );

	// Jump if grounded. Returns false when airborne.
	bool Jump();

	// Release the ground so an external impulse can carry the mover into the air. Otherwise the
	// pogo pulls it back down.
	void Launch();

	// Gravity scale is used to size the pogo forces, so change it here.
	void SetGravityScale( float gravityScale );

	b3BodyId GetBodyId() const
	{
		return m_moverId;
	}

	void Draw() const;

	static constexpr int m_skirtCount = 8;
	static constexpr int m_probeCapacity = m_skirtCount + 1;

	float m_jumpSpeed;
	float m_maxSpeed;
	float m_minSpeed;
	float m_stopSpeed;
	float m_accelerate;

	// Fraction of the ground acceleration available in the air.
	float m_airSteer;

	// Ground friction in units of 1/time.
	float m_friction;

	// A character usually wants to fall faster than the rest of the world.
	float m_gravityScale;

	// Force budget the mover joint has for reaching the target velocity. The air value
	// is small so the character keeps its momentum while jumping.
	float m_maxGroundForce;
	float m_maxAirForce;

	float m_pogoRestLength;
	float m_pogoHertz;
	float m_pogoDampingRatio;

	// Pogo force limits as a multiple of the mover weight.
	float m_pogoCompressionScale;
	float m_pogoTensionScale;

	// Casts an octagon skirt of rays around the capsule when the central ray misses or
	// lands on a surface too steep to walk, so the mover can stand on a ledge or a thin
	// rail, or step off the foot of a steep slope.
	bool m_enableSkirtPogo;

	// Used for the pogo to detect moving platforms.
	uint64_t m_platformMaterialId;

	// Surfaces steeper than this are not ground.
	float m_minGroundNormalY;

	b3Capsule m_capsule;
	b3Filter m_filter;

	b3WorldId m_worldId;
	b3BodyId m_groundId;
	b3BodyId m_moverId;
	b3BodyId m_surfaceBodyId;
	b3JointId m_moverJointId;
	b3JointId m_pogoJointId;

	DynamicMoverCastResult m_castResult;

	b3Vec3 m_velocity;
	float m_timeSliding;
	float m_slideDuration;
	bool m_onGround;
	bool m_walkable;
	bool m_jumping;
	int m_jumpTicks;

	// Pogo joint state, carried across the joint being recreated each update.
	float m_pogoImpulse;
	float m_pogoVelocity;
	float m_pogoLength;

	// Pogo ray cast results for drawing.
	DynamicMoverProbe m_probes[m_probeCapacity];
	int m_probeCount;
	int m_selectedProbe;

private:
	void CreateMoverJoint( b3BodyId baseId );
};
