// SPDX-FileCopyrightText: 2026 Erin Catto
// SPDX-License-Identifier: MIT

#include "geometric_mover.h"

#include "gfx/draw.h"

#include "box3d/box3d.h"
#include "box3d/math_functions.h"

#include <assert.h>
#include <float.h>

static bool MoverFilterCallback( b3ShapeId shapeId, void* context )
{
	GeometricMover* self = (GeometricMover*)context;
	for ( int i = 0; i < self->m_ignoreCount; ++i )
	{
		if ( B3_ID_EQUALS( shapeId, self->m_ignoreShapeIds[i] ) )
		{
			return false;
		}
	}

	return true;
}

static bool PlaneResultFcn( b3ShapeId shapeId, const b3PlaneResult* planeResults, int planeCount,
							void* context )
{
	if ( MoverFilterCallback( shapeId, context ) == false )
	{
		// ignore these planes but continue looking for more
		return true;
	}

	GeometricMover* self = static_cast<GeometricMover*>( context );
	float maxPush = FLT_MAX;
	bool clipVelocity = true;
	MoverShapeUserData* userData =
		static_cast<MoverShapeUserData*>( b3Shape_GetUserData( shapeId ) );
	if ( userData != nullptr )
	{
		maxPush = userData->maxPush;
		clipVelocity = userData->clipVelocity;
	}

	for ( int i = 0; i < planeCount && self->m_planeCount < GeometricMover::m_planeCapacity; ++i )
	{
		assert( b3IsValidPlane( planeResults[i].plane ) );
		self->m_planes[self->m_planeCount] = {
			.plane = planeResults[i].plane,
			.pushLimit = maxPush,
			.push = 0.0f,
			.clipVelocity = clipVelocity,
		};

		// The plane point is relative to the origin passed to the collide call
		self->m_planeExtras[self->m_planeCount] = {
			.point = b3OffsetPos( self->m_position, planeResults[i].point ),
			.shapeId = shapeId,
		};
		self->m_planeCount += 1;
	}

	return true;
}

GeometricMover::GeometricMover()
{
	GeometricMoverDef def;
	m_jumpSpeed = def.jumpSpeed;
	m_maxSpeed = def.maxSpeed;
	m_minSpeed = def.minSpeed;
	m_stopSpeed = def.stopSpeed;
	m_accelerate = def.accelerate;
	m_friction = def.friction;
	m_gravity = def.gravity;
	m_pogoRestLength = def.pogoRestLength;
	m_pogoHertz = def.pogoHertz;
	m_pogoDampingRatio = def.pogoDampingRatio;
	m_clipVelocity = true;
	m_ignoreShapeIds = nullptr;
	m_ignoreCount = 0;

	m_capsule = def.capsule;
	m_worldId = b3_nullWorldId;
	m_position = def.position;
	m_velocity = b3Vec3_zero;
	m_pogoVelocity = 0.0f;
	m_onGround = false;

	m_planeCount = 0;
	m_totalIterations = 0;

	m_pogoOrigin = b3Pos_zero;
	m_pogoTranslation = b3Vec3_zero;
	m_pogoPoint = b3Pos_zero;
	m_pogoHit = false;
}

void GeometricMover::Create( b3WorldId worldId, const GeometricMoverDef* def )
{
	m_capsule = def->capsule;
	m_jumpSpeed = def->jumpSpeed;
	m_maxSpeed = def->maxSpeed;
	m_minSpeed = def->minSpeed;
	m_stopSpeed = def->stopSpeed;
	m_accelerate = def->accelerate;
	m_friction = def->friction;
	m_gravity = def->gravity;
	m_pogoRestLength = def->pogoRestLength;
	m_pogoHertz = def->pogoHertz;
	m_pogoDampingRatio = def->pogoDampingRatio;

	m_worldId = worldId;
	m_position = def->position;
	m_velocity = b3Vec3_zero;
	m_pogoVelocity = 0.0f;
	m_onGround = false;
	m_planeCount = 0;
	m_totalIterations = 0;
	m_pogoHit = false;
}

bool GeometricMover::Jump()
{
	if ( m_onGround == false )
	{
		return false;
	}

	m_velocity.y = m_jumpSpeed;
	m_onGround = false;
	return true;
}

