#pragma once
#include "Scene.h"
//#include <typeinfo>

namespace OD{

struct OD_API Script{
    friend struct ScriptComponent;

    virtual Type GetTypeId() const = 0;

    virtual ~Script(){}
    virtual void OnStart(){}
    virtual void OnDestroy(){}
    virtual void OnUpdate(){}
    virtual void OnLateUpdate(){}
    //virtual void OnDraw(){}

    virtual void OnParallelUpdate(){} //INFO: Experimental

    inline Entity& GetEntity(){ return entity; }
    inline Scene* GetScene(){ return scene; }
    
protected:
    Entity entity;
    Scene* scene;

private:
    bool hasStarted = false;
};

template<typename T>
class ScriptBase: public Script{
public:
    Type GetTypeId() const override { return GetType<T>(); }
};

//TODO: Make Serializable
struct OD_API ScriptComponent{
    friend struct ScriptSystem;
    friend struct Scene;

    static void OnGui(Entity& e, Scene& scene);

    ScriptComponent() = default;
    ScriptComponent(const ScriptComponent& s);

    template <typename T>
    T* GetScript(){
        static_assert(std::is_base_of<OD::Script, T>::value);
        return static_cast<T*>(instances[GetType<T>()].instance);
    }

    template <typename T>
    T* GetScriptDynamic(){
        for(auto& i: instances){
            T* out = dynamic_cast<T*>(i.second.instance);
            if(out != nullptr) return out;
        }

        return nullptr;
    }

    template<typename T>
    bool HasScript(){
        return instances.count(GetType<T>());
    }

    template<typename T>
    T* AddScript(){
        Assert(HasScript<T>() == false);
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

    template<typename T>
    T* AddOrGetScript(){
        if(HasScript<T>() == false) return AddScript<T>();
        return GetScript<T>();
    }

    template<typename T>
    void RemoveScript(){
        if(HasScript<T>() == false) return;

        delete instances[GetType<T>()].instance;
        instances.erase(GetType<T>());
    }

    void RemoveAllScripts();

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar);

private:
    //Scene* scene = nullptr;
    //Entity entity = EntityNull;
    int version = 10;

    struct ScriptHolder{
        Script* instance;
        Script* (*InstantiateScript)(ScriptHolder&);
    };

    std::unordered_map<Type, ScriptHolder> instances;

    void _Update(Entity e, Scene& scene, bool isLate = false);
    void _ParallelUpdate(Entity e, Scene& scene, bool isLate = false);
};

struct OD_API ScriptSystem: public System{
    ScriptSystem(){ name = "ScriptSystem"; }
    void OnInit(Scene& scene);
    void OnEnd(Scene& scene);
    //System* Clone(Scene* inScene) const override { return new ScriptSystem(inScene); }

    virtual inline int Type() override { return SystemType::Stand | SystemType::Late; }
    virtual void Update(Scene& scene) override;
    virtual void LateUpdate(Scene& scene) override;
private:
    static void OnDestroyScript(entt::registry & r, entt::entity e);
};

void ScriptModuleInit();

}

#include "Scripts.inl"