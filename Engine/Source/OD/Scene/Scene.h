#pragma once
#include "OD/Defines.h"
#include "OD/Core/Transform.h"
#include "OD/Core/Asset.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Serialization/SerializationFull.h"
#include "OD/Graphics/Camera.h"
#include "OD/Core/Module.h"
#include "OD/Core/Lua.h"
#include <unordered_map>
#include <string>
#include <functional>
#include <algorithm>
#include <entt/entt.hpp>
#include <taskflow/taskflow.hpp> 

namespace OD {

using Entity = entt::entity;
using Registry = entt::registry;
#define EntityNull entt::null
//#define EntityNull (Entity)UINT32_MAX

//struct Entity;
class System;
class Scene;
class Model;

enum OD_API_IMPORT Layers{
    //LayerNone = 0,
    Layer0 = 1 << 0,  // 0001
    Layer1 = 1 << 1,  // 0010
    Layer2 = 1 << 2,  // 0100
    Layer3 = 1 << 3,  // 1000
    Layer4 = 1 << 4,
    Layer5 = 1 << 5, 
    Layer6 = 1 << 6, 
    Layer7 = 1 << 7, 
    Layer8 = 1 << 8,
    Layer9 = 1 << 9,
    //LayerMax = 10
};

constexpr int LayerNone = 0;
constexpr int LayerMax = 10;
constexpr int AllLayers = Layer0 | Layer1 | Layer2 | Layer3 | Layer4 | Layer5 | Layer6 | Layer7 | Layer8 | Layer9;

inline int GetLayerIndex(Layers layer){
    assert(layer != LayerNone && layer < (1 << LayerMax)); // make sure it's a valid single-bit layer
    return static_cast<int>(std::log2(static_cast<int>(layer)));
}

inline Layers IndexToLayer(int index){
    return static_cast<Layers>(1 << index);
}

struct OD_API LayerMask{
    int mask = AllLayers;
    static int GetLayerByName(const std::string& name);

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDumpNVP(ar, mask);
    }
};

struct OD_API GlobalSceneData{
    std::vector<std::string> layerNames = {
        "Layer0",
        "Layer1",
        "Layer2",
        "Layer3",
        "Layer4",
        "Layer5",
        "Layer6",
        "Layer7",
        "Layer8",
        "Layer9",
    };
};

OD_API GlobalSceneData& GetGlobalSceneData();

struct OD_API SelfDisable{
    template <class Archive>
    void serialize(Archive & ar){}
};

class OD_API TransformComponent{
    friend struct Scene;
    friend class cereal::access;
public:
    //static constexpr auto in_place_delete = true;
    
    TransformComponent() = default;

    inline Vector3 Forward(){ return Rotation() * Vector3Forward; }
    inline Vector3 Back(){ return Rotation() * Vector3Back; }
    inline Vector3 Left(){ return Rotation() * Vector3Left; }
    inline Vector3 Right(){ return Rotation() * Vector3Right; }
    inline Vector3 Up(){ return Rotation() * Vector3Up; }
    inline Vector3 Down(){ return Rotation() * Vector3Down; }

    Matrix4 GlobalModelMatrix();
    inline Matrix4 GetLocalModelMatrix(){ return transform.GetLocalModelMatrix(); }

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

    Vector3 Scale();

    inline Vector3 LocalPosition(){ return transform.LocalPosition(); }
    inline void LocalPosition(Vector3 pos){ transform.LocalPosition(pos); }
    inline Vector3 LocalEulerAngles(){ return transform.LocalEulerAngles(); }
    inline void LocalEulerAngles(Vector3 euler){ transform.LocalEulerAngles(euler); }
    inline Quaternion LocalRotation(){ return transform.LocalRotation(); }
    inline void LocalRotation(Quaternion rot){ transform.LocalRotation(rot); }
    inline Vector3 LocalScale(){ return transform.LocalScale(); }
    inline void LocalScale(Vector3 scale){ transform.LocalScale(scale); }
    inline void SetLocalModelMatrix(Matrix4 matrix){ transform = Transform(matrix); }

    inline Entity Parent(){ return parent; }
    inline bool HasParent(){ /*return parent != entt::null;*/ return hasParent; }
    inline std::vector<Entity> Children(){ return children; }

    bool FindEntityInChildren(const std::string& name, Entity& out);

    template <class Archive>
    void serialize(Archive & ar);

