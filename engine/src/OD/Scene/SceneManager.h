#pragma once
#include "OD/Defines.h"
#include "Scene.h"

namespace OD{

class OD_API SceneManager: public Module{
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

#define OD_REGISTER_CORE_COMPONENT_TYPE(componentName) static inline const CoreComponentTypeRegistrator<componentName> componentNameReg{#componentName} 

}

#include "SceneManager.inl"