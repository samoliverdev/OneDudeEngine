#pragma once

#include "OD/Scene/Scene.h"
#include "OD/Core/Asset.h"

namespace OD{

class Editor;

class InspectorPanel{
public:
    inline void SetEditor(Editor* editor){ _editor = editor; }
    inline void SetScene(Scene* scene){ _scene = scene; }

    void OnGui();
    
private:
    Scene* _scene;
    Editor* _editor;

    void DrawComponents(Entity entity);  
    void ShowAddComponent(Entity entity);
    void DrawComponentFromCoreComponents(Entity e, std::string name, SceneManager::CoreComponent &f);
    void DrawComponentFromSerializeFuncs(Entity e, std::string name, SceneManager::SerializeFuncs &sf);
};

}