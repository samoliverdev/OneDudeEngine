#include "PhysicsSystem.h"
#include "OD/Core/Application.h"
#include "OD/Scene/Scene.h"
//#include "OD/Phys.h"

namespace OD{

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
    
}

struct Rigidbody{
    btBoxShape* _shape = nullptr;
    btRigidBody* _body = nullptr;
    btDynamicsWorld* _world = nullptr;
    btDefaultMotionState* _motionState = nullptr;
};

PhysicsSystem::PhysicsSystem(){
    _collisionConfiguration = new btDefaultCollisionConfiguration();
    _dispatcher = new btCollisionDispatcher(_collisionConfiguration);
    _broadphase = new btDbvtBroadphase();
    _solver = new btSequentialImpulseConstraintSolver();
    _world = new btDiscreteDynamicsWorld(_dispatcher, _broadphase, _solver, _collisionConfiguration);
    _world->setGravity(btVector3(0.0f, -9.80665f, 0.0f));
}

PhysicsSystem::~PhysicsSystem(){
    delete _collisionConfiguration;
    delete _dispatcher;
    delete _broadphase;
    delete _solver;
    delete _world;
}

void PhysicsSystem::OnRemoveRigidbody(entt::registry & r, entt::entity e){
    //LogInfo("Removing Rigidbody");

    RigidbodyComponent& rb = r.get<RigidbodyComponent>(e);

    Rigidbody* data = rb._data;

    data->_world->removeRigidBody(data->_body);
    delete data->_shape;
    delete data->_motionState;
    delete data->_body;
    delete data;
}

void PhysicsSystem::Init(Scene* scene){
    _scene = scene;
    _scene->GetRegistry().on_destroy<RigidbodyComponent>().connect<&OnRemoveRigidbody>();
}

void PhysicsSystem::Update(){
    _world->stepSimulation(Application::deltaTime());

    auto view = scene()->GetRegistry().view<RigidbodyComponent, TransformComponent>();

    for(auto e: view){
        RigidbodyComponent& rb = view.get<RigidbodyComponent>(e);
        TransformComponent& transform = view.get<TransformComponent>(e);

        if(rb._data == nullptr){
            AddRigidbody(rb, transform);
        }

        Assert(rb._data != nullptr);

        //Rigidbody* data = static_cast<Rigidbody*>(rb->data);
        Rigidbody* data = rb._data;

        btTransform trans;
		data->_motionState->getWorldTransform(trans);
        //trans = rb->m_pBody->getWorldTransform();

        btVector3 pos = trans.getOrigin();
        btQuaternion rot = trans.getRotation();
        
        transform.position(Vector3(pos.getX(), pos.getY(), pos.getZ()));
        transform.rotation(Quaternion(rot.getX(), rot.getY(), rot.getZ(), rot.getW()));
    }
}

void PhysicsSystem::AddRigidbody(RigidbodyComponent& c, TransformComponent& t){
    LogInfo("Add Rigidbody");
    Rigidbody* data = new Rigidbody();

    c._data = data;

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

    _world->addRigidBody(data->_body);
}

void PhysicsSystem::SetShape(RigidbodyComponent& rb){
    Assert(rb._data != nullptr);
    //Rigidbody* data = static_cast<Rigidbody*>(rb->data);
    Rigidbody* data = rb._data;

    if(data->_shape != nullptr) delete data->_shape;
    data->_shape = new btBoxShape(btVector3(rb._boxShapeSize.x/2, rb._boxShapeSize.y/2, rb._boxShapeSize.z/2));
    data->_body->setCollisionShape(data->_shape);

    //_shape->setLocalScaling(btVector3(boxShapeSize.x, boxShapeSize.y, boxShapeSize.z));
}

void PhysicsSystem::SetMass(RigidbodyComponent& rb){
    Assert(rb._data != nullptr);
    //Rigidbody* data = static_cast<Rigidbody*>(rb->data);
    Rigidbody* data = rb._data;

    //if(_shape == nullptr) _shape = new btBoxShape(btVector3(boxShapeSize.x/2, boxShapeSize.y/2, boxShapeSize.z/2));

    btVector3 localInertia(0,0,0);
    if (rb._mass != 0.0f) data->_shape->calculateLocalInertia(rb._mass, localInertia);
    
    //if(_body == nullptr) return;
    data->_body->setMassProps(rb._mass, localInertia);
}

}