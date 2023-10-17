#pragma once

#include "Scene.h"
#include "OD/Serialization/Archive.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"
#include <vector>
#include <stdlib.h>

namespace OD{

struct Script{
    friend struct ScriptComponent;

    virtual void Serialize(ArchiveNode& s){}

    virtual void OnStart(){}
    virtual void OnDestroy(){}
    virtual void OnUpdate(){}
    //virtual void OnDraw(){}

    inline Entity& entity(){ return _entity; }
    
private:
    Entity _entity;
    bool _hasStarted = false;
};

struct ScriptComponent{
    friend struct ScriptSystem;
    friend struct Scene;

    static void Serialize(YAML::Emitter& out, Entity& e);
    static void Deserialize(YAML::Node& in, Entity& e);
    static void OnGui(Entity& e);

    ScriptComponent() = default;
    ScriptComponent(const ScriptComponent& s){
        LogInfo("Copping");
        for(auto i: s._instances){
            ScriptHolder holder = {i.second.InstantiateScript(i.second), i.second.InstantiateScript};
            _instances[i.first] = holder;
        }
    }

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
        _instances[GetType<T>()] = holder;
        
        return c;
    }

    template <typename T>
    T* GetScript(){
        static_assert(std::is_base_of<OD::Script, T>::value);
        return static_cast<T*>(_instances[GetType<T>()].instance);
    }

    template<typename T>
    bool HasScript(){
        return _instances.count(GetType<T>());
    }

    template<typename T>
    T* AddOrGetScript(){
        if(HasScript<T>() == false) return AddScript<T>();
        return GetScript<T>();
    }

    void RemoveAllScripts(){
        /*for(auto it = _instances.begin(); it != _instances.end();) {
            Assert(it->second.instance != nullptr);
            it->second.instance->OnDestroy();
            delete it->second.instance;
            it = _instances.erase(it);
        }*/

        for(auto i: _instances){
            delete i.second.instance;
        }
        _instances.clear();
    }

private:
    struct ScriptHolder{
        Script* instance;
        Script* (*InstantiateScript)(ScriptHolder&);
    };

    std::unordered_map<Type, ScriptHolder> _instances;

    void _Update(Entity e){
        for(auto i: _instances){
            if(i.second.instance == nullptr){
                i.second.instance = i.second.InstantiateScript(i.second);
            }

            i.second.instance->_entity = e;
            Assert(i.second.instance->_entity.IsValid() == true);

            if(i.second.instance->_hasStarted == false){
                i.second.instance->OnStart();
                i.second.instance->_hasStarted = true;
            }
            i.second.instance->OnUpdate();
        }
    }
};

struct ScriptSystem: public System{
    virtual void Init(Scene* scene) override{
        _scene = scene;
        _scene->GetRegistry().on_destroy<ScriptComponent>().connect<&OnDestroyScript>();
    }

    virtual void Update() override{
        auto view = scene()->GetRegistry().view<ScriptComponent>();

        for(auto entity: view){
            auto& c = view.get<ScriptComponent>(entity);
            c._Update(Entity(entity, scene()));
        }
    }
private:
    inline static void OnDestroyScript(entt::registry & r, entt::entity e){
        ScriptComponent& s = r.get<ScriptComponent>(e);
        s.RemoveAllScripts();
        //LogInfo("On Destry ScriptComponent ___");
    }
};

}