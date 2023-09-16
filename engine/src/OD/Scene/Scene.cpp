#include "Scene.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include "Scripts.h"
#include "OD/OD.h"
#include "OD/Serialization/Serialization.h"
#include "OD/RendererSystem/CameraComponent.h"

namespace OD{

#pragma region TransformComponent
void TransformComponent::Serialize(YAML::Emitter& out, Entity& e){
    out << YAML::Key << "TransformComponent";
    out << YAML::BeginMap;

    auto& transform = e.GetComponent<TransformComponent>();
    out << YAML::Key << "localPosition" << YAML::Value << transform.localPosition();
    out << YAML::Key << "localRotation" << YAML::Value << transform.localRotation();
    out << YAML::Key << "localScale" << YAML::Value << transform.localScale();
    
    out << YAML::EndMap;
}

void TransformComponent::Deserialize(YAML::Node& in, Entity& e){
    auto& tc = e.GetComponent<TransformComponent>();
    tc.localPosition(in["localPosition"].as<Vector3>());
    tc.localRotation(in["localRotation"].as<Quaternion>());
    tc.localScale(in["Scale"].as<Vector3>());
}

void TransformComponent::OnGui(Entity& e){
    
}

Matrix4 TransformComponent::globalModelMatrix(){
    if(_hasParent){
        TransformComponent& parent = _registry->get<TransformComponent>(_parent);
        return parent.GetLocalModelMatrix() * GetLocalModelMatrix();
    }
    return GetLocalModelMatrix();
}

Vector3 TransformComponent::InverseTransformDirection(Vector3 dir){
    Matrix4 matrix4 = globalModelMatrix();
    return matrix4.inverse() * Vector4(dir.x, dir.y, dir.z, 0);
    //return matrix4.inverse().MultiplyVector(dir);
}

Vector3 TransformComponent::TransformDirection(Vector3 dir){
    Matrix4 matrix4 = globalModelMatrix();
    return matrix4 * Vector4(dir.x, dir.y, dir.z, 0);
    //return matrix4.MultiplyVector(dir);
}

Vector3 TransformComponent::InverseTransformPoint(Vector3 point){
    Matrix4 matrix4 = globalModelMatrix();
    return matrix4.inverse() * Vector4(point.x, point.y, point.z, 1);
    //return matrix4.inverse().MultiplyPoint(point);
}

Vector3 TransformComponent::TransformPoint(Vector3 point){
    Matrix4 matrix4 = globalModelMatrix();
    return matrix4 * Vector4(point.x, point.y, point.z, 1);
    //return matrix4.MultiplyPoint(point);
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
void InfoComponent::Serialize(YAML::Emitter& out, Entity& e){
    out << YAML::Key << "InfoComponent";
    out << YAML::BeginMap;

    auto& info = e.GetComponent<InfoComponent>();
    out << YAML::Key << "name" << YAML::Value << info.name;
    
    out << YAML::EndMap;
}

void InfoComponent::Deserialize(YAML::Node& in, Entity& e){

}

void InfoComponent::OnGui(Entity& e){
    
}
#pragma endregion

#pragma region Scene
void ApplySerializer(Archive& s, std::string name, YAML::Emitter& out){
    out << YAML::Key << name;
    out << YAML::BeginMap;
    for(auto i: s.values()){
        if(i.type == ArchiveValue::Type::Float) out << YAML::Key << i.name << YAML::Value << (*i.floatValue);
        if(i.type == ArchiveValue::Type::Int) out << YAML::Key << i.name << YAML::Value << (*i.intValue);
        if(i.type == ArchiveValue::Type::String) out << YAML::Key << i.name << YAML::Value << (*i.stringValue);
        if(i.type == ArchiveValue::Type::Vector3) out << YAML::Key << i.name << YAML::Value << (*i.vector3Value);
        if(i.type == ArchiveValue::Type::Vector4) out << YAML::Key << i.name << YAML::Value << (*i.vector4Value);
        if(i.type == ArchiveValue::Type::Quaternion) out << YAML::Key << i.name << YAML::Value << (*i.quaternionValue);
    }
    out << YAML::EndMap;
}

void LoadSerializer(Archive& s, YAML::Node& node){
    for(auto i: s.values()){
        if(i.type == ArchiveValue::Type::Float){
            *i.floatValue = node[i.name].as<float>();
        }
        if(i.type == ArchiveValue::Type::Int){
            *i.intValue = node[i.name].as<int>();
        }
        if(i.type == ArchiveValue::Type::Vector3){
            *i.vector3Value = node[i.name].as<Vector3>();
        }
        if(i.type == ArchiveValue::Type::Vector4){
            *i.vector4Value = node[i.name].as<Vector4>();
        }
        if(i.type == ArchiveValue::Type::Quaternion){
            *i.quaternionValue = node[i.name].as<Quaternion>();
        }
        if(i.type == ArchiveValue::Type::String){
            *i.stringValue = node[i.name].as<std::string>();
        }
    }
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

Entity Scene::GetMainCamera2(){
    for(auto e: _registry.view<CameraComponent>()){
        return Entity(e, this);
    }

    return Entity();
}

void Scene::SerializeEntity(YAML::Emitter& out, Entity& e){
    out << YAML::BeginMap;
    //out << YAML::Key << "Entity" << YAML::Value << "10";

    if(e.HasComponent<InfoComponent>()) InfoComponent::Serialize(out, e);
    if(e.HasComponent<TransformComponent>()) TransformComponent::Serialize(out, e);
    if(e.HasComponent<CameraComponent>()) CameraComponent::Serialize(out, e);
    if(e.HasComponent<RigidbodyComponent>()) RigidbodyComponent::Serialize(out, e);
    if(e.HasComponent<MeshRendererComponent>()) MeshRendererComponent::Serialize(out, e);

    if(e.HasComponent<ScriptComponent>()){
        auto& component = e.GetComponent<ScriptComponent>();

        out << YAML::Key << "ScriptComponent";
        out << YAML::BeginMap;

        for(auto i: component._instances){
            Archive s;
            i.second->Serialize(s);
            if(s.values().empty() == false) continue;

            ApplySerializer(s, s.name(), out);
        }
       
        out << YAML::EndMap;
    }

    for(auto func: SceneManager::Get()._serializeFuncs){
        if(func.second.hasComponent(e) == false) continue;
        
        Archive s;
        func.second.serialize(e, s);

        Assert(s.Values().empty() == false);

        ApplySerializer(s, func.first, out);
    }

    out << YAML::EndMap;
}

void Scene::Save(const char* path){
    YAML::Emitter out;

    out << YAML::BeginMap;
    out << YAML::Key << "Scene" << YAML::Value << "Untitled";
    out << YAML::Key << "Entities" << YAML::BeginSeq;
    _registry.view<TransformComponent, InfoComponent>().each([&](auto entityId, auto& transform, auto& info){
        Entity e(entityId, this);
        if(e.IsValid() == false) return;

        SerializeEntity(out, e);
    });
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
            std::string name;

            auto infoComponent = e["InfoComponent"];
            if(infoComponent){
                name = infoComponent["Name"].as<std::string>();
            }

            Entity deserializedEntity = AddEntity(name);

            LogInfo("Loading Enity: %s", name.c_str());

            auto transform = e["TransformComponent"];
            if(transform) TransformComponent::Deserialize(transform, deserializedEntity);

            auto cam = e["CameraComponent"];
            if(cam) CameraComponent::Deserialize(cam, deserializedEntity);

            auto rb = e["RigidbodyComponent"];
            if(rb) RigidbodyComponent::Deserialize(rb, deserializedEntity);

            auto meshRenderer = e["MeshRendererComponent"];
            if(meshRenderer) MeshRendererComponent::Deserialize(meshRenderer, deserializedEntity);

            auto script = e["ScriptComponent"];
            if(script){
                for(auto func: SceneManager::Get()._serializeScriptFuncs){
                    auto component = script[func.first];
                    if(component){
                        LogInfo("%s", func.first.c_str());

                        Archive s;
                        func.second.serialize(deserializedEntity, s);
                        LoadSerializer(s, component);
                    }
                }
            }

            for(auto func: SceneManager::Get()._serializeFuncs){
                auto component = e[func.first];
                if(component){
                    LogInfo("%s", func.first.c_str());

                    Archive s;
                    func.second.serialize(deserializedEntity, s);
                    LoadSerializer(s, component);
                }
            }
        }
    }
}
#pragma endregion

}