#pragma once
#include "Scene.h"
#include <taskflow/taskflow.hpp>

namespace OD{

template<typename... Components, typename Func>
void TransformComponent::ForEachWithTransformTaskflow(Scene& scene, Func&& func){
    #ifdef ExperimentalTransformOptimzation

    auto& registry = scene.GetRegistry();
    auto& taskflow = scene.GetTaskflow();

    std::unordered_map<Entity, tf::Task> entityTasks;

    auto view = registry.view<TransformComponent, Components...>();

    // First pass: create tasks for each entity
    for(auto entity : view){
        tf::Task task = taskflow.emplace([&, entity](){
            auto& transform = registry.get<TransformComponent>(entity);
            if constexpr(sizeof...(Components) > 0){
                func(entity, transform, registry.get<Components>(entity)...);
            } else {
                func(entity, transform);
            }
        }).name("TransformTask");

        entityTasks[entity] = task;
    }

    // Second pass: setup dependencies (parent before child)
    for(auto [entity, task]: entityTasks){
        auto& transform = registry.get<TransformComponent>(entity);
        if(transform.HasParent()){
            TransformComponent& p = scene.GetComponent<TransformComponent>(transform.Parent());
            if(p.isCollection) continue;

            Entity parent = transform.Parent();
            if (auto it = entityTasks.find(parent); it != entityTasks.end()) {
                it->second.precede(task); // parent -> child
            }
        }
    }

    scene.RunAllTaskAndSync();

#endif
}

}