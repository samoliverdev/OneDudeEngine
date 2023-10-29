#pragma once

#include "OD/Defines.h"
#include "OD/Core/Transform.h"
#include "OD/Core/AssetManager.h"
#include "OD/Serialization/Archive.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"
#include "OD/Renderer/Renderer.h"
#include "OD/Renderer/Camera.h"
#include "OD/Core/Instrumentor.h"
#include <unordered_map>
#include <string>
#include <entt/entt.hpp>
#include <functional>
#include <algorithm>
#include <yaml-cpp/yaml.h>

namespace OD {

using EntityId = entt::entity;

struct Entity;
struct System;
struct Scene;

class TransformComponent: public Transform{
    friend struct Scene;

public:
    inline Vector3 forward(){ return rotation() * Vector3Forward; }
    inline Vector3 back(){ return rotation() * Vector3Back; }
    inline Vector3 left(){ return rotation() * Vector3Left; }
    inline Vector3 right(){ return rotation() * Vector3Right; }
    inline Vector3 up(){ return rotation() * Vector3Up; }
    inline Vector3 down(){ return rotation() * Vector3Down; }

    Matrix4 globalModelMatrix();
    
    Vector3 InverseTransformDirection(Vector3 dir); //Transforms a direction from world space to local space. The opposite of Transform.TransformDirection.
    Vector3 TransformDirection(Vector3 dir); //Transforms direction from local space to world space.

    Vector3 InverseTransformPoint(Vector3 point); //Transforms position from world space to local space.
    Vector3 TransformPoint(Vector3 point);  //Transforms position from local space to world space.

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
    friend struct Scene;

    std::string name = "Entity";
    std::string tag =  "";

    inline EntityId id() const { return _id; }

private:
    bool _active;
    EntityId _id;
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

    static Scene* Copy(Scene* other);

    Entity AddEntity(std::string name = "Entity"){
        EntityId e = _registry.create();
        
        InfoComponent& info = _registry.emplace<InfoComponent>(e);
        info.name = name;
        info._id = e;
        
        TransformComponent& transform = _registry.emplace<TransformComponent>(e);
        transform._registry = &_registry;
    
        return Entity(e, this);
    }
    
    void DestroyEntity(EntityId entity){
        //_DestroyEntity(entity);
        _toDestroy.push_back(entity);
    }

    bool IsChildOf(EntityId parent, EntityId child){
        TransformComponent& _parent = _registry.get<TransformComponent>(parent);

        for(auto i: _parent._children){
            if(i == child) return true;
            bool r = IsChildOf(i, child);
            if(r == true) return true;
        }

        return false;
    }

    /*bool IsParentOf(EntityId parent, EntityId entity){
        TransformComponent& _entity = _registry.get<TransformComponent>(parent);

        if(_entity._hasParent){
            if(_entity.parent() == parent) return true;
            return IsParentOf(_entity.parent(), entity);
        }

        return false;
    }*/

    void CleanParent(EntityId entity){
        TransformComponent& _entity = _registry.get<TransformComponent>(entity);

        if(_entity.hasParent()){
            TransformComponent& _p = _registry.get<TransformComponent>(_entity.parent());
            _p._children.erase(
                std::remove(_p._children.begin(), _p._children.end(), entity),
                _p._children.end()
            );
        }

        _entity._hasParent = false;
    }

    void SetParent(EntityId parent, EntityId child){
        if(parent == child){
            LogWarning("ERROR: Trying set parent with itself");
            return;
        }

        TransformComponent& _parent = _registry.get<TransformComponent>(parent);
        TransformComponent& _child = _registry.get<TransformComponent>(child);

        if(IsChildOf(child, parent)){
            LogWarning("ERROR: Trying set parent with one of your childrens");
            return;
        }

        if(_child.hasParent()){
            TransformComponent& _p = _registry.get<TransformComponent>(_child.parent());
            _p._children.erase(
                std::remove(_p._children.begin(), _p._children.end(), child),
                _p._children.end()
            );
        }

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
        OD_PROFILE_SCOPE("Scene::Update");

        for(auto e: _toDestroy){
            _DestroyEntity(e);
        }
        _toDestroy.clear();

        //if(_running == false) return;

        for(auto s: _physicsSystems) s->Update();
        if(_running == false) return;
        for(auto s: _standSystems) s->Update();
    }