    inline operator Transform() {
        return Transform(GlobalModelMatrix());
        //return Transform(Position(), Rotation(), LocalScale()); 
    }

    inline Transform ToTransform(){ 
        return Transform(GlobalModelMatrix()); 
        //return Transform(Position(), Rotation(), LocalScale()); 
    }

    static void CreateLuaBind(sol::state& lua);

private:
    Transform transform;
    std::vector<Entity> children;

    Entity parent = entt::null;
    bool hasParent = false;

    entt::registry* registry = nullptr;
};

enum class EntityType{
    Stand,
    PrefabRoot,
    PrefabChild
};

struct OD_API InfoComponent{
    friend struct Scene;

    std::string name = "Entity";
    std::string tag =  "";
    Layers layer = Layers::Layer0;
    bool enable = true;
    bool hidden = false;
    bool notSave = false;

    //inline EntityId Id() const { return id; }
    inline EntityType Type() const { return entityType; }

    template <class Archive>
    void serialize(Archive & ar);

    static void CreateLuaBind(sol::state& lua);

private:
    bool active;
    //EntityId id;
    EntityType entityType = EntityType::Stand;
    std::string prefabPath;
};

template<typename T>
auto _AddComponent(Scene* scene, Entity entity, const sol::table& comp, sol::this_state s);

template<typename T>
bool _HasComponent(Scene* scene, Entity entity);

template<typename T>
auto _GetComponent(Scene* scene, Entity entity, sol::this_state s);

template<typename T>
void _RemoveComponent(Scene* scene, Entity entity);

enum SystemType{//FIXME: Maybe Rename
    None = 0,
    Physics =  1 << 0, 
    Stand = 1 << 1, 
    Late = 1 << 2,
    Renderer = 1 << 3
};

class OD_API System{
public:
    System(Scene* inScene):scene(inScene){}
    virtual ~System(){}

    virtual int Type(){ return SystemType::Stand; } //FIXME: Maybe Rename
    virtual void PhysicsUpdate(){}
    virtual void Update(){}
    virtual void LateUpdate(){}
    virtual void Render(){}
    virtual void OnDrawGizmos(Camera& cam){} //FIXME: Maybe Add a SystemType::OnDrawGizmos
    virtual void OnDrawGizmosSelected(Camera& cam, Entity entity){} //FIXME: Maybe Add a SystemType::OnDrawGizmosSelected

    virtual bool ExecuteAlways(){ return false; }
    
    Scene* GetScene(){ return scene; }

protected:
    Scene* scene;
};

class OD_API Scene: public Asset {
public:
    friend struct EntityHandle;

    inline bool Running(){ return running; }

    Scene();
    Scene(Scene& other);
    ~Scene();

    Entity AddEntity(const std::string& name = "Entity");
    template<typename... T, typename Func> Entity AddEntityWith(std::string name, Func func);

    Entity DuplicateEntity(Entity e);

    void DestroyEntity(Entity entity);
    void DestroyEntityImmediate(Entity entity);
    bool IsChildOf(Entity parent, Entity child);
    void CleanParent(Entity e);
    void SetParent(Entity parent, Entity child);
    //inline void SetParent(Entity& parent, Entity& child){ SetParent(parent.Id(), child.Id()); }
    
    template<typename T, typename... Args> T& AddComponent(Entity entity, Args&&... args);
    template<typename T> T& AddComponent(Entity entity);
    template<typename T> void AddTagComponent(Entity entity);
    template<typename T> T& GetComponent(Entity entity);
    template<typename T> T* TryGetComponent(Entity entity);
    template<typename T> T* TryGetComponentInParent(Entity entity);
    template<typename T> T* TryGetComponentInChildren(Entity entity);
    template<typename T> bool HasComponent(Entity entity);
    template<typename T> T& AddOrGetComponent(Entity entity);
    template<typename T> void RemoveComponent(Entity entity);

    template<typename T> void AddComponentRecursive(Entity entity);
    template<typename T> void RemoveComponentRecursive(Entity entity);

    template<typename T> Entity TryFindEntityWithComponentInParent(Entity entity);

    template<typename T> inline static void RegisterMetaComponent();

    bool IsValid(Entity entity);

    Entity Instantiate(const Ref<Model> model, bool staticRenderer = false, int overrideLayer = LayerNone);
    Entity InstantiatePrefab(const char* prefabPath);
    
