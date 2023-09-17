#pragma once

#include "OD/Scene/Scene.h"

namespace OD{

class SceneHierarchyPanel{
public:
    inline void SetScene(Scene* scene){ _scene = scene; }
    void OnGui(bool* showSceneHierarchy, bool* showInspector);
    inline void UnselectContext(){ _selectionContext = Entity(); }
    Entity selectionContext(){ return _selectionContext; }
    
private:
    Scene* _scene;
    Entity _selectionContext;

    void DrawEntityNode(Entity entity, bool root);
    void DrawComponents(Entity entity);  
    void ShowAddComponent(Entity entity);

    void DrawComponentFromSerializeFuncs(Entity e, std::string name, SceneManager::SerializeFuncs &sf);

    void DrawArchive(Archive& ar);
};

}