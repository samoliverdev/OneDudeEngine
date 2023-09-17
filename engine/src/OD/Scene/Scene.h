#pragma once

#include "OD/Defines.h"
#include "OD/Core/Transform.h"
#include "OD/Core/AssetManager.h"
#include "OD/Serialization/Archive.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"
#include "OD/Renderer/Renderer.h"
#include "OD/Renderer/Camera.h"
#include <unordered_map>
#include <string>
#include <entt/entt.hpp>
#include <functional>
#include <yaml-cpp/yaml.h>

namespace OD {

using EntityId = entt::entity;

struct Entity;
struct System;
struct Scene;

class TransformComponent: public Transform{
    static void Serialize(YAML::Emitter& out, Entity& e);
    static void Deserialize(YAML::Node& in, Entity& e);
    static void OnGui(Entity& e);
    
    friend struct Scene;

public:
    Matrix4 globalModelMatrix();
    
    Vector3 InverseTransformDirection(Vector3 dir); //Transforms a direction from world space to local space. The opposite of Transform.TransformDirection.
    Vector3 TransformDirection(Vector3 dir); //	Transforms position from local space to world space.

    Vector3 InverseTransformPoint(Vector3 point); //Transforms position from world space to local space.
    Vector3 TransformPoint(Vector3 point); //Transforms direction from local space to world space.

    Vector3 position();
    void position(Vector3 position);

    Quaternion rotation();
    void rotation(Quaternion rotation);

    inline EntityId parent(){ return _parent; }
    inline bool hasParent(){ return _hasParent; }
    inline std::vector<EntityId> children(){ return _children; }

private:
    std::vector<EntityId> _children;

    EntityId _parent;
    bool _hasParent;

    entt::registry* _registry;
};

struct InfoComponent{
    static void Serialize(YAML::Emitter& out, Entity& e);
    static void Deserialize(YAML::Node& in, Entity& e);
    static void OnGui(Entity& e);
    
    friend struct Scene;

    std::string name = "Entity";

private:
    bool _active;
};

struct Entity{
    friend struct Scene;
public:

    Entity():
        _isValid(false){}

    Entity(EntityId id, Scene* scene):
        _id(id), 
        _scene(scene),
        _isValid(true){}

    template<typename T>
    T& AddComponent(){
        return _scene->_registry.emplace<T>(_id);
    }

    template <typename T>
    T& GetComponent(){
        return _scene->_registry.get<T>(_id);
    }

    template<typename T>
    bool HasComponent(){
        return _scene->_registry.any_of<T>(_id);
    }

    template<typename T>
    T& AddOrGetComponent(){
        if(HasComponent<T>() == false) return AddComponent<T>();
        return GetComponent<T>();
    }

    template<typename T>
    void RemoveComponent(){
        Assert(HasComponent<T>() && "Entity does not have component!");
        _scene->_registry.remove<T>(_id);
    }

    inline bool IsValid(){ return _isValid; }
    inline EntityId id(){ return _id; }
    inline Scene* scene(){ return _scene; }

    bool operator==(const Entity& other) const {
        return _id == other._id && _scene == other._scene;
    }

    bool operator!=(const Entity& other) const {
        return !(*this == other);
    }

private:
    EntityId _id;
    Scene* _scene;
    bool _isValid = false;
};

enum class SystemType{
    Stand, Renderer, Physics
};

struct System{
    virtual SystemType Type(){ return SystemType::Stand; }

    inline virtual void Init(Scene* scene){ _scene = scene; }
    virtual void Update(){}
    
    Scene* scene(){ return _scene; }

protected:
    Scene* _scene;
};

struct Scene: public Asset {
    friend struct Entity;

    inline bool running(){ return _running; }

    Scene();
    ~Scene();

    Entity AddEntity(std::string name = "Entity"){
        EntityId e = _registry.create();

        InfoComponent& info = _registry.emplace<InfoComponent>(e);
        info.name = name;
        
        TransformComponent& transform = _registry.emplace<TransformComponent>(e);
        transform._registry = &_registry;
    
        return Entity(e, this);
    }

    void DestroyEntity(EntityId entity){
        _toDestroy.push_back(entity);
    }

    void SetParent(EntityId parent, EntityId child){
        TransformComponent& _parent = _registry.get<TransformComponent>(parent);
        TransformComponent& _child = _registry.get<TransformComponent>(child);

        _parent._children.emplace_back(child);
        _child._parent = parent;
        _child._hasParent = true;
    }
    
    Camera& GetMainCamera(){ return _mainCamera; }

