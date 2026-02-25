#pragma once
#include "OD/Core/Asset.h"
#include "OD/Core/Transform.h"
//#include "OD/Serialization/Serialization.h"
#include "OD/Serialization/SerializationFull.h" //TODO: Remove this to optimization
#include "OD/Graphics/Camera.h"
#include "OD/Core/Lua.h"
#include <unordered_map>
#include <string>
#include <functional>
#include <algorithm>
#include <entt/entt.hpp>
#include <taskflow/taskflow.hpp>

namespace sol{ class state; }

namespace OD {

using Entity = entt::entity;
using Registry = entt::registry;
#define EntityNull entt::null
//#define EntityNull (Entity)UINT32_MAX

//struct Entity;
class System;
class Scene;
class Model;
class Prefab;

enum OD_API_IMPORT Layers: int{
    Layer0 = 0,
    Layer1,
    Layer2,
    Layer3, 
    Layer4,
    Layer5, 
    Layer6, 
    Layer7, 
    Layer8,
    Layer9,
    LayerCount // always last
};

constexpr int LayerNone = -1;
/*constexpr int LayerMax = 10;
constexpr int AllLayers = Layer0 | Layer1 | Layer2 | Layer3 | Layer4 | Layer5 | Layer6 | Layer7 | Layer8 | Layer9;*/

constexpr uint32_t AllLayersMask = (1u << LayerCount) - 1;
constexpr uint32_t NoneLayersMask = 0;

inline constexpr uint32_t LayerToMask(int layerIndex) {
    return (1u << layerIndex);
}

struct OD_API LayerMask{
    //int mask = AllLayers;
    
    static int GetLayerByName(const std::string& name);

    uint32_t mask = AllLayersMask;

    static inline LayerMask FromLayer(int layer) { return { LayerToMask(layer) }; }
    static inline LayerMask FromLayers(std::initializer_list<int> layers) {
        uint32_t m = 0;
        for (int l : layers) m |= LayerToMask(l);
        return { m };
    }

    bool Contains(int layer) const { return (mask & LayerToMask(layer)) != 0; }

    template <class Archive>
    void serialize(Archive & ar){
        ArchiveDumpNVP(ar, mask);
    }
};

struct OD_API GlobalSceneData{
    std::array<std::string, LayerCount> layerNames = {
        "Default", //"Layer0",
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

    template<class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, layerNames);
    }

    void OnImGuiRender();
};

OD_API GlobalSceneData& GetGlobalSceneData();

//Fix DLL Problem
#define EmptyComponentBody char _dummy = 0; \
    template <class Archive> void serialize(Archive & ar){} 

struct OD_API SelfDisable{ EmptyComponentBody };
struct OD_API SkipDraw{ EmptyComponentBody };
struct OD_API DontSave{ EmptyComponentBody };
struct OD_API HideInEditor{ EmptyComponentBody };

#define ExperimentalTransformOptimzation

class OD_API TransformComponent{
    friend class Scene;
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

    const Matrix4& GlobalModelMatrixReadSafe() const;
    const Matrix4 GlobalModelMatrix();

    Matrix4 GetLocalModelMatrix();

    //Transforms a direction from world space to local space. The opposite of Transform.TransformDirection.
    Vector3 InverseTransformDirection(Vector3 dir); 
    
    //Transforms direction from local space to world space.
    Vector3 TransformDirection(Vector3 dir); 

    //Transforms position from world space to local space.
    Vector3 InverseTransformPoint(Vector3 point); 

    //Transforms position from local space to world space.
    Vector3 TransformPoint(Vector3 point);  

    const Vector3& PositionReadSafe() const;

    Vector3 Position();
    void Position(Vector3 position);

    Quaternion Rotation();
    void Rotation(Quaternion rotation);

    Vector3 Scale();

    Vector3 LocalPosition();
    void LocalPosition(Vector3 pos);

    Vector3 LocalEulerAngles();
    void LocalEulerAngles(Vector3 euler);
    
    Quaternion LocalRotation();
    void LocalRotation(Quaternion rot);
    
    Vector3 LocalScale();
    void LocalScale(Vector3 scale);
    
    void SetLocalModelMatrix(Matrix4 matrix);

    inline Entity Parent(){ return parent; }
    inline bool HasParent(){ /*return parent != entt::null;*/ return hasParent; }
    inline std::vector<Entity> Children(){ return children; }

    bool FindEntityInChildren(const std::string& name, Entity& out);

    template <class Archive>
    void serialize(Archive & ar);

    operator Transform();
    Transform ToTransform();

    static void CreateLuaBind(sol::state& lua);
    
    void SetGlobalAsDirty();

    void UpdateGlobalTransformCacheIfNeeded(bool updateChild = true);

    static void UpdateAllTransformMatrix(class Scene& scene);
    
    template<typename... Components, typename Func>
    static void ForEachWithTransformTaskflow(Scene& scene, Func&& func);

    #ifdef ExperimentalTransformOptimzation
    bool isCollection = false;
    #endif

private:
    Transform localTransform;
    #ifdef ExperimentalTransformOptimzation
        #ifdef TransformLessDataOptimzation
        Transform globalTransform;
        Matrix4 localModelMatrix = Matrix4Identity;
        Matrix4 globalModelMatrix = Matrix4Identity;
        bool globalIsDirty = true;
        #else
        Transform globalTransform;
        bool globalIsDirty = true;
        #endif
    #endif