    void Draw(){
        OD_PROFILE_SCOPE("Scene::Draw");

        Renderer::Begin();
        for(auto& s: _rendererSystems) s->Update();        
        Renderer::End();
    }

    entt::registry& GetRegistry(){ return _registry; }

    void Save(const char* path);
    void Load(const char* path);

private:
    Entity _AddEntity(EntityId targetId, std::string name = "Entity"){
        EntityId e = _registry.create(targetId);
        
        InfoComponent& info = _registry.emplace<InfoComponent>(e);
        info.name = name;
        info._id = e;
        
        TransformComponent& transform = _registry.emplace<TransformComponent>(e);
        transform._registry = &_registry;
    
        return Entity(e, this);
    }

    void _DestroyEntity(EntityId entity){
        TransformComponent& transform = _registry.get<TransformComponent>(entity);

        for(auto i: transform._children){
            _DestroyEntity(i);
        }

        if(transform._hasParent){
            TransformComponent& parent = _registry.get<TransformComponent>(transform._parent);
            //parent._children.clear();
            parent._children.erase(
                std::remove(
                    parent._children.begin(), 
                    parent._children.end(), 
                    entity
                ), 
                parent._children.end()
            );
        }

        _registry.destroy(entity);
    }
    
    void SerializeEntity(YAML::Emitter& out, Entity& e);
    Entity DeserializeEntity(YAML::Node& e);

    void TransformSerialize(YAML::Emitter& out, Entity& e);
    void TransformDeserialize(YAML::Node& in, Entity& e);
    void InfoSerialize(YAML::Emitter& out, Entity& e);
    void InfoDeserialize(YAML::Node& in, Entity& e);

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
    friend class InspectorPanel;
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

    inline void activeScene(Scene* s){ 
        _activeScene = s; 
    }

    inline Scene* NewScene(){
        delete _activeScene;
        _activeScene = new Scene();
        return _activeScene;
    }

    template<typename T>
    void RegisterCoreComponent(const char* name){
        Assert(_coreComponentsSerializer.find(name) == _coreComponentsSerializer.end());
        //LogInfo("OnRegisterCoreComponent: %s", name.c_str());

        CoreComponent funcs;

        funcs.hasComponent = [](Entity& e){
            return e.HasComponent<T>();
        };

        funcs.addComponent = [](Entity& e){
            e.AddOrGetComponent<T>();
        };

        funcs.removeComponent = [](Entity& e){
            e.RemoveComponent<T>();
        };

        funcs.serialize = [](YAML::Emitter& out, Entity& e){
            e.AddOrGetComponent<T>();
            T::Serialize(out, e);
        };

        funcs.deserialize = [](YAML::Node& in, Entity& e){
            e.AddOrGetComponent<T>();
            T::Deserialize(in, e);
        };

        funcs.onGui = [](Entity& e){
            e.AddOrGetComponent<T>();
            T::OnGui(e);
        };

        funcs.copy = [](entt::registry& dst, entt::registry& src){
            auto view = src.view<T>();
            for(auto e: view){
                T& c = view.template get<T>(e);
                dst.emplace_or_replace<T>(e, c);
            }
        };
        
        _coreComponentsSerializer[name] = funcs;
    }

