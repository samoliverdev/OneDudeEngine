#pragma once

#include "Scene.h"
#include "OD/Serialization/Archive.h"
#include "OD/Serialization/Serialization.h"
#include "OD/Core/ImGui.h"
#include <vector>

namespace OD{

struct Script{
    friend struct ScriptComponent;

    virtual void Serialize(Archive& s){}

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
    //static void Serialize(YAML::Emitter& out, Entity& e);
    //static void Deserialize(YAML::Node& in, Entity& e);
    //static void OnGui(Entity& e);
    
    friend struct ScriptSystem;
    friend struct Scene;

    template<typename T>
    T* AddScript(){
        static_assert(std::is_base_of<OD::Script, T>::value);
        T* c = new T();
        _instances[GetType<T>()] = c;
        return c;
    }

    template <typename T>
    T* GetScript(){
        static_assert(std::is_base_of<OD::Script, T>::value);
        return static_cast<T*>(_instances[GetType<T>()]);
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
        for(auto it = _instances.begin(); it != _instances.end();) {
            Assert(it->second != nullptr);
            it->second->OnDestroy();
            delete it->second;
            it = _instances.erase(it);
        }
    }

private:
    std::unordered_map<Type, Script*> _instances;

    void _Update(Entity e){
        for(auto i: _instances){
            i.second->_entity = e;
            Assert(i.second->_entity.IsValid() == true);

            if(i.second->_hasStarted == false){
                i.second->OnStart();
                i.second->_hasStarted = true;
            }
            i.second->OnUpdate();
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