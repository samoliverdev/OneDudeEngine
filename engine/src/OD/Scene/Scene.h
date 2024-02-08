#pragma once

#include "OD/Defines.h"
#include "OD/Core/Transform.h"
#include "OD/Core/AssetManager.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Serialization/CerealImGui.h"
#include "OD/Core/ImGui.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Graphics/Camera.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Core/Module.h"
#include <unordered_map>
#include <string>
#include <entt/entt.hpp>
#include <functional>
#include <algorithm>

namespace OD {

using EntityId = entt::entity;

struct Entity;
struct System;
struct Scene;

class TransformComponent: public Transform{
    friend struct Scene;
    friend class cereal::access;

public:
    inline Vector3 Forward(){ return Rotation() * Vector3Forward; }
    inline Vector3 Back(){ return Rotation() * Vector3Back; }
    inline Vector3 Left(){ return Rotation() * Vector3Left; }
    inline Vector3 Right(){ return Rotation() * Vector3Right; }
    inline Vector3 Up(){ return Rotation() * Vector3Up; }
    inline Vector3 Down(){ return Rotation() * Vector3Down; }

    Matrix4 GlobalModelMatrix();
    
    //Transforms a direction from world space to local space. The opposite of Transform.TransformDirection.
    Vector3 InverseTransformDirection(Vector3 dir); 
    
    //Transforms direction from local space to world space.
    Vector3 TransformDirection(Vector3 dir); 

    //Transforms position from world space to local space.
    Vector3 InverseTransformPoint(Vector3 point); 

    //Transforms position from local space to world space.
    Vector3 TransformPoint(Vector3 point);  

    Vector3 Position();
    void Position(Vector3 position);

    Quaternion Rotation();
    void Rotation(Quaternion rotation);

    inline EntityId Parent(){ return parent; }
    inline bool HasParent(){ return hasParent; }
    inline std::vector<EntityId> Children(){ return children; }

    template <class Archive>
    void serialize(Archive & ar);

    inline operator Transform(){ return Transform(Position(), Rotation(), LocalScale()); } 

private:
    std::vector<EntityId> children;

    EntityId parent;
    bool hasParent;

    entt::registry* registry;
};

struct InfoComponent{
    friend struct Scene;

    std::string name = "Entity";
    std::string tag =  "";

    inline EntityId Id() const { return id; }

    template <class Archive>
    void serialize(Archive & ar);

private:
    bool active;
    EntityId id;
};

struct Entity{
    friend struct Scene;
public:

    Entity():isValid(false){}
    Entity(EntityId _id, Scene* _scene):id(_id), scene(_scene), isValid(true){}

    template<typename T> T& AddComponent();
    template<typename T> T& GetComponent();
    template<typename T> bool HasComponent();
    template<typename T> T& AddOrGetComponent();
    template<typename T> void RemoveComponent();

    inline bool IsValid(){ return isValid; }
    inline EntityId Id(){ return id; }
    inline Scene* GetScene(){ return scene; }

    inline bool operator==(const Entity& other) const { return id == other.id && scene == other.scene; }
    inline bool operator!=(const Entity& other) const { return !(*this == other); }

private:
    EntityId id;
    Scene* scene;
    bool isValid = false;
};

enum class SystemType{
    Stand, Renderer, Physics
};

class System{
public:
    System(Scene* inScene):scene(inScene){}

    virtual System* Clone(Scene* inScene) const = 0;

    virtual SystemType Type(){ return SystemType::Stand; }
    virtual void Update(){}
    
    Scene* GetScene(){ return scene; }

protected:
    Scene* scene;
};

class Scene: public Asset {
public:
    friend struct Entity;

    inline bool Running(){ return running; }

    Scene();
    Scene(Scene& other);
    ~Scene();

    static Scene* Copy(Scene* other);

    Entity AddEntity(std::string name = "Entity");
    void DestroyEntity(EntityId entity);
    bool IsChildOf(EntityId parent, EntityId child);
    void CleanParent(EntityId e);
    void SetParent(EntityId parent, EntityId child);
    
    Entity GetMainCamera2();
    Camera& GetMainCamera();

    template<typename T> void AddSystem();
    template<typename T> void RemoveSystem();
    template<typename T> T* GetSystem();
    template<typename T> T* GetSystemDynamic();

