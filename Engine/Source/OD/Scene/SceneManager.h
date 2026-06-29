#pragma once
#include "Scene.h"
#include "OD/Core/Module.h"

namespace OD{

class Script;
struct ScriptComponent;

class OD_API SceneManager: public Module{
public:
    //SceneManager(){ name = "SceneManager"; }

    enum class SceneState {Playing, Paused, Editor};

    static SceneManager& Get();

    bool _HasTempScene();
    void _LoadTempClonedScene(bool isSave);
    
    void LoadScene(const std::string& path);
    bool IsLoading();

    SceneState GetSceneState();
    inline bool InEditor();
    Ref<Scene> GetActiveScene();
    void SetActiveScene(Ref<Scene> s);
    Ref<Scene> NewScene();
    void DestroyActiveScene();

    template<typename T> void RegisterCoreComponent(const std::string& name, const std::string& groupName = "");
    template<typename T> void UnRegisterCoreComponent(const std::string& name);

    //Empty/Tag Components can cause crach on dll usage, for now recomend use "EmptyComponentBody" with normal RegisterComponent
    template<typename T> void RegisterTagComponent(const std::string& name, const std::string& groupName = "");

    //template<typename T> void RegisterCoreComponentSimple(const char* name);
    template<typename T> void RegisterComponent(const std::string& name, const std::string& groupName = "");
    template<typename T> void RegisterScript(const std::string& name);
    template<typename T> void RegisterSystem(const std::string& name);    

    template<typename T> void AddGlobalSystem();
    template<typename T> void RemoveGlobalSystem();
    template<typename T> T* GetGlobalSystem();
    template<typename T> T* GetGlobalSystemDynamic();
    
    // Module Parent Overloaded
    void OnInit() override;
    void OnExit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender(float deltaTime) override;
    void OnGUI() override;
    void OnResize(int width, int height) override;

    inline bool DeleteOnExit() override { return false; }

private:
    SceneManager(){ name = "SceneManager"; }

    struct SerializeFuncs{
        std::string groupName;
        std::string displayName;
        
        std::function<bool(Entity&,Scene&)> hasComponent;
        std::function<void(Entity&,Scene&)> addComponent;
        std::function<void(Entity&,Scene&)> removeComponent;
        std::function<void(Entity&,Scene&)> resetComponent;
        std::function<void(Entity&, Entity&,Scene&,Scene&)> copyComponent;
        std::function<void(Entity&,Scene&)> onGui;
        std::function<void(entt::registry& dst, entt::registry& src)> copy;
        std::function<void(ODOutputArchive& out, std::vector<entt::entity>& entities, entt::registry& registry, std::string name)> snapshotOut;
        std::function<void(ODInputArchive& out, std::unordered_map<entt::entity,entt::entity>& loadLookup, entt::registry& registry, std::string name)> snapshotIn;

        std::function<Script*(ScriptComponent&)> addScript;
        std::function<void(ScriptComponent&)> removeScript;
        std::function<void(ODOutputArchive&, Script*)> scriptSave;
        std::function<void(ODInputArchive&, Script*)> scriptLoad;

        std::function<Type()> getType;
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

    std::vector<System*> globalStandSystems;
    std::vector<System*> globalAnimationSystems;
    
    std::vector<System*> globalPrePhysicsSystems;
    std::vector<System*> globalFixedPhysicsSystems;
    std::vector<System*> globalPostPhysicsSystems;

    std::vector<System*> globalLateSystems;
    std::vector<System*> globalRendererSystems;
    std::unordered_map<Type, System*> globalSystems;

    SceneState sceneState;
    bool inEditor;

    Ref<Scene> activeScene = nullptr;
    
    Ref<Scene> tempScene = nullptr;
    bool tempSceneIsSave = false;
    bool toLoadTempScene = false;
    
    std::string toLoad;
    bool isLoading = false;

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