        template<typename T>
    void RegisterCoreComponentSimple(const char* name){
        Assert(_coreComponentsSerializer.find(name) == _coreComponentsSerializer.end());
        //LogInfo("OnRegisterCoreComponent: %s", name.c_str());

        CoreComponent funcs;

        funcs.hasComponent = [](Entity& e){
            return e.HasComponent<T>();
        };

        funcs.addComponent = [](Entity& e){
            e.AddOrGetComponent<T>();
        };

        funcs.removeComponent = [](Entity& e){
            e.RemoveComponent<T>();
        };

        funcs.serialize = [name](YAML::Emitter& out, Entity& e){
            T& c = e.AddOrGetComponent<T>();

            ArchiveNode root(ArchiveNode::Type::Object, std::string(name), nullptr);
            c.Serialize(root);

            ArchiveNode::SaveSerializer(root, out);
        };

        funcs.deserialize = [](YAML::Node& in, Entity& e){
            T& c = e.AddOrGetComponent<T>();
            
            ArchiveNode root;
            c.Serialize(root);
            ArchiveNode::LoadSerializer(root, in);
        };

        funcs.onGui = [](Entity& e){
            T& c = e.AddOrGetComponent<T>();

            ArchiveNode root;
            c.Serialize(root);
            ArchiveNode::DrawArchive(root);
        };

        funcs.copy = [](entt::registry& dst, entt::registry& src){
            auto view = src.view<T>();
            for(auto e: view){
                T& c = view.template get<T>(e);
                dst.emplace_or_replace<T>(e, c);
            }
        };
        
        _coreComponentsSerializer[name] = funcs;
    }

    template<typename T>
    void RegisterComponent(const char* name){
        Assert(_componentsSerializer.find(name) == _componentsSerializer.end());

        SerializeFuncs funcs;

        funcs.hasComponent = [](Entity& e){
            return e.HasComponent<T>();
        };

        funcs.addComponent = [](Entity& e){
            e.AddOrGetComponent<T>();
        };

        funcs.removeComponent = [](Entity& e){
            e.RemoveComponent<T>();
        };

        funcs.serialize = [](Entity& e, ArchiveNode& s){
            auto& c = e.AddOrGetComponent<T>();
            c.Serialize(s);
        };

        funcs.copy = [](entt::registry& dst, entt::registry& src){
            auto view = src.view<T>();
            for(auto e: view){
                T& c = view.template get<T>(e);
                dst.emplace_or_replace<T>(e, c);
            }
        };
        
        _componentsSerializer[name] = funcs;
    }

    template<typename T>
    void RegisterScript(const char* name){
        Assert(_scriptsSerializer.find(name) == _scriptsSerializer.end());

        SerializeFuncs funcs;

        funcs.hasComponent = [](Entity& e){
            if(e.HasComponent<ScriptComponent>() == false) return false;

            auto& c = e.GetComponent<ScriptComponent>();
            return c.HasScript<T>();
        };

        funcs.serialize = [](Entity& e, ArchiveNode& s){
            auto& script = e.AddOrGetComponent<ScriptComponent>();
            auto* c = script.AddOrGetScript<T>();
            c->Serialize(s);
        };
        
        _scriptsSerializer[name] = funcs;
    }

    template<typename T>
    void RegisterSystem(const char* name){
        Assert(_addSystemFuncs.find(name) == _addSystemFuncs.end());

        _addSystemFuncs[name] = [&](Scene& e){
            e.AddSystem<T>();
        };
    }   

private:
    SceneManager(){}

    struct SerializeFuncs{
        std::function<bool(Entity&)> hasComponent;
        std::function<void(Entity&)> addComponent;
        std::function<void(Entity&)> removeComponent;
        std::function<void(Entity&,ArchiveNode&)> serialize;
        std::function<void(entt::registry& dst, entt::registry& src)> copy;
    };

    struct CoreComponent{
        std::function<bool(Entity&)> hasComponent;
        std::function<void(Entity&)> addComponent;
        std::function<void(Entity&)> removeComponent;
        std::function<void(YAML::Emitter&, Entity&)> serialize;
        std::function<void(YAML::Node&, Entity&)> deserialize;
        std::function<void(Entity&)> onGui;
        std::function<void(entt::registry& dst, entt::registry& src)> copy;
    };

    SceneState _sceneState;
    bool _inEditor;

    Scene* _activeScene;

    std::unordered_map<const char*, CoreComponent> _coreComponentsSerializer;
    std::unordered_map<const char*, SerializeFuncs> _componentsSerializer;
    std::unordered_map<const char*, SerializeFuncs> _scriptsSerializer;
    std::unordered_map<const char*, std::function<void(Scene&)> > _addSystemFuncs;
};

}