    Entity GetMainCamera();

    Entity FindEntityByName(const std::string& name);

    template<typename T> void AddSystem();
    template<typename T> void RemoveSystem();
    template<typename T> T* GetSystem();
    template<typename T> T* GetSystemDynamic();

    inline Registry& GetRegistry(){ return registry; }
    inline const auto& GetSystems(){ return systems; }
    inline const std::vector<System*>& GetStandSystems(){ return standSystems; }
    inline const std::vector<System*>& GetPhysicsSystems(){ return physicsSystems; }
    inline const std::vector<System*>& GetLateSystems(){ return lateSystems; }
    inline const std::vector<System*>& GetRendererSystems(){ return rendererSystems; }

    void Start();
    void Update();
    void Draw();

    void Save(const char* path, Entity root);
    void Load(const char* path);

    void UnpackPrefab(Entity entity, bool all = false);

    static void CreateLuaBind(sol::state& lua);

    inline auto& GetExecutor(){ return executor; }
    inline auto& GetTaskflow(){ return taskflow; }
private:
    void _AddEntityPrefab(entt::registry& registry, std::vector<entt::entity>& entities, std::vector<entt::entity>& allEntities, entt::entity root, std::string prefabPath, bool isRoot = false);
    void _Load(const char* path, entt::entity prefab);
    void _DestroyEntity(Entity entity, bool removeFromParent = false);
    void _LoadTransform(ODInputArchive& archive, std::unordered_map<entt::entity,entt::entity>& loadLookup, entt::registry& registry, std::string componentName, bool handleRootPrefab = false);
    void _Unpack(Entity e, bool all);
    Entity _DuplicateEntity(Entity e, bool isRoot);

    bool running = false;

    std::vector<System*> physicsSystems;
    std::vector<System*> standSystems;
    std::vector<System*> lateSystems;
    std::vector<System*> rendererSystems;
    std::unordered_map<Type, System*> systems;
    //std::unordered_map<Type, std::function<void(Scene&)>> systemsAdd;
    std::vector<std::function<void(Scene&)>> systemsAdd;
    std::vector<Entity> toDestroy;

    entt::registry registry;

    tf::Executor executor;
    tf::Taskflow taskflow;
};

struct OD_API EntityHandle{
public:
    friend struct Scene;

    EntityHandle() = default;
    EntityHandle(Entity _entity, Scene* _scene):entity(_entity), scene(_scene){}

    //template<typename T, typename... Args> T& AddComponent(Args&&... args);
    template<typename T> inline T& AddComponent(){ return scene->AddComponent<T>(entity); }
    template<typename T> inline T& GetComponent(){ return scene->GetComponent<T>(entity); }
    template<typename T> inline T* TryGetComponent(){ return scene->TryGetComponent<T>(entity); }
    template<typename T> inline T* TryGetComponentInParent(){ return scene->TryGetComponentInParent<T>(entity); }
    template<typename T> inline T* TryGetComponentInChildren(){ return scene->TryGetComponentInChildren<T>(entity); }
    template<typename T> inline bool HasComponent(){ return scene->HasComponent<T>(entity); }
    template<typename T> inline T& AddOrGetComponent(){ return scene->AddOrGetComponent<T>(entity); }
    template<typename T> inline void RemoveComponent(){ return scene->RemoveComponent<T>(entity); }

    inline bool IsValid(){ return scene != nullptr && scene->registry.valid(entity); }
    inline Entity GetEntity(){ return entity; }
    inline Scene* GetScene(){ return scene; }

    inline bool operator==(const EntityHandle& other) const { return entity == other.entity && scene == other.scene; }
    inline bool operator!=(const EntityHandle& other) const { return !(*this == other); }

    static void CreateLuaBind(sol::state& lua);

private:
    Entity entity = entt::null;
    Scene* scene = nullptr;
};

template<typename... Components>
struct GroupOfComps {
    static Entity Create(Registry& registry, Components&&... components){
        Entity entity = registry.create();
        (EmplaceComponent<Components>(registry, entity, std::forward<Components>(components)), ...);
        return entity;
    }

    static Entity Create(Registry& registry){
        Entity entity = registry.create();
        (EmplaceComponent<Components>(registry, entity, Components{}), ...);
        return entity;
    }

