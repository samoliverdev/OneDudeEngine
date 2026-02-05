#pragma once
#include "Scene.h"

namespace OD{

class OD_API Prefab: public Asset{
    friend class Scene;
public:
    Prefab();
    ~Prefab();

    template<typename T> bool HasComponentInRoot();
    template<typename T> T& GetComponentInRoot();
    template<typename T> T* TryGetComponentInRoot();

    inline Entity Root(){ return root; }

    void OnGui() override;
    bool LoadFromFile(const std::string& path) override;
    std::vector<std::string> GetFileAssociations() override;
private:
    Scene* scene = nullptr;
    Entity root = EntityNull;
};

}

#include "Prefab.inl"