    #ifdef TransformLessDataOptimzation
    Vector3 localEulerAngles = Vector3Zero;
    bool localEulerAnglesIsDirt = true;
    #endif

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
    inline const std::string& PrefabPath() const { return prefabPath; }

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
    Stand = 1 << 1, 
    Animation = 1 << 2,
    PrePhysics = 1 << 3,
    FixedPhysics = 1 << 4,
    PostPhysics =  1 << 5, 
    Late = 1 << 6,
    Renderer = 1 << 7
};

class OD_API System{
public:
    virtual ~System(){}

    virtual void OnInit(Scene& scene){} // Called a System Constructor
    virtual void OnEnd(Scene& scene){} // Called a System Destructor

    virtual void OnStart(Scene& scene){} // Call On Start running scene
    virtual void OnStop(Scene& scene){} // Call On Stop running scene

    virtual int Type(){ return SystemType::Stand; } //FIXME: Maybe Rename
    virtual void Update(Scene& scene){}
    virtual void AnimationUpdate(Scene& scene){}
    virtual void PrePhysicsUpdate(Scene& scene){}
    virtual void FixedPhysicsUpdate(Scene& scene){}
    virtual void PostPhysicsUpdate(Scene& scene){}
    virtual void LateUpdate(Scene& scene){}
    virtual void Render(Scene& scene){}
    virtual void OnDrawGizmos(Scene& scene, Camera& cam){} //FIXME: Maybe Add a SystemType::OnDrawGizmos
    virtual void OnDrawGizmosSelected(Scene& scene, Camera& cam, Entity entity){} //FIXME: Maybe Add a SystemType::OnDrawGizmosSelected
    
    virtual int ExecutionSortPriority(SystemType type){ return 1; }; 
    virtual bool ExecuteAlways(){ return false; }
    
    inline const std::string& Name(){ return name; }

protected:
    std::string name;
};

class OD_API Scene: public Asset {
public:
    friend struct EntityHandle;
    friend struct Prefab;

    inline bool Running(){ return running; }

    Scene(bool withoutDefaultSystems = false);
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
    //TODO: To Deprecate later
    //INFO: this can cause alot of asset load, becose new USE_WEAK_PTR Update, if the prefab entity is constant create and destory
    Entity InstantiatePrefab(const char* prefabPath);
    Entity InstantiatePrefab(const Prefab& prefab);
    
    Entity GetMainCamera();

    Entity FindEntityByName(const std::string& name);
    Entity FindEntityByNameInChildren(Entity parent, const std::string& name);

    template<typename T> void AddSystem();
    template<typename T> void RemoveSystem();
    template<typename T> T* GetSystem();
    template<typename T> T* GetSystemDynamic();

    inline Registry& GetRegistry(){ return registry; }
    inline const auto& GetSystems(){ return systems; }
    inline const std::vector<System*>& GetStandSystems(){ return standSystems; }
    inline const std::vector<System*>& GetAnimationSystems(){ return animationSystems; }
    inline const std::vector<System*>& GetPrePhysicsSystems(){ return prePhysicsSystems; }
    inline const std::vector<System*>& GetFixedPhysicsSystems(){ return fixedPhysicsSystems; }
    inline const std::vector<System*>& GetPostPhysicsSystems(){ return postPhysicsSystems; }
    inline const std::vector<System*>& GetLateSystems(){ return lateSystems; }
    inline const std::vector<System*>& GetRendererSystems(){ return rendererSystems; }

    inline float FixedUpdateAccumulator(){ return fixedUpdateAccumulator; }

    void Start();
    void Stop();

    void Update();
    void Draw();

    void Save(const char* path, Entity root);
    void Load(const char* path);

    void UnpackPrefab(Entity entity, bool all = false);

    static void CreateLuaBind(sol::state& lua);

    void RunAllTaskAndSync();
    //tf::Executor& GetExecutor();
    tf::Taskflow& GetTaskflow();
    //inline auto& GetExecutor(){ return executor; }
    //inline auto& GetTaskflow(){ return taskflow; }
private:
    bool isPrefab = false; //INFO: this is temporary, until refactoty Prefab Load
    void _AddEntityPrefab(entt::registry& registry, std::vector<entt::entity>& entities, std::vector<entt::entity>& allEntities, entt::entity root, std::string prefabPath, bool isRoot = false);
    void _Load(const char* path, entt::entity prefab);
    void _DestroyEntity(Entity entity, bool removeFromParent = false);
    void _LoadTransform(ODInputArchive& archive, std::unordered_map<entt::entity,entt::entity>& loadLookup, entt::registry& registry, std::string componentName, bool handleRootPrefab = false);
    void _Unpack(Entity e, bool all, bool unpackRoot);
    Entity _DuplicateEntity(Entity e, bool isRoot);
    Entity _DuplicateEntity(Entity e, bool isRoot, Scene& other);

    bool running = false;

    std::vector<System*> standSystems;
    std::vector<System*> animationSystems;
    
    std::vector<System*> prePhysicsSystems;
    std::vector<System*> fixedPhysicsSystems;
    std::vector<System*> postPhysicsSystems;

    std::vector<System*> lateSystems;
    std::vector<System*> rendererSystems;
    std::unordered_map<Type, System*> systems;
    //std::unordered_map<Type, std::function<void(Scene&)>> systemsAdd;
    std::vector<std::function<void(Scene&)>> systemsAdd;
    std::vector<Entity> toDestroy;

    entt::registry registry;

    //tf::Executor executor;
    tf::Taskflow taskflow;

    bool transIsDirty = true;
    float fixedUpdateAccumulator = 0;
};

}

#include "Scene.inl"