    static Entity Create(Scene& scene, const std::string& name, Components&&... components){
        Entity entity = scene.AddEntity(name);
        (emplaceComponent<Components>(scene.GetRegistry(), entity, std::forward<Components>(components)), ...);
        return entity;
    }

    static Entity Create(Scene& scene, const std::string& name){
        Entity entity = scene.AddEntity(name);
        (EmplaceComponent<Components>(scene.GetRegistry(), entity, Components{}), ...);
        return entity;
    }

    template<typename Component>
    static void EmplaceComponent(Registry& registry, Entity entity, Component&& component){
        if constexpr (std::is_same_v<std::decay_t<Component>, TransformComponent>){
            std::cout << "Special handling for TransformComponent\n";
            if(!registry.any_of<TransformComponent>(entity)) registry.emplace<TransformComponent>(entity, std::forward<Component>(component));
        } else if constexpr (std::is_same_v<std::decay_t<Component>, InfoComponent>) { 
            std::cout << "Special handling for InfoComponent\n";
            if(!registry.any_of<InfoComponent>(entity)) registry.emplace<InfoComponent>(entity, std::forward<Component>(component));
        }else {
            registry.emplace<Component>(entity, std::forward<Component>(component));
        }
    }

    static auto GetView(Registry& registry){
        return registry.view<Components...>();
    }
};

template<typename T>
class UndoValueComponentCommand : public IUndoCommand {
public:
    UndoValueComponentCommand(Scene* scene, Entity entity, const T& oldValue, const T& newValue)
        : scene(scene), entity(entity), oldValue(oldValue), newValue(newValue) {}

    void Undo() override {
        if(scene->IsValid(entity) && scene->HasComponent<T>(entity))
            scene->GetComponent<T>(entity) = oldValue;
    }

    void Redo() override {
        if(scene->IsValid(entity) && scene->HasComponent<T>(entity))
            scene->GetComponent<T>(entity) = newValue;
    }

private:
    Scene* scene;
    Entity entity;
    T oldValue;
    T newValue;
};

template<typename T>
class UndoValueComponentBatchCommand : public IUndoCommand {
public:
    UndoValueComponentBatchCommand(
        Scene* scene,
        const std::vector<Entity>& entities,
        const std::vector<T>& oldValues,
        const std::vector<T>& newValues)
        :scene(scene), entities(entities), oldValues(oldValues), newValues(newValues){
        assert(entities.size() == oldValues.size());
        assert(entities.size() == newValues.size());
    }

    void Undo() override {
        for (size_t i = 0; i < entities.size(); ++i) {
            Entity entity = entities[i];
            if (scene->IsValid(entity) && scene->HasComponent<T>(entity)) {
                scene->GetComponent<T>(entity) = oldValues[i];
            }
        }
    }

    void Redo() override {
        for (size_t i = 0; i < entities.size(); ++i) {
            Entity entity = entities[i];
            if (scene->IsValid(entity) && scene->HasComponent<T>(entity)) {
                scene->GetComponent<T>(entity) = newValues[i];
            }
        }
    }

private:
    Scene* scene;
    std::vector<Entity> entities;
    std::vector<T> oldValues;
    std::vector<T> newValues;
};

template<typename T>
class UndoAddComponent : public IUndoCommand {
public:
    UndoAddComponent(Scene* scene, Entity entity)
        : scene(scene), entity(entity) {}

    void Undo() override {
        if (scene->IsValid(entity) && scene->HasComponent<T>(entity))
            scene->RemoveComponent<T>(entity);
    }

    void Redo() override {
        if (scene->IsValid(entity) && !scene->HasComponent<T>(entity))
            scene->AddComponent<T>(entity);
    }

private:
    Scene* scene;
    Entity entity;
};

template<typename T>
class UndoRemoveComponent : public IUndoCommand {
public:
    UndoRemoveComponent(Scene* scene, Entity entity)
        : scene(scene), entity(entity) {
        if (scene->IsValid(entity) && scene->HasComponent<T>(entity))
            backup = scene->GetComponent<T>(entity);
    }

    void Undo() override {
        if (scene->IsValid(entity) && !scene->HasComponent<T>(entity))
            scene->AddComponent<T>(entity, backup);
    }

    void Redo() override {
        if (scene->IsValid(entity) && scene->HasComponent<T>(entity))
            scene->RemoveComponent<T>(entity);
    }

private:
    Scene* scene;
    Entity entity;
    T backup;
};


}

#include "Scene.inl"