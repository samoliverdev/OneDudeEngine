/**
 * @file DynamicModule.h
 * @brief Declaration of a dynamic module example for the OD engine.
 *
 * This module demonstrates how to integrate a dynamic library or plugin into
 * the OD framework. It provides life-cycle hooks such as initialization,
 * update, rendering, and teardown. Additionally, it manages a Taskflow
 * executor to demonstrate parallel or asynchronous tasks.
 *
 * Copyright (C) 2023
 * This file is part of the OD engine. See the LICENSE file for details.
 */

#pragma once

#include <OD/OD.h>           // OD::Module definition, macros, etc.
#include <taskflow/taskflow.hpp> // For tf::Taskflow, tf::Executor
#include <string>            // Potential for string usage
#include <vector>            // Potential for standard containers

#ifdef _WIN32
 #define OD_DYNAMICMODULE_EXPORT __declspec(dllexport)
#else
 #define OD_DYNAMICMODULE_EXPORT
#endif

extern "C" {
    /**
     * @brief Factory function used by the engine to create an instance of this module.
     * 
     * This function is exported as C-style, so the engine can discover and load the
     * module at runtime. The macro ensures correct symbol visibility on various platforms.
     */
    OD_DYNAMICMODULE_EXPORT OD::Module* CreateInstance();
}

/**
 * @class DynamicModule
 * @brief A sample dynamic module that integrates with the OD engine.
 *
 * This module demonstrates how to handle initialization, shutdown,
 * per-frame updates, rendering, GUI operations, and window resize events.
 * It also includes a Taskflow executor for managing asynchronous tasks
 * or multi-threaded workloads if desired.
 */
class DynamicModule : public OD::Module {
public:
    /**
     * @brief Module constructor. Typically, minimal logic since OD handles loading.
     */
    DynamicModule() = default;

    /**
     * @brief Virtual destructor ensures proper cleanup in derived classes, if any.
     */
    virtual ~DynamicModule() = default;

    /**
     * @brief Called when the engine loads the module for the first time.
     *
     * You can register custom components, allocate resources, or do other
     * setup tasks required by your module. The OD engine calls this once.
     */
    void OnInit() override;

    /**
     * @brief Called when the engine is about to unload this module.
     *
     * Clean up resources, deregister components, or finalize anything that
     * needs a graceful shutdown. This is the final call before the module is
     * effectively removed from memory.
     */
    void OnExit() override;

    /**
     * @brief Per-frame update logic. Called every frame or tick by the engine.
     *
     * @param deltaTime The time elapsed (in seconds) since the last frame.
     *
     * Use this for game logic, animations, or simulations that need to update
     * on each engine tick.
     */
    void OnUpdate(float deltaTime) override;

    /**
     * @brief Per-frame rendering logic. Called every frame if rendering is enabled.
     *
     * @param deltaTime The time elapsed (in seconds) since the last frame.
     *
     * Typically queue up any rendering calls or pass data to the engine's
     * renderer. Minimal logic if you rely on the engine's standard pipeline.
     */
    void OnRender(float deltaTime) override;

    /**
     * @brief GUI or immediate-mode UI drawing calls. Invoked every frame if UI is active.
     *
     * This is a good place to add ImGui-based or custom UI logic for the module.
     */
    void OnGUI() override;

    /**
     * @brief Notifies the module that the rendering surface or viewport has been resized.
     *
     * @param width  The new window/surface width in pixels.
     * @param height The new window/surface height in pixels.
     *
     * Typically used to recalculate aspect ratios or other rendering
     * parameters.
     */
    void OnResize(int width, int height) override;

private:
    /**
     * @brief A Taskflow-based graph for scheduling concurrent jobs.
     *
     * This can be used to manage multi-threaded tasks, e.g. loading resources,
     * asynchronous calculations, or background streaming.
     */
    tf::Taskflow taskflow;

    /**
     * @brief Executor to run the Taskflow job graph.
     *
     * The Executor manages worker threads and dispatches tasks defined
     * in `taskflow`. Ensure you handle thread safety appropriately.
     */
    tf::Executor executor;
};
