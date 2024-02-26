#pragma once
#include "OD/Defines.h"
#include "OD/Scene/Scene.h"
#include "OD/Scene/SceneManager.h"
#include "OD/Core/Asset.h"
#include "OD/Editor/EditorPanel.h"

namespace OD{

class Editor;

class OD_API InspectorPanel: public EditorPanel{
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