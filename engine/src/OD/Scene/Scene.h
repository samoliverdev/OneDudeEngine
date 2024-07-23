#pragma once

#include "OD/Defines.h"
#include "OD/Core/Transform.h"
#include "OD/Core/Asset.h"
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

class OD_API TransformComponent{
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

    inline EntityId Parent(){ return parent; }
    inline bool HasParent(){ /*return parent != entt::null;*/ return hasParent; }
    inline std::vector<EntityId> Children(){ return children; }

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

private:
    Transform transform;
    std::vector<EntityId> children;

    EntityId parent = entt::null;
    bool hasParent;

    entt::registry* registry;
};

struct OD_API InfoComponent{
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

struct OD_API Entity{
    friend struct Scene;
public:

    Entity():isValid(false){}
    Entity(EntityId _id, Scene* _scene):id(_id), scene(_scene), isValid(true){}

    template<typename T> T& AddComponent();
    template<typename T> T& GetComponent();
    template<typename T> T* TryGetComponent();
    template<typename T> bool HasComponent();
    template<typename T> T& AddOrGetComponent();
    template<typename T> void RemoveComponent();

    bool IsValid();
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

class OD_API System{
public:
    System(Scene* inScene):scene(inScene){}
    virtual ~System(){}

    virtual System* Clone(Scene* inScene) const = 0;

    virtual SystemType Type(){ return SystemType::Stand; }
    virtual void Update(){}
    virtual void OnRender(){}
    virtual void OnDrawGizmos(){}
    
    Scene* GetScene(){ return scene; }

protected:
    Scene* scene;
};

class OD_API Scene: public Asset {
public:
    friend struct Entity;

    inline bool Running(){ return running; }

    Scene();
    Scene(Scene& other);
    ~Scene();

    static Scene* Copy(Scene* other);

    Entity AddEntity(std::string name = "Entity");
    template<typename... T, typename Func> Entity AddEntityWith(std::string name, Func func);

    void DestroyEntity(EntityId entity);
    bool IsChildOf(EntityId parent, EntityId child);
    void CleanParent(EntityId e);
    void SetParent(EntityId parent, EntityId child);

    Entity Instantiate(const Ref<Model> model);
    
    Entity GetMainCamera2();
    Camera& GetMainCamera();

    template<typename T> void AddSystem();
    template<typename T> void RemoveSystem();
    template<typename T> T* GetSystem();
    template<typename T> T* GetSystemDynamic();

    inline entt::registry& GetRegistry(){ return registry; }
    inline const std::vector<System*>& GetStandSystems(){ return standSystems; }

    void Start();
    void Update();
    void Draw();

    void Save(const char* path);
    void Load(const char* path);

private:
    void _DestroyEntity(EntityId entity);
    void _LoadTransform(ODInputArchive& archive, entt::registry& registry, std::string componentName);
    
    bool running = false;

    Camera mainCamera;  

    std::vector<System*> standSystems;
    std::vector<System*> rendererSystems;
    std::vector<System*> physicsSystems;
    std::unordered_map<Type, System*> systems;
    std::vector<EntityId> toDestroy;

    entt::registry registry;
};

}

#include "Scene.inl"