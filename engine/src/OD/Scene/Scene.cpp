#include "Scene.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include "Scripts.h"
#include "OD/OD.h"
#include "OD/Serialization/Serialization.h"
#include "OD/RendererSystem/CameraComponent.h"

namespace OD{

#pragma region TransformComponent
Matrix4 TransformComponent::globalModelMatrix(){
    if(_hasParent){
        TransformComponent& parent = _registry->get<TransformComponent>(_parent);
        return parent.GetLocalModelMatrix() * GetLocalModelMatrix();
    }
    return GetLocalModelMatrix();
}

Vector3 TransformComponent::InverseTransformDirection(Vector3 dir){
    Matrix4 matrix4 = globalModelMatrix();
    //return matrix4.inverse() * Vector4(dir.x, dir.y, dir.z, 0);
    return matrix4.inverse().MultiplyVector(dir);
}

Vector3 TransformComponent::TransformDirection(Vector3 dir){
    Matrix4 matrix4 = globalModelMatrix();
    //return matrix4 * Vector4(dir.x, dir.y, dir.z, 0);
    return matrix4.MultiplyVector(dir);
}

Vector3 TransformComponent::InverseTransformPoint(Vector3 point){
    Matrix4 matrix4 = globalModelMatrix();
    //return matrix4.inverse() * Vector4(point.x, point.y, point.z, 1);
    return matrix4.inverse().MultiplyPoint(point);
}

Vector3 TransformComponent::TransformPoint(Vector3 point){
    Matrix4 matrix4 = globalModelMatrix();
    //return matrix4 * Vector4(point.x, point.y, point.z, 1);
    return matrix4.MultiplyPoint(point);
}

//Quaternion InverseTransformRot(Quaternion world, Quaternion rot){
//    return Quaternion::Inverse(world) * rot;
//}

Vector3 TransformComponent::position(){ 
    if(_hasParent){
        TransformComponent& parent = _registry->get<TransformComponent>(_parent);
        return parent.TransformPoint(localPosition());
    }
    return localPosition();
}

void TransformComponent::position(Vector3 position){
    if(_hasParent){
        TransformComponent& parent = _registry->get<TransformComponent>(_parent);
        localPosition(parent.InverseTransformPoint(position));
    } else {
        localPosition(position);
    }
}

Quaternion TransformComponent::rotation(){
    if(_hasParent){
        TransformComponent& parent = _registry->get<TransformComponent>(_parent);
        return parent.rotation() * localRotation();
    } 
    return localRotation();
}

void TransformComponent::rotation(Quaternion rotation){
    //Assert(false);
    if(_hasParent){
        TransformComponent& parent = _registry->get<TransformComponent>(_parent);
        Quaternion world = parent.rotation();
        //localRotation(InverseTransformRot(world, rotation));
        localRotation(Quaternion::Inverse(world) * rotation);
    } else {
        localRotation(rotation);
    }
}
#pragma endregion

#pragma region InfoComponent
#pragma endregion

#pragma region Scene

void Scene::TransformSerialize(YAML::Emitter& out, Entity& e){
    out << YAML::Key << "TransformComponent";
    out << YAML::BeginMap;

    auto& transform = e.GetComponent<TransformComponent>();
    out << YAML::Key << "localPosition" << YAML::Value << transform.localPosition();
    out << YAML::Key << "localRotation" << YAML::Value << transform.localRotation();
    out << YAML::Key << "localScale" << YAML::Value << transform.localScale();

    out << YAML::Key << "Children" << YAML::Value << YAML::BeginSeq;
    for(auto i: transform._children){
        Entity entity = Entity(i, this);
        SerializeEntity(out, entity);
    }
    out << YAML::EndSeq;
    
    out << YAML::EndMap;
}

void Scene::TransformDeserialize(YAML::Node& in, Entity& e){
    auto& tc = e.GetComponent<TransformComponent>();
    tc.localPosition(in["localPosition"].as<Vector3>());
    tc.localRotation(in["localRotation"].as<Quaternion>());
    tc.localScale(in["localScale"].as<Vector3>());

    auto entities = in["Children"];
    if(entities){
        for(auto _e: entities){
            Entity children = DeserializeEntity(_e);
            SetParent(e.id(), children.id());
        }
    }
}

void Scene::InfoSerialize(YAML::Emitter& out, Entity& e){
    out << YAML::Key << "InfoComponent";
    out << YAML::BeginMap;

    auto& info = e.GetComponent<InfoComponent>();
    out << YAML::Key << "name" << YAML::Value << info.name;
    
    out << YAML::EndMap;
}

void Scene::InfoDeserialize(YAML::Node& in, Entity& e){
    auto& tc = e.GetComponent<InfoComponent>();
    tc.name = in["name"].as<std::string>();
}

Scene::Scene(){
    for(auto i: SceneManager::Get()._addSystemFuncs){
        LogInfo("Adding system: %s", i.first.c_str());
        i.second(*this);
    }
}

Scene::~Scene(){
    for(auto i: _standSystems) delete i;
    for(auto i: _rendererSystems) delete i;
    for(auto i: _physicsSystems) delete i;
}

Scene* Scene::Copy(Scene* other){
    Scene* scene = new Scene();

    auto view = other->_registry.view<InfoComponent, TransformComponent>();
    for(auto i: view){
        entt::entity e = scene->_registry.create(i);

        auto& c = view.get<TransformComponent>(i);
        TransformComponent& nt = scene->_registry.emplace_or_replace<TransformComponent>(e, c);
        nt._registry = &scene->_registry;

        auto& c2 = view.get<InfoComponent>(i);
        scene->_registry.emplace_or_replace<InfoComponent>(e, c2);
    }

    for(auto i: SceneManager::Get()._coreComponents){
        i.second.copy(scene->_registry, other->_registry);
    }
    for(auto i: SceneManager::Get()._serializeFuncs){
        i.second.copy(scene->_registry, other->_registry);
    }

    return scene;
}

Entity Scene::GetMainCamera2(){
    for(auto e: _registry.view<CameraComponent>()){
        return Entity(e, this);
    }

    return Entity();
}

void Scene::SerializeEntity(YAML::Emitter& out, Entity& e){
    out << YAML::BeginMap;
    //out << YAML::Key << "Entity" << YAML::Value << "10";

    if(e.HasComponent<InfoComponent>()) InfoSerialize(out, e);
    if(e.HasComponent<TransformComponent>()) TransformSerialize(out, e);

    for(auto func: SceneManager::Get()._coreComponents){
        if(func.second.hasComponent(e) == false) continue;
        func.second.serialize(out, e);
    }

    for(auto func: SceneManager::Get()._serializeFuncs){
        if(func.second.hasComponent(e) == false) continue;
        
        ArchiveNode s(ArchiveNode::Type::Object, func.first, nullptr);
        func.second.serialize(e, s); Assert(s.values.empty() == false);
        ArchiveNode::SaveSerializer(s, func.first, out);
    }

    out << YAML::EndMap;
}

Entity Scene::DeserializeEntity(YAML::Node& e){
    Entity deserializedEntity = AddEntity();

    auto infoComponent = e["InfoComponent"];
    if(infoComponent) InfoDeserialize(infoComponent, deserializedEntity);

    auto transform = e["TransformComponent"];
    if(transform) TransformDeserialize(transform, deserializedEntity);

    for(auto func: SceneManager::Get()._coreComponents){
        auto component = e[func.first];
        if(component){
            func.second.deserialize(component, deserializedEntity);
        }
    }

    for(auto func: SceneManager::Get()._serializeFuncs){
        auto component = e[func.first];
        if(component){
            LogInfo("%s", func.first.c_str());

            ArchiveNode s(ArchiveNode::Type::Object, func.first, nullptr);
            func.second.serialize(deserializedEntity, s);
            //LoadSerializer(s, component);
            ArchiveNode::LoadSerializer(s, component);
        }
    }

    return deserializedEntity;
}

void Scene::Save(const char* path){
    YAML::Emitter out;

    out << YAML::BeginMap;
    out << YAML::Key << "Scene" << YAML::Value << "Untitled";
    out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

    auto v = _registry.view<entt::entity>();
    for(auto it = v.rbegin(); it != v.rend(); ++it){
        Entity e(*it, this);
        if(e.IsValid() == false) return;
        TransformComponent& transform = e.GetComponent<TransformComponent>();
        if(transform.hasParent()) return;

        SerializeEntity(out, e);
    }

    out << YAML::EndSeq;
    out << YAML::EndMap;

    std::ofstream fout(path);
    fout << out.c_str();
}

void Scene::Load(const char* path){
    std::ifstream stream(path);
    std::stringstream strStream;
    strStream << stream.rdbuf();

    YAML::Node data = YAML::Load(strStream.str());
    if(!data["Scene"]){
        LogInfo("Coud not load Scene: %s", path);
        return;
    }

    std::string sceneName = data["Scene"].as<std::string>();
    LogInfo("Deserialing scene %s", sceneName.c_str());

    auto entities = data["Entities"];
    if(entities){
        for(auto e: entities){
            DeserializeEntity(e);
        }
    }
}
#pragma endregion

#pragma region SceneManager

#pragma endregion

}