// Solve a normal constraint between the mover and each touched rigid body. This is the
// same velocity constraint the solver uses for a contact, except the mover is treated as
// having infinite mass, so all of the impulse lands on the body.
void GeometricMover::PushBodies()
{
	for ( int i = 0; i < m_planeCount; ++i )
	{
		b3BodyId bodyId = b3Shape_GetBody( m_planeExtras[i].shapeId );
		if ( b3Body_GetType( bodyId ) != b3_dynamicBody )
		{
			continue;
		}

		MoverShapeUserData* userData =
			static_cast<MoverShapeUserData*>( b3Shape_GetUserData( m_planeExtras[i].shapeId ) );
		if ( userData != nullptr && userData->canMoverPush == false )
		{
			continue;
		}

		// Plane normals point at the mover
		b3Pos point = m_planeExtras[i].point;
		b3Vec3 normal = b3Neg( m_planes[i].plane.normal );

		float invMassA = 0.0f;
		float invMassB = b3Body_GetInverseMass( bodyId );
		b3Matrix3 invIB = b3Body_GetWorldInverseRotationalInertia( bodyId );

		b3Pos centerB = b3Body_GetWorldCenter( bodyId );
		b3Vec3 rB = b3SubPos( point, centerB );

		b3Vec3 rnB = b3Cross( rB, normal );
		float kNormal = invMassA + invMassB + b3Dot( rnB, b3MulMV( invIB, rnB ) );
		float normalMass = kNormal > 0.0f ? 1.0f / kNormal : 0.0f;

		b3Vec3 vB = b3Body_GetLinearVelocity( bodyId );
		b3Vec3 omegaB = b3Body_GetAngularVelocity( bodyId );
		b3Vec3 vrB = b3Add( vB, b3Cross( omegaB, rB ) );
		float vn = b3Dot( b3Sub( vrB, m_velocity ), normal );

		// Push only, a separating body is left alone
		float impulse = b3MaxFloat( -normalMass * vn, 0.0f );

		b3Vec3 P = b3MulSV( impulse, normal );
		m_velocity = b3MulSub( m_velocity, invMassA, P );

		b3Body_ApplyLinearImpulse( bodyId, P, point, true );
	}
}

