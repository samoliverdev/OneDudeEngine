#pragma once
#include "OD/Defines.h"
#include "Scene.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"
#include <stdlib.h>

namespace OD{

struct OD_API Script{
    friend struct ScriptComponent;

    virtual void OnStart(){}
    virtual void OnDestroy(){}
    virtual void OnUpdate(){}
    //virtual void OnDraw(){}

    inline Entity& GetEntity(){ return entity; }
    
private:
    Entity entity;
    bool hasStarted = false;
};

//TODO: Make Serializable
struct OD_API ScriptComponent{
    friend struct ScriptSystem;
    friend struct Scene;

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive & ar){}

    static void OnGui(Entity& e);

    ScriptComponent() = default;
    ScriptComponent(const ScriptComponent& s);

    template<typename T>
    T* AddScript(){
        static_assert(std::is_base_of<OD::Script, T>::value);

        T* c = new T();

        ScriptHolder holder = {
            c,
            [](ScriptHolder& s){ 
                T* r = new T();
                if(s.instance != nullptr) *r = *static_cast<T*>(s.instance);
                return static_cast<Script*>(r);  
            }
        };
        instances[GetType<T>()] = holder;
        
        return c;
    }

    template <typename T>
    T* GetScript(){
        static_assert(std::is_base_of<OD::Script, T>::value);
        return static_cast<T*>(instances[GetType<T>()].instance);
    }

    template<typename T>
    bool HasScript(){
        return instances.count(GetType<T>());
    }

    template<typename T>
    T* AddOrGetScript(){
        if(HasScript<T>() == false) return AddScript<T>();
        return GetScript<T>();
    }

    void RemoveAllScripts();

private:
    int version = 10;

    struct ScriptHolder{
        Script* instance;
        Script* (*InstantiateScript)(ScriptHolder&);
    };

    std::unordered_map<Type, ScriptHolder> instances;

    void _Update(Entity e);
};

struct OD_API ScriptSystem: public System{
    ScriptSystem(Scene* scene);
    ~ScriptSystem();
    System* Clone(Scene* inScene) const override { return new ScriptSystem(inScene); }

    virtual void Update() override;
private:
    static void OnDestroyScript(entt::registry & r, entt::entity e);
};

}