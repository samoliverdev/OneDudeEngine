#include "PhysicsSystem.h"
#include "OD/Core/Application.h"
#include "OD/Scene/Scene.h"
#include "OD/Serialization/ImGuiArchive.h"
#include <set>
#include <algorithm>
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletDynamics/Dynamics/btDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletCollision/CollisionShapes/btMaterial.h>
#include <BulletCollision/CollisionShapes/btTriangleShape.h>

namespace OD{

#pragma region Core
inline Vector3 FromBullet(btVector3 v){ return Vector3(v.x(), v.y(), v.z()); }
inline btVector3 ToBullet(Vector3 v){ return btVector3(v.x, v.y, v.z); }

inline Quaternion FromBullet(btQuaternion v){ return Quaternion(v.x(), v.y(), v.z(), v.w()); }
inline btQuaternion ToBullet(Quaternion v){ return btQuaternion(v.x, v.y, v.z, v.w); }

inline btTransform ToBullet(Transform v){
    btTransform t;
    t.setIdentity();
    t.setOrigin(btVector3(v.LocalPosition().x, v.LocalPosition().y, v.LocalPosition().z));
    t.setRotation(btQuaternion(v.LocalRotation().x, v.LocalRotation().y, v.LocalRotation().z, v.LocalRotation().w));
    return t;
}

inline btTransform ToBullet(TransformComponent& v){
    btTransform t;
    t.setIdentity();
    t.setOrigin(btVector3(v.Position().x, v.Position().y, v.Position().z));
    t.setRotation(btQuaternion(v.Rotation().x, v.Rotation().y, v.Rotation().z, v.Rotation().w));
    return t;
}


struct Rigidbody{
    bool updating = false;
    btCollisionShape* shape = nullptr;
    btRigidBody* body = nullptr;
    btDynamicsWorld* world = nullptr;
    btDefaultMotionState* motionState = nullptr;

    EntityId entityId;
};

typedef std::pair<const btRigidBody*, const btRigidBody*> CollisionPair;
typedef std::set<CollisionPair> CollisionPairs;

struct PhysicsWorld{
    btBroadphaseInterface* broadphase;
    btCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btConstraintSolver* solver;
    btDynamicsWorld* world;
    CollisionPairs pairsLastUpdate;
};

class Debuger: public btIDebugDraw{
public:
    Scene* scene = nullptr;

    void drawLine(const btVector3 &from,const btVector3&to, const btVector3 &color) override{
        if(scene != nullptr){
            TransformComponent& cam = scene->GetMainCamera2().GetComponent<TransformComponent>();
            if(math::distance(cam.Position(), FromBullet(from)) > 50) return;
        }

        Graphics::DrawLine(
            Vector3(from.x(), from.y(), from.z()), 
            Vector3(to.x(), to.y(), to.z()), 
            Vector3(color.x(), color.y(), color.z()),
            2
        );
    }

    void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override {
        Transform transform;
        transform.LocalPosition(Vector3(PointOnB.x(), PointOnB.y(), PointOnB.z()));
        transform.LocalScale(Vector3(0.25f, 0.25f, 0.25f));

        Graphics::DrawWireCube(
            transform.GetLocalModelMatrix(), 
            Vector3(color.x(), color.y(), color.z()), 
            2
        );

        Graphics::DrawLine(
            transform.LocalPosition(), 
            transform.LocalPosition() + Vector3(normalOnB.x(), normalOnB.y(), normalOnB.z()), 
            Vector3(color.x(), color.y(), color.z()),
            2
        );
    }