    template <typename T>
    inline void AddSystem(){
        static_assert(std::is_base_of<OD::System, T>::value);
        Assert(_systems.find(GetType<T>()) == _systems.end() && "System Already has been added");

        auto new_system = new T();
        new_system->Init(this);

        _systems[GetType<T>()] = new_system;
        if(new_system->Type() == SystemType::Stand) _standSystems.push_back(new_system);
        if(new_system->Type() == SystemType::Renderer) _rendererSystems.push_back(new_system);
        if(new_system->Type() == SystemType::Physics) _physicsSystems.push_back(new_system);
    }

    template<typename T>
    inline T* GetSystem(){
        static_assert(std::is_base_of<OD::System, T>::value);

        if(_systems.find(GetType<T>()) == _systems.end()) return nullptr;
        return static_cast<T*>(_systems[GetType<T>()]);
    }

    Entity GetMainCamera2();

    void Start(){
        _running = true;
    }

    void Update(){ 
        for(auto e: _toDestroy){
            _DestroyEntity(e);
        }
        _toDestroy.clear();

        if(_running == false) return;

        for(auto s: _physicsSystems) s->Update();
        for(auto s: _standSystems) s->Update();
    }

    void Draw(){
        Renderer::Begin();
        for(auto& s: _rendererSystems) s->Update();        
        Renderer::End();
    }

    entt::registry& GetRegistry(){ return _registry; }

    void Save(const char* path);
    void Load(const char* path);

private:
    void _DestroyEntity(EntityId entity){
        TransformComponent& transform = _registry.get<TransformComponent>(entity);

        for(auto i: transform._children){
            DestroyEntity(i);
        }

        if(transform._hasParent){
            TransformComponent& parent = _registry.get<TransformComponent>(transform._parent);
            parent._children.clear();
        }

        _registry.destroy(entity);
    }
    
    void SerializeEntity(YAML::Emitter& out, Entity& e);
    void ApplySerializer(Archive& s, std::string name, YAML::Emitter& out);
    void LoadSerializer(Archive& s, YAML::Node& node);

    bool _running = false;

    Camera _mainCamera;  

    std::vector<System*> _standSystems;
    std::vector<System*> _rendererSystems;
    std::vector<System*> _physicsSystems;
    std::unordered_map<Type, System*> _systems;
    std::vector<EntityId> _toDestroy;

    entt::registry _registry;
};

struct SceneManager{
    friend class Editor;
    friend class SceneHierarchyPanel;
    friend struct Scene;
    friend struct ScriptComponent;

    enum class SceneState {Playing, Paused, Editor};

    inline static SceneManager& Get(){
        static SceneManager sceneManager;
        return sceneManager;
    }

    inline SceneState sceneState(){ return _sceneState; }
    inline bool inEditor(){ return _inEditor; }

    inline Scene* activeScene(){ 
        if(_activeScene == nullptr) return NewScene();
        return _activeScene; 
    }

    inline Scene* NewScene(){
        delete _activeScene;
        _activeScene = new Scene();
        return _activeScene;
    }

    template<typename T>
    void RegisterComponent(std::string name){
        Assert(_serializeFuncs.find(name) == _serializeFuncs.end());

        SerializeFuncs funcs;

        funcs.hasComponent = [](Entity& e){
            return e.HasComponent<T>();
        };

        funcs.serialize = [](Entity& e, Archive& s){
            auto& c = e.AddOrGetComponent<T>();
            c.Serialize(s);
        };
        
        _serializeFuncs[name] = funcs;
    }

    template<typename T>
    void RegisterScript(std::string name){
        Assert(_serializeScriptFuncs.find(name) == _serializeScriptFuncs.end());

        SerializeFuncs funcs;

        funcs.hasComponent = [](Entity& e){
            if(e.HasComponent<ScriptComponent>() == false) return false;

            auto& c = e.GetComponent<ScriptComponent>();
            return c.HasScript<T>();
        };

        funcs.serialize = [](Entity& e, Archive& s){
            auto& script = e.AddOrGetComponent<ScriptComponent>();
            auto* c = script.AddOrGetScript<T>();

            c->Serialize(s);
        };
        
        _serializeScriptFuncs[name] = funcs;
    }

    template<typename T>
    void RegisterSystem(std::string name){
        Assert(_addSystemFuncs.find(name) == _addSystemFuncs.end());

        _addSystemFuncs[name] = [&](Scene& e){
            e.AddSystem<T>();
        };
    }   

    void DrawArchive(Archive& ar);

private:
    struct SerializeFuncs{
        std::function<bool(Entity&)> hasComponent;
        std::function<void(Entity&,Archive&)> serialize;
    };

    SceneState _sceneState;
    bool _inEditor;

    Scene* _activeScene;

    std::unordered_map<std::string, SerializeFuncs> _serializeFuncs;
    std::unordered_map<std::string, SerializeFuncs> _serializeScriptFuncs;
    std::unordered_map<std::string, std::function<void(Scene&)> > _addSystemFuncs;
};

}