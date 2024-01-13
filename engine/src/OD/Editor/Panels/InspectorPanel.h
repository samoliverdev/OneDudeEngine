#pragma once

#include "OD/Scene/Scene.h"
#include "OD/Core/Asset.h"
#include "OD/Editor/EditorPanel.h"

namespace OD{

class Editor;

class InspectorPanel: public EditorPanel{
public:
    InspectorPanel();
    void OnGui() override;
    
private:
    void DrawComponents(Entity entity);  
    void ShowAddComponent(Entity entity);
    void DrawComponentFromCoreComponents(Entity e, std::string name, SceneManager::CoreComponent &f);
    void DrawComponentFromSerializeFuncs(Entity e, std::string name, SceneManager::SerializeFuncs &sf);
};

}