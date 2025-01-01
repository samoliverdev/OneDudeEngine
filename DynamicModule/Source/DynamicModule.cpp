/**
 * @file DynamicModule.cpp
 * @brief Implements the DynamicModule for the OD engine, including a sample
 *        DynamicComponent with a basic serialization example.
 *
 * Copyright (C) 2001-2024
 * Licensed under the terms of the GNU General Public License (GPL) v3 or later.
 */

#include "DynamicModule.h"

// Example: If the engine's logging macros support debug levels, we can customize
// logging calls with verbosity as needed.
#include <sstream> // for std::ostringstream (if desired for advanced logging)

// Use the OD namespace or specify otherwise
using namespace OD;

/**
 * @class DynamicComponent
 * @brief A demonstration component with simple data fields to serialize/deserialize.
 *
 * This component is registered with SceneManager as a "DynamicComponent"
 * and can be attached to entities in the scene at runtime.
 */
struct DynamicComponent
{
    std::string name;
    int age = 0;
    float test = 20.0f;
    std::string st = "dfddfd";

    /**
     * @brief Cereal-compatible serialization function.
     * This function saves/loads the component's data fields.
     *
     * @tparam Archive Cereal archive type.
     * @param ar       The cereal archive to read from/write to.
     */
    template <class Archive>
    void serialize(Archive& ar)
    {
        ArchiveDump(ar, CEREAL_NVP(name));
        ArchiveDump(ar, CEREAL_NVP(age));
        ArchiveDump(ar, CEREAL_NVP(test));
        ArchiveDump(ar, CEREAL_NVP(st));
    }
};

/**
 * @brief OD's required factory function for dynamic module creation.
 *        Called by engine code to instantiate this module.
 *
 * @return A pointer to a newly created DynamicModule.
 */
OD::Module* CreateInstance()
{
    return new DynamicModule();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of DynamicModule Methods
///////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Called when the DynamicModule is initialized by the engine.
 *        Typically used for registering custom components/systems.
 */
void DynamicModule::OnInit()
{
    LogInfo("DynamicModule::OnInit - Beginning initialization of dynamic module...");

    // Example: We register our sample DynamicComponent so the engine can
    // understand how to create, serialize, and manage it.
    SceneManager::Get().RegisterCoreComponent<DynamicComponent>("DynamicComponent");

    // Additional initialization logic or resource loading can go here.
    // Example: Create an entity and attach the dynamic component
    // Scene* scene = SceneManager::Get().GetActiveScene();
    // if(scene) {
    //     Entity e = scene->AddEntity("FromDynamicModule");
    //     e.AddComponent<DynamicComponent>();
    // }

    LogInfo("DynamicModule::OnInit - Done registering 'DynamicComponent' with SceneManager.");
}

/**
 * @brief Called when the module is about to exit or unload.
 *        Free any resources or references you might have allocated in OnInit().
 */
void DynamicModule::OnExit()
{
    LogInfo("DynamicModule::OnExit - Cleaning up dynamic module resources...");
    // Perform any necessary teardown or resource releasing tasks here.
    // Example: remove or finalize any dynamic data if needed.
    LogInfo("DynamicModule::OnExit - Teardown complete.");
}

/**
 * @brief Called each frame to update logic, animations, or other dynamic aspects.
 *
 * @param deltaTime The time (in seconds) since the last frame. Generally used to
 *                  keep animation or movement consistent across varying framerates.
 */
void DynamicModule::OnUpdate(float deltaTime)
{
    // If needed, we could run logic that requires consistent updates per frame.
    // e.g., checking for user input, or updating dynamic entity behavior.
    // For now, we just have a placeholder to avoid spamming logs every frame.

    // Example verbose debug:
    // LogDebug("DynamicModule::OnUpdate - dt = {}", deltaTime);
}

/**
 * @brief Called each frame to render or queue rendering operations for this module.
 *
 * @param deltaTime The time (in seconds) since the last frame; often used for
 *                  interpolation or extrapolation of visuals.
 */
void DynamicModule::OnRender(float deltaTime)
{
    // If this module contributed visual elements, we would
    // add calls to the engine's rendering pipeline here.
    // Example debug:
    // LogDebug("DynamicModule::OnRender - dt = {}", deltaTime);
}

/**
 * @brief Called when the GUI is drawn. Typically, game devs can place ImGui calls
 *        or other immediate-mode UI logic here.
 */
void DynamicModule::OnGUI()
{
    // e.g., if ImGui is integrated, we could create windows or debug panels here.
    // ImGui::Begin("DynamicModule Debug");
    // ImGui::Text("Test: %f", someValue);
    // ImGui::End();
}

/**
 * @brief Called when the rendering surface or window is resized, typically used
 *        to reconfigure the viewport, update projection matrices, etc.
 *
 * @param width  The new width of the window or rendering surface in pixels.
 * @param height The new height of the window or rendering surface in pixels.
 */
void DynamicModule::OnResize(int width, int height)
{
    // Usually, we'll pass along the new dimensions to our rendering pipeline, etc.
    // Example debug:
    LogInfo("DynamicModule::OnResize - New size: {} x {}", width, height);
}