// Movement follows the Quake ground and air model:
// https://github.com/id-Software/Quake/blob/master/QW/client/pmove.c#L390
void GeometricMover::Update( float timeStep, b3Vec3 forward, b3Vec3 right, b3Vec2 throttle,
							 float speedScale )
{
	// Friction
	float speed = b3Length( m_velocity );
	if ( speed < m_minSpeed )
	{
		m_velocity.x = 0.0f;
		m_velocity.z = 0.0f;
	}
	else
	{
		// Linear damping above stopSpeed and fixed reduction below stopSpeed
		float control = speed < m_stopSpeed ? m_stopSpeed : speed;

		// friction has units of 1/time
		float drop = control * m_friction * timeStep;
		float newSpeed = b3MaxFloat( 0.0f, speed - drop );
		float ratio = newSpeed / speed;
		m_velocity.x *= ratio;
		m_velocity.z *= ratio;
	}

	float maxSpeed = speedScale * m_maxSpeed;

	b3Vec3 desiredVelocity = maxSpeed * throttle.x * forward + maxSpeed * throttle.y * right;
	float desiredSpeed;
	b3Vec3 desiredDirection = b3GetLengthAndNormalize( &desiredSpeed, desiredVelocity );

	if ( desiredSpeed > maxSpeed )
	{
		desiredSpeed = maxSpeed;
	}

	if ( m_onGround )
	{
		m_velocity.y = 0.0f;
	}

	// Accelerate
	float currentSpeed = b3Dot( m_velocity, desiredDirection );
	float addSpeed = desiredSpeed - currentSpeed;
	if ( addSpeed > 0.0f )
	{
		float accelSpeed = m_accelerate * maxSpeed * timeStep;
		if ( accelSpeed > addSpeed )
		{
			accelSpeed = addSpeed;
		}

		m_velocity += accelSpeed * desiredDirection;
	}

	m_velocity.y -= m_gravity * timeStep;

	// The probe reaches past the rest length so the spring can be stretched
	float rayLength = m_pogoRestLength + m_capsule.radius;
	b3Pos rayOrigin = m_position + m_capsule.center1;
	b3Vec3 rayTranslation = -rayLength * b3Vec3_axisY;
	b3QueryFilter pogoFilter = { 1, ~2ull, 0, "pogo" };
	b3RayResult rayResult =
		b3World_CastRayClosest( m_worldId, rayOrigin, rayTranslation, pogoFilter );

	m_pogoOrigin = rayOrigin;
	m_pogoTranslation = rayTranslation;
	m_pogoPoint = rayResult.point;

	// After gravity was applied, disable pogo when still moving up.
	// Avoids getting pulled back to the ground when jumping.
	bool suppressPogo = m_velocity.y > 0.0f;

	if ( rayResult.hit == false || suppressPogo )
	{
		m_onGround = false;
		m_pogoVelocity = 0.0f;
	}
	else
	{
		m_onGround = true;
		float pogoCurrentLength = rayResult.fraction * rayLength;
		float offset = pogoCurrentLength - m_pogoRestLength;
		m_pogoVelocity =
			b3SpringDamper( m_pogoHertz, m_pogoDampingRatio, offset, m_pogoVelocity, timeStep );
	}

	m_pogoHit = m_onGround;

	b3Pos startPosition = m_position;
	b3Pos target = m_position + timeStep * m_velocity + timeStep * m_pogoVelocity * b3Vec3_axisY;

	// Want the mover to collide with allies
	b3QueryFilter moverFilter = {
		.categoryBits = 1, .maskBits = ~0ull, .id = 1, .name = "mover_collide" };

	// The cast should ignore allies
	b3QueryFilter castFilter = {
		.categoryBits = 1, .maskBits = ~2ull, .id = 1, .name = "mover_cast" };

	m_totalIterations = 0;
	float tolerance = 0.01f;

	for ( int iteration = 0; iteration < 5; ++iteration )
	{
		m_planeCount = 0;

		b3World_CollideMover( m_worldId, m_position, &m_capsule, moverFilter, PlaneResultFcn,
							  this );

		b3Vec3 targetDelta = target - m_position;
		b3PlaneSolverResult result = b3SolvePlanes( targetDelta, m_planes, m_planeCount );

		m_totalIterations += result.iterationCount;

		float fraction = b3World_CastMover( m_worldId, m_position, &m_capsule, result.delta,
											castFilter, MoverFilterCallback, this );

		b3Vec3 delta = fraction * result.delta;
		m_position = m_position + delta;

		if ( b3LengthSquared( delta ) < tolerance * tolerance )
		{
			break;
		}
	}

	PushBodies();

	if ( m_clipVelocity )
	{
		// Stop pushing into surfaces so speed can't accumulate against a wall
		m_velocity = b3ClipVector( m_velocity, m_planes, m_planeCount );
	}
	else if ( timeStep > 0.0f )
	{
		m_velocity = ( 1.0f / timeStep ) * ( m_position - startPosition );
	}
}

void GeometricMover::Draw() const
{
	if ( m_pogoHit )
	{
		DrawLine( m_pogoOrigin, m_pogoPoint, MakeColor( b3_colorGreen ) );
	}
	else
	{
		DrawLine( m_pogoOrigin, b3OffsetPos( m_pogoOrigin, m_pogoTranslation ),
				  MakeColor( b3_colorGray ) );
	}

	for ( int i = 0; i < m_planeCount; ++i )
	{
		b3Plane plane = m_planes[i].plane;
		b3Pos p1 = m_position + ( plane.offset - m_capsule.radius ) * plane.normal;
		b3Pos p2 = p1 + 0.1f * plane.normal;
		DrawPoint( p1, 5.0f, MakeColor( b3_colorYellow ) );
		DrawLine( p1, p2, MakeColor( b3_colorYellow ) );
	}

	DrawSolidCapsule( { m_position, b3Quat_identity }, m_capsule, MakeColor( b3_colorBlue ) );
	DrawLine( m_position, m_position + m_velocity, MakeColor( b3_colorPurple ) );
}
