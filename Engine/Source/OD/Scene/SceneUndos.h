#pragma once
#include "Scene.h"
#include "OD/Core/Undo.h"

namespace OD{

template<typename T>
class UndoValueComponentCommand : public IUndoCommand {
public:
    UndoValueComponentCommand(Scene* scene, Entity entity, const T& oldValue, const T& newValue)
        : scene(scene), entity(entity), oldValue(oldValue), newValue(newValue) {}

    void Undo() override {
        if(scene->IsValid(entity) && scene->HasComponent<T>(entity))
            scene->GetComponent<T>(entity) = oldValue;
    }

    void Redo() override {
        if(scene->IsValid(entity) && scene->HasComponent<T>(entity))
            scene->GetComponent<T>(entity) = newValue;
    }

private:
    Scene* scene;
    Entity entity;
    T oldValue;
    T newValue;
};

template<typename T>
class UndoValueComponentBatchCommand : public IUndoCommand {
public:
    UndoValueComponentBatchCommand(
        Scene* scene,
        const std::vector<Entity>& entities,
        const std::vector<T>& oldValues,
        const std::vector<T>& newValues)
        :scene(scene), entities(entities), oldValues(oldValues), newValues(newValues){
        assert(entities.size() == oldValues.size());
        assert(entities.size() == newValues.size());
    }

    void Undo() override {
        for (size_t i = 0; i < entities.size(); ++i) {
            Entity entity = entities[i];
            if (scene->IsValid(entity) && scene->HasComponent<T>(entity)) {
                scene->GetComponent<T>(entity) = oldValues[i];
            }
        }
    }

    void Redo() override {
        for (size_t i = 0; i < entities.size(); ++i) {
            Entity entity = entities[i];
            if (scene->IsValid(entity) && scene->HasComponent<T>(entity)) {
                scene->GetComponent<T>(entity) = newValues[i];
            }
        }
    }

private:
    Scene* scene;
    std::vector<Entity> entities;
    std::vector<T> oldValues;
    std::vector<T> newValues;
};

template<typename T>
class UndoAddComponent : public IUndoCommand {
public:
    UndoAddComponent(Scene* scene, Entity entity)
        : scene(scene), entity(entity) {}

    void Undo() override {
        if (scene->IsValid(entity) && scene->HasComponent<T>(entity))
            scene->RemoveComponent<T>(entity);
    }

    void Redo() override {
        if (scene->IsValid(entity) && !scene->HasComponent<T>(entity))
            scene->AddComponent<T>(entity);
    }

private:
    Scene* scene;
    Entity entity;
};

template<typename T>
class UndoRemoveComponent : public IUndoCommand {
public:
    UndoRemoveComponent(Scene* scene, Entity entity)
        : scene(scene), entity(entity) {
        if (scene->IsValid(entity) && scene->HasComponent<T>(entity))
            backup = scene->GetComponent<T>(entity);
    }

    void Undo() override {
        if (scene->IsValid(entity) && !scene->HasComponent<T>(entity))
            scene->AddComponent<T>(entity, backup);
    }

    void Redo() override {
        if (scene->IsValid(entity) && scene->HasComponent<T>(entity))
            scene->RemoveComponent<T>(entity);
    }

private:
    Scene* scene;
    Entity entity;
    T backup;
};

}
