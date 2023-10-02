#include "PhysicsSystem.h"
#include "OD/Core/Application.h"
#include "OD/Scene/Scene.h"
#include <set>
#include <algorithm>

namespace OD{

PhysicsSystemStartup physicsSystemStartup;

#pragma region Core
struct Rigidbody{
    btBoxShape* _shape = nullptr;
    btRigidBody* _body = nullptr;
    btDynamicsWorld* _world = nullptr;
    btDefaultMotionState* _motionState = nullptr;

    EntityId entityId;
};

class Debuger: public btIDebugDraw{
public:
    void drawLine(const btVector3 &from,const btVector3&to, const btVector3 &color) override{
        Renderer::DrawLine(
            Vector3(from.x(), from.y(), from.z()), 
            Vector3(to.x(), to.y(), to.z()), 
            Vector3(color.x(), color.y(), color.z()),
            2
        );
    }

    void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override {
        Transform transform;
        transform.localPosition(Vector3(PointOnB.x(), PointOnB.y(), PointOnB.z()));
        transform.localScale(Vector3(0.25f, 0.25f, 0.25f));

        Renderer::DrawWireCube(
            transform.GetLocalModelMatrix(), 
            Vector3(color.x(), color.y(), color.z()), 
            2
        );

        Renderer::DrawLine(
            transform.localPosition(), 
            transform.localPosition() + Vector3(normalOnB.x(), normalOnB.y(), normalOnB.z()), 
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
void RigidbodyComponent::Serialize(YAML::Emitter& out, Entity& e){
    out << YAML::Key << "RigidbodyComponent";
    out << YAML::BeginMap;

    auto& cam = e.GetComponent<RigidbodyComponent>();
    out << YAML::Key << "mass" << YAML::Value << cam.mass();
    out << YAML::Key << "boxShapeSize" << YAML::Value << cam.shape();
    
    out << YAML::EndMap;
}

void RigidbodyComponent::Deserialize(YAML::Node& in, Entity& e){
    auto& c = e.AddOrGetComponent<RigidbodyComponent>();
    c.mass(in["mass"].as<float>());
    c.shape(in["boxShapeSize"].as<Vector3>());
}

void RigidbodyComponent::OnGui(Entity& e){
    RigidbodyComponent& rb = e.GetComponent<RigidbodyComponent>();

    const char* optionsString[] = {"Dynamic", "Static", "Kinematic", "Trigger"};
    const char* curOptionString = optionsString[(int)rb.type()];
    if(ImGui::BeginCombo("Type", curOptionString)){
        for(int i = 0; i < 4; i++){
            bool isSelected = curOptionString == optionsString[i];
            if(ImGui::Selectable(optionsString[i], isSelected)){
                curOptionString = optionsString[i];
                rb.type((RigidbodyComponent::Type)i);
            }

            if(isSelected) ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    float shape[] = {rb.shape().x, rb.shape().y, rb.shape().z};
    if(ImGui::DragFloat3("shape", shape)){
        rb.shape(Vector3(shape[0], shape[1], shape[2]));
    }

    float mass = rb.mass();
    if(ImGui::DragFloat("mass", &mass)){
        rb.mass(mass);
    }

    bool neverSleep = rb.neverSleep();
    if(ImGui::Checkbox("neverSleep", &neverSleep)){
        rb.neverSleep(neverSleep);
    }
}

void RigidbodyComponent::shape(Vector3 boxShapeSize){
    _boxShapeSize = boxShapeSize;
    _shapeIsDity = false;

    if(_data == nullptr) return;

    if(_data->_shape != nullptr) delete _data->_shape;
    _data->_shape = new btBoxShape(btVector3(_boxShapeSize.x/2, _boxShapeSize.y/2, _boxShapeSize.z/2));
    _data->_body->setCollisionShape(_data->_shape);

    mass(_mass);

    _data->_body->activate(true);
}

void RigidbodyComponent::mass(float m){
    _mass = m;
    _massIsDirty = true;

    //return;
    if(_data == nullptr) return;

    _massIsDirty = false;

    btVector3 localInertia(0,0,0);
    if (_mass != 0.0f) _data->_shape->calculateLocalInertia(_mass, localInertia);
    _data->_body->setMassProps(_mass, localInertia);
    _data->_body->activate(true);
}

void RigidbodyComponent::type(RigidbodyComponent::Type value){
    _type = value;

    if(_data == nullptr) return;

    if(_type == Type::Dynamic) _data->_body->setCollisionFlags(btCollisionObject::CF_DYNAMIC_OBJECT);
    if(_type == Type::Static) _data->_body->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
    if(_type == Type::Kinematic) _data->_body->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
    if(_type == Type::Trigger) _data->_body->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE | btCollisionObject::CF_KINEMATIC_OBJECT);

    _data->_body->activate(true);
}

void RigidbodyComponent::neverSleep(bool value){
    _neverSleep = value;

    if(_data == nullptr) return;

    if(value){
        _data->_body->setActivationState(DISABLE_DEACTIVATION);
    } else {
        _data->_body->setActivationState(ACTIVE_TAG);
    }
}

Vector3 RigidbodyComponent::position(){
    if(_data == nullptr) return Vector3::zero;
    btTransform trans = _data->_body->getWorldTransform();
    return FromBullet(trans.getOrigin());
}

void RigidbodyComponent::position(Vector3 position){
    if(_data == nullptr) return;
    btTransform trans;
    trans.setIdentity();
    trans.setOrigin(ToBullet(position));
    _data->_body->setWorldTransform(trans);
    _data->_motionState->setWorldTransform(trans);

    _data->_body->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
    _data->_body->setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));
    _data->_body->clearForces();
}

Vector3 RigidbodyComponent::velocity(){
    if(_data == nullptr) return Vector3::zero;
    return FromBullet(_data->_body->getLinearVelocity());
}

void RigidbodyComponent::velocity(Vector3 v){
    if(_data == nullptr) return;
    return _data->_body->setLinearVelocity(ToBullet(v));
}

void RigidbodyComponent::ApplyForce(Vector3 v){
    if(_data == nullptr) return;
    _data->_body->applyCentralForce(ToBullet(v));
}

void RigidbodyComponent::ApplyTorque(Vector3 v){
    if(_data == nullptr) return;
    _data->_body->applyTorque(ToBullet(v));
}

void RigidbodyComponent::ApplyImpulse(Vector3 v){
    if(_data == nullptr) return;
    _data->_body->applyCentralImpulse(ToBullet(v));
}

#pragma endregion

#pragma region PhysicsSystem

PhysicsSystem* PhysicsSystem::instance;

PhysicsSystem::PhysicsSystem(){
    _collisionConfiguration = new btDefaultCollisionConfiguration();
    _dispatcher = new btCollisionDispatcher(_collisionConfiguration);
    _broadphase = new btDbvtBroadphase();
    _solver = new btSequentialImpulseConstraintSolver();
    _world = new btDiscreteDynamicsWorld(_dispatcher, _broadphase, _solver, _collisionConfiguration);
    _world->setGravity(btVector3(0.0f, -9.80665f, 0.0f));

    _world->setDebugDrawer(&debuger);
}

PhysicsSystem::~PhysicsSystem(){
    delete _collisionConfiguration;
    delete _dispatcher;
    delete _broadphase;
    delete _solver;
    delete _world;
}

void PhysicsSystem::OnRemoveRigidbody(entt::registry & r, entt::entity e){
    LogInfo("Removing Rigidbody");

    RigidbodyComponent& rb = r.get<RigidbodyComponent>(e);

    if(rb._data == nullptr) return;

    Rigidbody* data = rb._data;
    data->_world->removeRigidBody(data->_body);

    Assert(PhysicsSystem::Get() != nullptr);
    auto& _pairsLastUpdate = PhysicsSystem::Get()->_pairsLastUpdate;
    for(auto i = _pairsLastUpdate.begin(); i != _pairsLastUpdate.end(); ){
        if(i->first == data->_body || i->second == data->_body){
            _pairsLastUpdate.erase(i++);
        } else {
            ++i;
        }
    }

    delete data->_shape;
    delete data->_motionState;
    delete data->_body;
    delete data;
}

void PhysicsSystem::Init(Scene* scene){
    _scene = scene;
    _scene->GetRegistry().on_destroy<RigidbodyComponent>().connect<&OnRemoveRigidbody>();

    instance = this;
}

void PhysicsSystem::Update(){
    if(scene()->running() == false) return;

    if(scene()->running())
        _world->stepSimulation(Application::deltaTime());
    
    auto view = scene()->GetRegistry().view<RigidbodyComponent, TransformComponent>();

    for(auto e: view){
        RigidbodyComponent& rb = view.get<RigidbodyComponent>(e);
        TransformComponent& transform = view.get<TransformComponent>(e);

        if(rb._data == nullptr){
            AddRigidbody(e, rb, transform);
        }

        Assert(rb._data != nullptr);

        Rigidbody* data = rb._data;

        btTransform trans;
		data->_motionState->getWorldTransform(trans);
        //trans = rb->m_pBody->getWorldTransform();

        btVector3 pos = trans.getOrigin();
        btQuaternion rot = trans.getRotation();
        
        transform.position(Vector3(pos.getX(), pos.getY(), pos.getZ()));
        transform.rotation(Quaternion(rot.getX(), rot.getY(), rot.getZ(), rot.getW()));

        if(rb._massIsDirty) SetMass(rb);
        if(rb._shapeIsDity) SetShape(rb);
        
        data->_shape->setLocalScaling(ToBullet(transform.localScale()));
    }

    CheckForCollisionEvents();
}

void PhysicsSystem::CheckForCollisionEvents(){
    // keep a list of the collision pairs we
	// found during the current update
	CollisionPairs pairsThisUpdate;

	// iterate through all of the manifolds in the dispatcher
	for(int i = 0; i < _dispatcher->getNumManifolds(); ++i){
		
		// get the manifold
		btPersistentManifold* pManifold = _dispatcher->getManifoldByIndexInternal(i);
		
		// ignore manifolds that have 
		// no contact points.
		if (pManifold->getNumContacts() > 0){
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
			if(_pairsLastUpdate.find(thisPair) == _pairsLastUpdate.end()) {
                Assert(pBody0 != nullptr);
                Assert(pBody0->getUserPointer() != nullptr);
                Assert(pBody1 != nullptr);
                Assert(pBody1->getUserPointer() != nullptr);
				//CollisionEvent((btRigidBody*)pBody0, (btRigidBody*)pBody1);

                Rigidbody* r1 = static_cast<Rigidbody*>(pBody0->getUserPointer());
                Rigidbody* r2 = static_cast<Rigidbody*>(pBody1->getUserPointer());
                Assert(r1 != nullptr);
                Assert(r2 != nullptr);
                
                Entity e1 = Entity(r1->entityId, _scene);
                Entity e2 = Entity(r2->entityId, _scene);
                Assert(e1.HasComponent<InfoComponent>());
                Assert(e2.HasComponent<InfoComponent>());

                LogInfo("OnCollision %s <==> %s", e1.GetComponent<InfoComponent>().name.c_str(), e2.GetComponent<InfoComponent>().name.c_str());

                RigidbodyComponent& _r1 = e1.GetComponent<RigidbodyComponent>();
                RigidbodyComponent& _r2 = e2.GetComponent<RigidbodyComponent>();

                if(_r1.type() == RigidbodyComponent::Type::Trigger) _r2.ApplyImpulse(Vector3::up * 25.0f);
                //if(_r2.type() == RigidbodyComponent::Type::Trigger) _r1.ApplyImpulse(Vector3::up * 20.0f);
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
	std::set_difference(_pairsLastUpdate.begin(), _pairsLastUpdate.end(),
	pairsThisUpdate.begin(), pairsThisUpdate.end(),
	std::inserter(removedPairs, removedPairs.begin()));
	
	// iterate through all of the removed pairs
	// sending separation events for them
	for(CollisionPairs::const_iterator iter = removedPairs.begin(); iter != removedPairs.end(); ++iter){
        Assert(iter->first != nullptr);
        Assert(iter->first->getUserPointer() != nullptr);
        Assert(iter->second != nullptr);
        Assert(iter->second->getUserPointer() != nullptr);
        //SeparationEvent((btRigidBody*)iter->first, (btRigidBody*)iter->second);

        Rigidbody* r1 = static_cast<Rigidbody*>(iter->first->getUserPointer());
        Rigidbody* r2 = static_cast<Rigidbody*>(iter->second->getUserPointer());
        Assert(r1 != nullptr);
        Assert(r2 != nullptr);

        Entity e1 = Entity(r1->entityId, _scene);
        Entity e2 = Entity(r2->entityId, _scene);
        Assert(e1.HasComponent<InfoComponent>());
        Assert(e2.HasComponent<InfoComponent>());

        LogInfo("OnSeparation %s <==> %s", e1.GetComponent<InfoComponent>().name.c_str(), e2.GetComponent<InfoComponent>().name.c_str());
	}
	
	// in the next iteration we'll want to
	// compare against the pairs we found
	// in this iteration
	_pairsLastUpdate = pairsThisUpdate;
}

void PhysicsSystem::ShowDebugGizmos(){
    //if(scene()->running()) return;

    _world->debugDrawWorld();
}

bool PhysicsSystem::Raycast(Vector3 pos, Vector3 dir, RayResult& hit){
    btVector3 _pos = btVector3(pos.x, pos.y, pos.z);
    btVector3 _dir = btVector3(dir.x, dir.y, dir.z);

    btCollisionWorld::ClosestRayResultCallback rayCallback(_pos, _dir);
    _world->rayTest(_pos, _dir, rayCallback);

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
        hit.entity = Entity(entityId, _scene);
        hit.hitPoint = FromBullet(rayCallback.m_hitPointWorld);
        hit.hitNormal = FromBullet(rayCallback.m_hitNormalWorld);
        return true;
    }

    return false;
}

void PhysicsSystem::AddRigidbody(EntityId entityId, RigidbodyComponent& c, TransformComponent& t){
    LogInfo("Add Rigidbody");
    Rigidbody* data = new Rigidbody();

    c._data = data;
    c._data->entityId = entityId;

    Vector3 pos = t.position();
    Quaternion rot = t.rotation();

    if(data->_shape == nullptr)
        data->_shape = new btBoxShape(btVector3(c._boxShapeSize.x/2, c._boxShapeSize.y/2, c._boxShapeSize.z/2));

    data->_world = _world;

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(pos.x, pos.y, pos.z));
    transform.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));

    data->_motionState = new btDefaultMotionState(transform);

    btVector3 localInertia(0,0,0);
    if (c._mass != 0.0f) data->_shape->calculateLocalInertia(c._mass, localInertia);

    data->_body = new btRigidBody(c._mass, data->_motionState, data->_shape, localInertia);
    data->_body->setUserPointer(data);

    c.type(c.type());
    c.neverSleep(c.neverSleep());

    _world->addRigidBody(data->_body);
}

void PhysicsSystem::SetShape(RigidbodyComponent& rb){
    return;
    Assert(rb._data != nullptr);
    Rigidbody* data = rb._data;

    rb._shapeIsDity = false;

    if(data->_shape != nullptr) delete data->_shape;
    data->_shape = new btBoxShape(btVector3(rb._boxShapeSize.x/2, rb._boxShapeSize.y/2, rb._boxShapeSize.z/2));
    data->_body->setCollisionShape(data->_shape);

    SetMass(rb);

    data->_body->activate(true);

    //_shape->setLocalScaling(btVector3(boxShapeSize.x, boxShapeSize.y, boxShapeSize.z));
    //data->_shape->setSafeMargin(btVector3(rb._boxShapeSize.x/2, rb._boxShapeSize.y/2, rb._boxShapeSize.z/2));
}

void PhysicsSystem::SetMass(RigidbodyComponent& rb){
    return;

    Assert(rb._data != nullptr);
    Rigidbody* data = rb._data;

    rb._massIsDirty = false;

    btVector3 localInertia(0,0,0);
    if (rb._mass != 0.0f) data->_shape->calculateLocalInertia(rb._mass, localInertia);
    data->_body->setMassProps(rb._mass, localInertia);

    data->_body->activate(true);
}
#pragma endregion

}