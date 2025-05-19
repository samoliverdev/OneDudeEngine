#pragma once
#include "OD/Defines.h"
#include "Scene.h"
#include "OD/Serialization/CerealImGui.h"

namespace OD{

class OD_API SceneManager: public Module{
public:
    enum class SceneState {Playing, Paused, Editor};

    static SceneManager& Get();

    SceneState GetSceneState();
    inline bool InEditor();
    Scene* GetActiveScene();
    void SetActiveScene(Scene* s);
    Scene* NewScene();
    void DestroyActiveScene();

    template<typename T> void RegisterCoreComponent(const std::string& name);
    template<typename T> void UnRegisterCoreComponent(const std::string& name);

    template<typename T> void RegisterTagComponent(const std::string& name);

    //template<typename T> void RegisterCoreComponentSimple(const char* name);
    template<typename T> void RegisterComponent(const std::string& name);
    template<typename T> void RegisterScript(const std::string& name);
    template<typename T> void RegisterSystem(const std::string& name);    
    
    // Module Parent Overloaded
    void OnInit() override;
    void OnExit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;

    inline bool DeleteOnExit() override { return false; }

private:
    SceneManager(){}

    struct SerializeFuncs{
        std::function<bool(Entity&,Scene&)> hasComponent;
        std::function<void(Entity&,Scene&)> addComponent;
        std::function<void(Entity&,Scene&)> removeComponent;
        std::function<void(Entity&, Entity&,Scene&)> copyComponent;
        std::function<void(Entity&,Scene&)> onGui;
        std::function<void(entt::registry& dst, entt::registry& src)> copy;
        std::function<void(ODOutputArchive& out, std::vector<entt::entity>& entities, entt::registry& registry, std::string name)> snapshotOut;
        std::function<void(ODInputArchive& out, std::unordered_map<entt::entity,entt::entity>& loadLookup, entt::registry& registry, std::string name)> snapshotIn;
    };

    /*struct CoreComponent{
        std::function<bool(Entity&)> hasComponent;
        std::function<void(Entity&)> addComponent;
        std::function<void(Entity&)> removeComponent;
        std::function<void(Entity&)> onGui;
        std::function<void(entt::registry& dst, entt::registry& src)> copy;
        std::function<void(ODOutputArchive& out, std::vector<entt::entity>& entities, entt::registry& registry, std::string name)> snapshotOut;
        std::function<void(ODInputArchive& out, std::unordered_map<entt::entity,entt::entity>& loadLookup, entt::registry& registry, std::string name)> snapshotIn;
    };*/

    SceneState sceneState;
    bool inEditor;

    Scene* activeScene;

    std::unordered_map<std::string, SerializeFuncs> coreComponentsSerializer;
    std::unordered_map<std::string, SerializeFuncs> componentsSerializer;
    std::unordered_map<std::string, SerializeFuncs> scriptsSerializer;
    //std::unordered_map<const char*, std::function<void(Scene&)> > addSystemFuncs;
    std::vector< std::function<void(Scene&)> > addSystemFuncs;

    friend class Editor;
    friend class SceneHierarchyPanel;
    friend class InspectorPanel;
    friend class Scene;
    friend struct ScriptComponent;
};

#define OD_REGISTER_CORE_COMPONENT_TYPE(componentName) static inline const CoreComponentTypeRegistrator<componentName> componentNameReg{#componentName} 

void SceneManagerModuleInit();

}

#include "SceneManager.inl"