    void reportErrorWarning(const char* warningString) override {}
	void draw3dText(const btVector3& location, const char* textString) override {}
	void setDebugMode(int debugMode) override {}
	int getDebugMode() const override { return 1; }
};

Debuger debuger;
#pragma endregion

#pragma region RigidbodyComponent

void RigidbodyComponent::OnGui(Entity& e){
    RigidbodyComponent& rb = e.GetComponent<RigidbodyComponent>();

    const char* optionsString[] = {"Dynamic", "Static", "Kinematic", "Trigger"};
    const char* curOptionString = optionsString[(int)rb.GetType()];
    if(ImGui::BeginCombo("Type", curOptionString)){
        for(int i = 0; i < 4; i++){
            bool isSelected = curOptionString == optionsString[i];
            if(ImGui::Selectable(optionsString[i], isSelected)){
                curOptionString = optionsString[i];
                rb.SetType((RigidbodyComponent::Type)i);
            }

            if(isSelected) ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    float mass = rb.Mass();
    if(ImGui::DragFloat("mass", &mass)){
        rb.Mass(mass);
    }

    bool neverSleep = rb.NeverSleep();
    if(ImGui::Checkbox("neverSleep", &neverSleep)){
        rb.NeverSleep(neverSleep);
    }

    CollisionShape shape = rb.GetShape();

    ImGui::Spacing();
    ImGui::SeparatorText("CollisionShape");

    const char* shapeTypeString[] = {"Box", "Sphere"};
    const char* curShapeTypeString = shapeTypeString[(int)rb.GetShape().type];
    if(ImGui::BeginCombo("CollisionShape", curShapeTypeString)){
        for(int i = 0; i < 2; i++){
            bool isSelected = curShapeTypeString == shapeTypeString[i];
            if(ImGui::Selectable(shapeTypeString[i], isSelected)){
                curShapeTypeString = shapeTypeString[i];
                shape.type = (CollisionShape::Type)i;
                rb.SetShape(shape);
            }

            if(isSelected) ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    shape = rb.GetShape();

    if(rb.shape.type == CollisionShape::Type::Box){
        float _shape[] = {shape.boxShapeSize.x, shape.boxShapeSize.y, shape.boxShapeSize.z};
        if(ImGui::DragFloat3("size", _shape)){
            shape.boxShapeSize = Vector3(_shape[0], _shape[1], _shape[2]);
            rb.SetShape(shape);
        }
    }

    if(rb.shape.type == CollisionShape::Type::Sphere){
        float _radius = shape.sphereRadius;
        if(ImGui::DragFloat("radius", &_radius)){
            shape.sphereRadius = _radius;
            rb.SetShape(shape);
        }
    }
}

void RigidbodyComponent::SetShape(CollisionShape inShape){
    shape = inShape;

    if(data == nullptr) return;

    if(data->shape != nullptr) delete data->shape;

    if(shape.type == CollisionShape::Type::Box){
        data->shape = new btBoxShape(btVector3(shape.boxShapeSize.x/2, shape.boxShapeSize.y/2, shape.boxShapeSize.z/2));
        if(data->body != nullptr) data->body->setCollisionShape(data->shape);
    }

    if(shape.type == CollisionShape::Type::Sphere){
        data->shape = new btSphereShape(shape.sphereRadius);
        if(data->body != nullptr) data->body->setCollisionShape(data->shape);
    }

    if(data->updating == false) UpdateSettings();
    //data->body->activate(true);
}

void RigidbodyComponent::UpdateSettings(){
    if(data->updating == false) data->world->removeRigidBody(data->body);

    if(mass > 0 && type == RigidbodyComponent::Type::Static) type = RigidbodyComponent::Type::Dynamic;
    if(mass <= 0 && type == RigidbodyComponent::Type::Dynamic) type = RigidbodyComponent::Type::Static;

    if(type == RigidbodyComponent::Type::Dynamic) data->body->setCollisionFlags(btCollisionObject::CF_DYNAMIC_OBJECT);
    if(type == RigidbodyComponent::Type::Static) data->body->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
    if(type == RigidbodyComponent::Type::Kinematic) data->body->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
    if(type == RigidbodyComponent::Type::Trigger) data->body->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE | btCollisionObject::CF_KINEMATIC_OBJECT);
    
    btVector3 localInertia(0,0,0);
    if (mass != 0.0f) data->shape->calculateLocalInertia(mass, localInertia);
    data->body->setMassProps(mass, localInertia);
    
    if(data->updating == false) data->world->addRigidBody(data->body);
}

void RigidbodyComponent::Mass(float m){
    mass = m;
    if(data == nullptr) return;
    UpdateSettings();
}

void RigidbodyComponent::SetType(RigidbodyComponent::Type value){
    type = value;
    if(data == nullptr) return;
    UpdateSettings();
}

void RigidbodyComponent::NeverSleep(bool value){
    neverSleep = value;

    if(data == nullptr) return;

    if(value){
        data->body->setActivationState(DISABLE_DEACTIVATION);
    } else {
        data->body->setActivationState(ACTIVE_TAG);
    }
}

Vector3 RigidbodyComponent::Position(){
    if(data == nullptr) return Vector3Zero;
    btTransform trans = data->body->getWorldTransform();
    return FromBullet(trans.getOrigin());
}

void RigidbodyComponent::Position(Vector3 position){
    if(data == nullptr) return;
    btTransform trans;
    trans.setIdentity();
    trans.setOrigin(ToBullet(position));
    data->body->setWorldTransform(trans);
    data->motionState->setWorldTransform(trans);

    data->body->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
    data->body->setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));
    data->body->clearForces();
}

Vector3 RigidbodyComponent::Velocity(){
    if(data == nullptr) return Vector3Zero;
    return FromBullet(data->body->getLinearVelocity());
}

void RigidbodyComponent::Velocity(Vector3 v){
    if(data == nullptr) return;
    return data->body->setLinearVelocity(ToBullet(v));
}

void RigidbodyComponent::ApplyForce(Vector3 v){
    if(data == nullptr) return;
    data->body->applyCentralForce(ToBullet(v));
}

void RigidbodyComponent::ApplyTorque(Vector3 v){
    if(data == nullptr) return;
    data->body->applyTorque(ToBullet(v));
}

void RigidbodyComponent::ApplyImpulse(Vector3 v){
    if(data == nullptr) return;
    data->body->applyCentralImpulse(ToBullet(v));
}

#pragma endregion

#pragma region PhysicsSystem

int ACCURACY = 10;

bool PhysicsSystem::IsSimulationEnable(){ return GetScene()->Running(); }

PhysicsSystem::PhysicsSystem(Scene* inScene):System(inScene){
    physicsWorld = new PhysicsWorld();
    physicsWorld->collisionConfiguration = new btDefaultCollisionConfiguration();
    physicsWorld->dispatcher = new btCollisionDispatcher(physicsWorld->collisionConfiguration);
    physicsWorld->broadphase = new btDbvtBroadphase();
    physicsWorld->solver = new btSequentialImpulseConstraintSolver();
    physicsWorld->world = new btDiscreteDynamicsWorld(
        physicsWorld->dispatcher, physicsWorld->broadphase, physicsWorld->solver, physicsWorld->collisionConfiguration
    );
    physicsWorld->world->setGravity(btVector3(0.0f, -9.8f, 0.0f));
    physicsWorld->world->setDebugDrawer(&debuger);
    physicsWorld->world->getSolverInfo().m_solverMode |= SOLVER_RANDMIZE_ORDER;
    physicsWorld->world->getDispatchInfo().m_enableSatConvex = true;
	physicsWorld->world->getSolverInfo().m_splitImpulse = true;

    btContactSolverInfo& info = physicsWorld->world->getSolverInfo();
    info.m_numIterations = 50;
    debuger.scene = inScene;

    this->scene->GetRegistry().on_destroy<RigidbodyComponent>().connect<&OnRemoveRigidbody>();
    this->scene->GetRegistry().ctx().emplace<PhysicsSystem*>(this);

    LogWarning("PhysicsSystem Contructor");
}

PhysicsSystem::~PhysicsSystem(){
    this->scene->GetRegistry().ctx().erase<PhysicsSystem*>();
    this->scene->GetRegistry().on_destroy<RigidbodyComponent>().disconnect<&OnRemoveRigidbody>();

    delete physicsWorld->world;
    delete physicsWorld->solver;
    delete physicsWorld->broadphase;
    delete physicsWorld->dispatcher;
    delete physicsWorld->collisionConfiguration;
    delete physicsWorld;

    LogWarning("PhysicsSystem Destructor"); 
}

void PhysicsSystem::OnRemoveRigidbody(entt::registry& r, entt::entity e){
    LogInfo("Removing Rigidbody");
    PhysicsSystem* physicsSystem = r.ctx().get<PhysicsSystem*>();
    RigidbodyComponent& rb = r.get<RigidbodyComponent>(e);
    physicsSystem->RemoveRigidbody(e, rb);
}

void PhysicsSystem::Update(){
    if(GetScene()->Running() == false) return;

    physicsWorld->world->stepSimulation(Application::DeltaTime(), ACCURACY);
    //world->synchronizeMotionStates();
    //world->performDiscreteCollisionDetection();

    auto view = GetScene()->GetRegistry().view<RigidbodyComponent, TransformComponent>();
    for(auto e: view){
        RigidbodyComponent& rb = view.get<RigidbodyComponent>(e);
        TransformComponent& transform = view.get<TransformComponent>(e);

        if(rb.data == nullptr) AddRigidbody(e, rb, transform);
        Assert(rb.data != nullptr);

        Rigidbody* data = rb.data;
        data->shape->setLocalScaling(ToBullet(transform.LocalScale()));

        if(rb.GetType() == RigidbodyComponent::Type::Dynamic || rb.GetType() == RigidbodyComponent::Type::Static){
            btTransform trans = data->body->getWorldTransform();
            transform.Position(FromBullet(trans.getOrigin()));
            transform.Rotation(FromBullet(trans.getRotation()));
        } else {
            btMotionState* motionState = data->body->getMotionState();
            btTransform physicsTransform = ToBullet(transform);
            motionState->setWorldTransform(physicsTransform);
        }
    }

    CheckForCollisionEvents();
}

void PhysicsSystem::CheckForCollisionEvents(){
    //return;
    // keep a list of the collision pairs we
	// found during the current update
	CollisionPairs pairsThisUpdate;

	// iterate through all of the manifolds in the dispatcher
	for(int i = 0; i < physicsWorld->dispatcher->getNumManifolds(); ++i){
		
		// get the manifold
		btPersistentManifold* pManifold = physicsWorld->dispatcher->getManifoldByIndexInternal(i);
		
		// ignore manifolds that have 
		// no contact points.
		if(pManifold->getNumContacts() > 0){
			// get the two rigid bodies involved in the collision
			const btRigidBody* pBody0 = static_cast<const btRigidBody*>(pManifold->getBody0());
			const btRigidBody* pBody1 = static_cast<const btRigidBody*>(pManifold->getBody1());
    
			// always create the pair in a predictable order
			// (use the pointer value..)
			bool const swapped = pBody0 > pBody1;
			const btRigidBody* pSortedBodyA = swapped ? pBody1 : pBody0;
			const btRigidBody* pSortedBodyB = swapped ? pBody0 : pBody1;
			
			// create the pair
			CollisionPair thisPair = std::make_pair(pSortedBodyA, pSortedBodyB);
			
			// insert the pair into the current list
			pairsThisUpdate.insert(thisPair);

			// if this pair doesn't exist in the list
			// from the previous update, it is a new
			// pair and we must send a collision event
			if(physicsWorld->pairsLastUpdate.find(thisPair) == physicsWorld->pairsLastUpdate.end()){
                if(pBody0->getUserPointer() == nullptr) continue;
                if(pBody1->getUserPointer() == nullptr) continue;

                Assert(pBody0 != nullptr);
                Assert(pBody0->getUserPointer() != nullptr);
                Assert(pBody1 != nullptr);
                Assert(pBody1->getUserPointer() != nullptr);
				//CollisionEvent((btRigidBody*)pBody0, (btRigidBody*)pBody1);

                Rigidbody* r1 = static_cast<Rigidbody*>(pBody0->getUserPointer());
                Rigidbody* r2 = static_cast<Rigidbody*>(pBody1->getUserPointer());
                Assert(r1 != nullptr);
                Assert(r2 != nullptr);
                
                Entity e1 = Entity(r1->entityId, scene);
                Entity e2 = Entity(r2->entityId, scene);
                Assert(e1.HasComponent<InfoComponent>());
                Assert(e2.HasComponent<InfoComponent>());

                LogInfo("OnCollision %s <==> %s", e1.GetComponent<InfoComponent>().name.c_str(), e2.GetComponent<InfoComponent>().name.c_str());
                //LogInfo("OnTriggerCallbacks: %zd", onTriggerCallbacks.size());

                for(auto i: onCollisionEnterCallbacks){
                    i(e1, e2);
                }

                RigidbodyComponent& _r1 = e1.GetComponent<RigidbodyComponent>();
                RigidbodyComponent& _r2 = e2.GetComponent<RigidbodyComponent>();
                
                if(_r1.GetType() == RigidbodyComponent::Type::Trigger){
                    for(auto i: onTriggerEnterCallbacks){
                        i(e1, e2);
                    }
                }
                if(_r2.GetType() == RigidbodyComponent::Type::Trigger){
                    for(auto i: onTriggerEnterCallbacks){
                        i(e2, e1);
                    }
                }
			}
		}
	}
	
	// create another list for pairs that
	// were removed this update
	CollisionPairs removedPairs;
	
	// this handy function gets the difference beween
	// two sets. It takes the difference between
	// collision pairs from the last update, and this 
	// update and pushes them into the removed pairs list
	std::set_difference(physicsWorld->pairsLastUpdate.begin(), physicsWorld->pairsLastUpdate.end(),
	pairsThisUpdate.begin(), pairsThisUpdate.end(),
	std::inserter(removedPairs, removedPairs.begin()));
	
	// iterate through all of the removed pairs
	// sending separation events for them
	for(CollisionPairs::const_iterator iter = removedPairs.begin(); iter != removedPairs.end(); ++iter){
        if(iter->first->getUserPointer() == nullptr) continue;
        if(iter->second->getUserPointer() == nullptr) continue;

        Assert(iter->first != nullptr);
        Assert(iter->first->getUserPointer() != nullptr);
        Assert(iter->second != nullptr);
        Assert(iter->second->getUserPointer() != nullptr);
        //SeparationEvent((btRigidBody*)iter->first, (btRigidBody*)iter->second);

        Rigidbody* r1 = static_cast<Rigidbody*>(iter->first->getUserPointer());
        Rigidbody* r2 = static_cast<Rigidbody*>(iter->second->getUserPointer());
        Assert(r1 != nullptr);
        Assert(r2 != nullptr);

        Entity e1 = Entity(r1->entityId, scene);
        Entity e2 = Entity(r2->entityId, scene);
        Assert(e1.HasComponent<InfoComponent>());
        Assert(e2.HasComponent<InfoComponent>());
        LogInfo("OnSeparation %s <==> %s", e1.GetComponent<InfoComponent>().name.c_str(), e2.GetComponent<InfoComponent>().name.c_str());

        for(auto i: onCollisionExitCallbacks){
            i(e1, e2);
        }

        RigidbodyComponent& _r1 = e1.GetComponent<RigidbodyComponent>();
        RigidbodyComponent& _r2 = e2.GetComponent<RigidbodyComponent>();
        
        if(_r1.GetType() == RigidbodyComponent::Type::Trigger){
            for(auto i: onTriggerExitCallbacks){
                i(e1, e2);
            }
        }
        if(_r2.GetType() == RigidbodyComponent::Type::Trigger){
            for(auto i: onTriggerExitCallbacks){
                i(e2, e1);
            }
        }
	}
	
	// in the next iteration we'll want to
	// compare against the pairs we found
	// in this iteration
	physicsWorld->pairsLastUpdate = pairsThisUpdate;
}

void PhysicsSystem::ShowDebugGizmos(){
    //if(scene()->running()) return;
    physicsWorld->world->debugDrawWorld();
}

bool PhysicsSystem::Raycast(Vector3 pos, Vector3 dir, RayResult& hit){
    //if(world == nullptr) return false;
    Assert(physicsWorld->world != nullptr);

    btVector3 _pos = btVector3(pos.x, pos.y, pos.z);
    btVector3 _dir = btVector3(dir.x, dir.y, dir.z);

    btCollisionWorld::ClosestRayResultCallback rayCallback(_pos, _dir);
    physicsWorld->world->rayTest(_pos, _dir, rayCallback);

    if(rayCallback.hasHit()){
        // if so, get the rigid body we hit
        btRigidBody* pBody = (btRigidBody*)btRigidBody::upcast(rayCallback.m_collisionObject);
        if(!pBody) return false;

        // prevent us from picking objects
        // like the ground plane
        //if(pBody->isStaticObject() || pBody->isKinematicObject()) return false;

        EntityId entityId = static_cast<Rigidbody*>(pBody->getUserPointer())->entityId;

        // set the result data
        //hit.pBody = pBody;
        hit.entity = Entity(entityId, scene);
        hit.hitPoint = FromBullet(rayCallback.m_hitPointWorld);
        hit.hitNormal = FromBullet(rayCallback.m_hitNormalWorld);
        return true;
    }

    return false;
}

void PhysicsSystem::AddOnCollisionEnterCallback(OnCollisionCallback callback){ onCollisionEnterCallbacks.push_back(callback); }
void PhysicsSystem::RemoveOnCollisionEnterCallback(OnCollisionCallback callback){ 
    onCollisionEnterCallbacks.erase(std::remove(onCollisionEnterCallbacks.begin(), onCollisionEnterCallbacks.end(), callback), onCollisionEnterCallbacks.end()); 
}

void PhysicsSystem::AddOnCollisionExitCallback(OnCollisionCallback callback){ onCollisionExitCallbacks.push_back(callback); }
void PhysicsSystem::RemoveOnCollisionExitCallback(OnCollisionCallback callback){
    onCollisionExitCallbacks.erase(std::remove(onCollisionExitCallbacks.begin(), onCollisionExitCallbacks.end(), callback), onCollisionExitCallbacks.end());
}

void PhysicsSystem::AddOnTriggerEnterCallback(OnCollisionCallback callback){ onTriggerEnterCallbacks.push_back(callback); }
void PhysicsSystem::RemoveOnTriggerEnterCallback(OnCollisionCallback callback){
    onTriggerEnterCallbacks.erase(std::remove(onTriggerEnterCallbacks.begin(), onTriggerEnterCallbacks.end(), callback), onTriggerEnterCallbacks.end());
}

void PhysicsSystem::AddOnTriggerExitCallback(OnCollisionCallback callback){ onTriggerExitCallbacks.push_back(callback); }
void PhysicsSystem::RemoveOnTriggerExitCallback(OnCollisionCallback callback){
    onTriggerExitCallbacks.erase(std::remove(onTriggerExitCallbacks.begin(), onTriggerExitCallbacks.end(), callback), onTriggerExitCallbacks.end());
}

void PhysicsSystem::AddRigidbody(EntityId entityId, RigidbodyComponent& c, TransformComponent& t){
    LogInfo("Add Rigidbody");
    Rigidbody* data = new Rigidbody();
    data->updating = true;
    data->world = physicsWorld->world;
    
    c.data = data;
    c.data->entityId = entityId;

    Vector3 pos = t.Position();
    Quaternion rot = t.Rotation();

    c.SetShape(c.GetShape());

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(pos.x, pos.y, pos.z));
    transform.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));

    data->motionState = new btDefaultMotionState(transform);

    btVector3 localInertia(0,0,0);
    if(c.mass != 0.0f) data->shape->calculateLocalInertia(c.mass, localInertia);
    
    //data->body = new btRigidBody(c.mass, data->motionState, data->shape, localInertia);
    btRigidBody::btRigidBodyConstructionInfo cInfo(c.mass, data->motionState, data->shape, localInertia);
    data->body = new btRigidBody(cInfo); //data->body = new btRigidBody(c.mass, data->motionState, data->shape);
    data->body->setUserPointer(data);

    //Update Rigidbody
    //c.SetType(c.GetType());
    c.UpdateSettings();
    c.NeverSleep(c.NeverSleep());
    /*data->body->setCcdMotionThreshold(1e-7);
    data->body->setCcdSweptSphereRadius(0.25);*/
    data->body->setCollisionFlags(data->body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

    physicsWorld->world->addRigidBody(data->body);

    data->updating = false;
}

void PhysicsSystem::RemoveRigidbody(EntityId entityId, RigidbodyComponent& rb){
    LogInfo("Remove Rigidbody");
    if(rb.data == nullptr) return;

    Rigidbody* data = rb.data;
    data->world->removeRigidBody(data->body);

    for(auto i = physicsWorld->pairsLastUpdate.begin(); i != physicsWorld->pairsLastUpdate.end(); ){
        if(i->first == data->body || i->second == data->body){
            physicsWorld->pairsLastUpdate.erase(i++);
        } else {
            ++i;
        }
    }

    delete data->shape;
    delete data->motionState;
    delete data->body;
    delete data;
    rb.data = nullptr;
}


#pragma endregion

}