    inline entt::registry& GetRegistry(){ return registry; }

    void Start();
    void Update();
    void Draw();

    void Save(const char* path);
    void Load(const char* path);

private:
    void _DestroyEntity(EntityId entity);
    
    bool running = false;

    Camera mainCamera;  

    std::vector<System*> standSystems;
    std::vector<System*> rendererSystems;
    std::vector<System*> physicsSystems;
    std::unordered_map<Type, System*> systems;
    std::vector<EntityId> toDestroy;

    entt::registry registry;
};

class SceneManager: public Module{
public:
    friend class Editor;
    friend class SceneHierarchyPanel;
    friend class InspectorPanel;
    friend class Scene;
    friend struct ScriptComponent;

    enum class SceneState {Playing, Paused, Editor};

    static SceneManager& Get();
    
    void OnInit() override;
    void OnExit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;

    SceneState GetSceneState();
    inline bool InEditor();
    Scene* GetActiveScene();
    void SetActiveScene(Scene* s);
    Scene* NewScene();

    template<typename T> void RegisterCoreComponent(const char* name);
    template<typename T> void RegisterCoreComponentSimple(const char* name);
    template<typename T> void RegisterComponent(const char* name);
    template<typename T> void RegisterScript(const char* name);
    template<typename T> void RegisterSystem(const char* name);   

private:
    SceneManager(){}

    struct SerializeFuncs{
        std::function<bool(Entity&)> hasComponent;
        std::function<void(Entity&)> addComponent;
        std::function<void(Entity&)> removeComponent;
        std::function<void(Entity&)> onGui;
        std::function<void(entt::registry& dst, entt::registry& src)> copy;
        std::function<void(ODOutputArchive& out, entt::registry& registry, std::string name)> snapshotOut;
        std::function<void(ODInputArchive& out, entt::registry& registry, std::string name)> snapshotIn;
    };

    struct CoreComponent{
        std::function<bool(Entity&)> hasComponent;
        std::function<void(Entity&)> addComponent;
        std::function<void(Entity&)> removeComponent;
        std::function<void(Entity&)> onGui;
        std::function<void(entt::registry& dst, entt::registry& src)> copy;
        std::function<void(ODOutputArchive& out, entt::registry& registry, std::string name)> snapshotOut;
        std::function<void(ODInputArchive& out, entt::registry& registry, std::string name)> snapshotIn;
    };

    SceneState sceneState;
    bool inEditor;

    Scene* activeScene;

    std::unordered_map<const char*, CoreComponent> coreComponentsSerializer;
    std::unordered_map<const char*, SerializeFuncs> componentsSerializer;
    std::unordered_map<const char*, SerializeFuncs> scriptsSerializer;
    std::unordered_map<const char*, std::function<void(Scene&)> > addSystemFuncs;
};

template<typename T>
struct CoreComponentTypeRegistrator{
    CoreComponentTypeRegistrator(const char* name){
        SceneManager::Get().RegisterCoreComponent<T>(name);
    }
};

template<typename T>
void _SaveComponent(ODOutputArchive& archive, entt::registry& registry, std::string componentName){
    auto view = registry.view<T>();
    std::vector<T> components;
    std::vector<entt::entity> componentsEntities;
    for(auto e: view){
        components.push_back(view.get<T>(e));
        componentsEntities.push_back(e);
    }
    archive(cereal::make_nvp(componentName + "s", components));
    archive(cereal::make_nvp(componentName + "Entities", componentsEntities));
}

template<typename T>
void _LoadComponent(ODInputArchive& archive, entt::registry& registry, std::string componentName){
    std::vector<T> components;
    std::vector<entt::entity> componentsEntities;
    archive(cereal::make_nvp(componentName + "s", components));
    archive(cereal::make_nvp(componentName + "Entities", componentsEntities));
    for(int i = 0; i < components.size(); i++){
        registry.emplace<T>(componentsEntities[i], components[i]);
    }
}

#define OD_REGISTER_CORE_COMPONENT_TYPE(componentName) static inline const CoreComponentTypeRegistrator<componentName> componentNameReg{#componentName} 

}

#include "Scene.inl"