#include "Scene.h"
#include <fstream>
#include "Scripts.h"
#include "OD/OD.h"
#include "OD/Serialization/Serialization.h"
#include "OD/RenderPipeline/CameraComponent.h"

namespace OD{

#pragma region TransformComponent
Matrix4 TransformComponent::GlobalModelMatrix(){
    /*Matrix4 result = transform.GetLocalModelMatrix();
    for(TransformComponent* p = registry->try_get<TransformComponent>(parent); p != nullptr; p = registry->try_get<TransformComponent>(p->parent)){
        result = p->GetLocalModelMatrix() * result;
    }
    return result;*/

    if(hasParent){
        TransformComponent& p = registry->get<TransformComponent>(parent);
        //return p.transform.GetLocalModelMatrix() * transform.GetLocalModelMatrix();
        return p.GlobalModelMatrix() * transform.GetLocalModelMatrix();
    }
    return transform.GetLocalModelMatrix();
}

Vector3 TransformComponent::InverseTransformDirection(Vector3 dir){
    Matrix4 matrix4 = GlobalModelMatrix();
    return math::inverse(matrix4) * Vector4(dir.x, dir.y, dir.z, 0);
}

Vector3 TransformComponent::TransformDirection(Vector3 dir){
    Matrix4 matrix4 = GlobalModelMatrix();
    return matrix4 * Vector4(dir.x, dir.y, dir.z, 0);
}

Vector3 TransformComponent::InverseTransformPoint(Vector3 point){
    Matrix4 matrix4 = GlobalModelMatrix();
    return math::inverse(matrix4) * Vector4(point.x, point.y, point.z, 1);
}

Vector3 TransformComponent::TransformPoint(Vector3 point){
    Matrix4 matrix4 = GlobalModelMatrix();
    return matrix4 * Vector4(point.x, point.y, point.z, 1);
}

//Quaternion InverseTransformRot(Quaternion world, Quaternion rot){
//    return Quaternion::Inverse(world) * rot;
//}

Vector3 TransformComponent::Position(){ 
    if(hasParent){
        TransformComponent& p = registry->get<TransformComponent>(parent);
        return p.TransformPoint(LocalPosition());
    }
    return LocalPosition();
}

void TransformComponent::Position(Vector3 position){
    if(hasParent){
        TransformComponent& p = registry->get<TransformComponent>(parent);
        LocalPosition(p.InverseTransformPoint(position));
    } else {
        LocalPosition(position);
    }
}

Quaternion TransformComponent::Rotation(){
    if(hasParent){
        TransformComponent& p = registry->get<TransformComponent>(parent);
        return p.Rotation() * LocalRotation();
    } 
    return LocalRotation();
}

void TransformComponent::Rotation(Quaternion rotation){
    //Assert(false);
    if(hasParent){
        TransformComponent& p = registry->get<TransformComponent>(parent);
        Quaternion world = p.Rotation();
        //localRotation(InverseTransformRot(world, rotation));
        LocalRotation(math::inverse(world) * rotation);
    } else {
        LocalRotation(rotation);
    }
}

Vector3 TransformComponent::Scale(){
    Transform t(GlobalModelMatrix());
    return t.LocalScale();
}

#pragma endregion

#pragma region InfoComponent
#pragma endregion

bool Entity::IsValid(){ return isValid && scene->registry.valid(id); }

#pragma region Scene

Scene::Scene(){
    for(auto i: SceneManager::Get().addSystemFuncs){
        LogInfo("Adding system: %s", i.first);
        i.second(*this);
    }
}

Scene::Scene(Scene& other){
    for(auto i: other.systems){
        System* newSystem = i.second->Clone(this);

        systems[i.first] = newSystem;
        if(newSystem->Type() == SystemType::Stand) standSystems.push_back(newSystem);
        if(newSystem->Type() == SystemType::Renderer) rendererSystems.push_back(newSystem);
        if(newSystem->Type() == SystemType::Physics) physicsSystems.push_back(newSystem);
    }

    auto view = other.registry.view<entt::entity>();
    for(auto it = view.begin(); it != view.end(); ++it){
        entt::entity e = registry.create(*it);

        auto& c = other.registry.get<TransformComponent>(*it);
        TransformComponent& nt = registry.emplace_or_replace<TransformComponent>(e, c);
        nt.registry = &registry;

        auto& c2 = other.registry.get<InfoComponent>(*it);
        this->registry.emplace_or_replace<InfoComponent>(e, c2);
    }

    for(auto i: SceneManager::Get().coreComponentsSerializer){
        i.second.copy(registry, other.registry);
    }
    for(auto i: SceneManager::Get().componentsSerializer){
        i.second.copy(registry, other.registry);
    }
}

Scene::~Scene(){
    registry.clear();
    for(auto i: standSystems) delete i;
    for(auto i: rendererSystems) delete i;
    for(auto i: physicsSystems) delete i;
}

Scene* Scene::Copy(Scene* other){
    return new Scene(*other); 

    Scene* scene = new Scene();

    auto view = other->registry.view<entt::entity>();
    for(auto it = view.begin(); it != view.end(); ++it){
        entt::entity e = scene->registry.create(*it);

        auto& c = other->registry.get<TransformComponent>(*it);
        TransformComponent& nt = scene->registry.emplace_or_replace<TransformComponent>(e, c);
        nt.registry = &scene->registry;

        auto& c2 = other->registry.get<InfoComponent>(*it);
        scene->registry.emplace_or_replace<InfoComponent>(e, c2);
    }

    /*auto view = other->_registry.view<InfoComponent, TransformComponent>();
    view.use<InfoComponent>();
    for(auto i: view){
        entt::entity e = scene->_registry.create(i);

        auto& c = view.get<TransformComponent>(i);
        TransformComponent& nt = scene->_registry.emplace_or_replace<TransformComponent>(e, c);
        nt._registry = &scene->_registry;

        auto& c2 = view.get<InfoComponent>(i);
        scene->_registry.emplace_or_replace<InfoComponent>(e, c2);
    }*/

    for(auto i: SceneManager::Get().coreComponentsSerializer){
        i.second.copy(scene->registry, other->registry);
    }
    for(auto i: SceneManager::Get().componentsSerializer){
        i.second.copy(scene->registry, other->registry);
    }

    return scene;
}

Entity Scene::AddEntity(std::string name){
    EntityId e = registry.create();
    
    InfoComponent& info = registry.emplace<InfoComponent>(e);
    info.name = name;
    info.id = e;
    
    TransformComponent& transform = registry.emplace<TransformComponent>(e);
    transform.registry = &registry;

    return Entity(e, this);
}

void Scene::DestroyEntity(EntityId entity){
    //if(registry.valid(entity) == false) return;
    //_DestroyEntity(entity);
    toDestroy.push_back(entity);
}

bool Scene::IsChildOf(EntityId parent, EntityId child){
    TransformComponent& _parent = registry.get<TransformComponent>(parent);

    for(auto i: _parent.children){
        if(i == child) return true;
        bool r = IsChildOf(i, child);
        if(r == true) return true;
    }

    return false;
}

void Scene::CleanParent(EntityId e){
    TransformComponent& entity = registry.get<TransformComponent>(e);

    if(entity.HasParent()){
        TransformComponent& p = registry.get<TransformComponent>(entity.Parent());
        p.children.erase(
            std::remove(p.children.begin(), p.children.end(), e),
            p.children.end()
        );
    }

    entity.parent = entt::null;
    entity.hasParent = false;
}

void Scene::SetParent(EntityId parent, EntityId child){
    if(parent == child){
        LogWarning("ERROR: Trying set parent with itself");
        return;
    }

    TransformComponent& _parent = registry.get<TransformComponent>(parent);
    TransformComponent& _child = registry.get<TransformComponent>(child);

    if(IsChildOf(child, parent)){
        LogWarning("ERROR: Trying set parent with one of your childrens");
        return;
    }

    if(_child.HasParent()){
        TransformComponent& p = registry.get<TransformComponent>(_child.Parent());
        p.children.erase(
            std::remove(p.children.begin(), p.children.end(), child),
            p.children.end()
        );
    }

    _parent.children.emplace_back(child);
    _child.parent = parent;
    _child.hasParent = true;
}

Entity Scene::Instantiate(const Ref<Model> model){
    if(model == nullptr){
        LogWarning("Trying instantiate a null model");
        return Entity();
    }

    Entity root = AddEntity("Root");

    for(auto i: model->renderTargets){
        Entity mesh = AddEntity(model->skeleton.GetJointName(i.bindPoseIndex));
        MeshRendererComponent& meshRenderer = mesh.AddComponent<MeshRendererComponent>();
        TransformComponent& transform = mesh.GetComponent<TransformComponent>();

        meshRenderer.material = model->materials[i.materialIndex];
        meshRenderer.mesh = model->meshs[i.meshIndex];
        auto targetMatrix = Transform(model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex));

        transform.LocalPosition(targetMatrix.LocalPosition());
        transform.LocalRotation(targetMatrix.LocalRotation());
        transform.LocalScale(targetMatrix.LocalScale());
        meshRenderer.UpdateAABB();

        SetParent(root.id, mesh.id);
    }

    return root;
}

Camera& Scene::GetMainCamera(){ 
    return mainCamera; 
}   

Entity Scene::GetMainCamera2(){
    for(auto e: registry.view<CameraComponent>()){
        return Entity(e, this);
    }
    return Entity();
}

void Scene::Start(){
    running = true;
}

void Scene::Update(){ 
    OD_PROFILE_SCOPE("Scene::Update");

    for(auto e: toDestroy){
        _DestroyEntity(e);
    }
    toDestroy.clear();

    //if(_running == false) return;

    for(auto s: physicsSystems) s->Update();
    if(running == false) return;
    for(auto s: standSystems) s->Update();
}

void Scene::Draw(){
    OD_PROFILE_SCOPE("Scene::Draw");

    Graphics::Begin();
    for(auto& s: rendererSystems) s->Update();        
    Graphics::End();
}

void Scene::_AddEntityPrefab(entt::registry& registry, std::vector<entt::entity>& entities, entt::entity entity, std::string prefabPath, bool isRoot){
    entities.push_back(entity);

    InfoComponent& infoComponent = registry.get<InfoComponent>(entity);
    infoComponent.entityType = isRoot ? EntityType::PrefabRoot : EntityType::PrefabChild;
    infoComponent.prefabPath = prefabPath;

    TransformComponent& trans = registry.get<TransformComponent>(entity);
    for(auto e: trans.children){
        _AddEntityPrefab(registry, entities, e, std::string(""));
    }
}

void Scene::Save(const char* path, entt::entity root){
    std::ofstream os(path);
    ODOutputArchive archive(os);

    std::vector<entt::entity> entities;
    std::vector<entt::entity> entitiesAll;
    
    /*auto entityView = registry.view<entt::entity>();
    std::vector<entt::entity> entities(entityView.begin(), entityView.end()); //std::vector<entt::entity> entities(entityView.rbegin(), entityView.rend());
    archive(cereal::make_nvp("Entities", entities));*/

    if(root == entt::null){
        registry.sort<InfoComponent>([](const entt::entity lhs, const entt::entity rhs){
            return lhs < rhs;
        });

        auto entityView = registry.view<TransformComponent, InfoComponent>();
        entityView.use<InfoComponent>();
        for(auto e: entityView){
            InfoComponent& infoComponent = registry.get<InfoComponent>(e);
            if(infoComponent.entityType == EntityType::Stand){
                entities.push_back(e);
                entitiesAll.push_back(e);
            }
            if(infoComponent.entityType == EntityType::PrefabRoot){
                entitiesAll.push_back(e);
            }
        }
    } else {
        //Assert(false);
        _AddEntityPrefab(registry, entities, root, path, true);
        entitiesAll = std::vector<entt::entity>(entities.begin(), entities.end());
    }

    archive(cereal::make_nvp("Entities", entitiesAll));

    _SaveComponent<InfoComponent>(archive, entitiesAll, registry, "InfoComponent");
    _SaveComponent<TransformComponent>(archive, entitiesAll, registry, "TransformComponent");

    for(auto i: SceneManager::Get().componentsSerializer){
        i.second.snapshotOut(archive, entities, registry, std::string(i.first));
    }
    for(auto i: SceneManager::Get().coreComponentsSerializer){
        i.second.snapshotOut(archive, entities, registry, std::string(i.first));
    }
}

void Scene::_LoadTransform(ODInputArchive& archive, std::unordered_map<entt::entity,entt::entity>& loadLookup, entt::registry& registry, std::string componentName, bool handleRootPrefab){
    std::vector<TransformComponent> components;
    std::vector<entt::entity> componentsEntities;
    archive(cereal::make_nvp(componentName + "s", components));
    archive(cereal::make_nvp(componentName + "Entities", componentsEntities));
    for(int i = 0; i < components.size(); i++){
        //if(loadLookup[componentsEntities[i]] == entt::null) continue;
        bool _handleRootPrefab = 
            registry.any_of<TransformComponent>(loadLookup[componentsEntities[i]]) && 
            registry.get<InfoComponent>(loadLookup[componentsEntities[i]]).entityType == EntityType::PrefabRoot;

        if(_handleRootPrefab == false){
            TransformComponent& trans = registry.emplace<TransformComponent>(loadLookup[componentsEntities[i]], components[i]);
            trans.registry = &registry;
            trans.hasParent = components[i].hasParent;
            if(trans.hasParent) 
                trans.parent = loadLookup[components[i].parent];
            else 
                trans.parent = entt::null;
            
            trans.children.clear();
            for(auto j: components[i].children){
                trans.children.push_back(loadLookup[j]);
            }
        } else {
            TransformComponent& trans = registry.get<TransformComponent>(loadLookup[componentsEntities[i]]);
            trans.children.clear();
            for(auto j: components[i].children){
                trans.children.push_back(loadLookup[j]);
            }
        }    
    }
}

void Scene::Load(const char* path){
    std::ifstream is(path);
    ODInputArchive archive(is);

    std::unordered_map<entt::entity, entt::entity> loadLookup;

    std::vector<entt::entity> entities;
    archive(cereal::make_nvp("Entities", entities));
    for(auto i: entities){
        entt::entity e = registry.create();
        loadLookup[i] = e;
    }

    _LoadComponent<InfoComponent>(archive, loadLookup, registry, "InfoComponent");
    _LoadTransform(archive, loadLookup, registry, "TransformComponent");

    for(auto i: SceneManager::Get().componentsSerializer){
        try{
            i.second.snapshotIn(archive, loadLookup, registry, std::string(i.first));
        }catch(...){}
    }
    for(auto i: SceneManager::Get().coreComponentsSerializer){
        try{
            i.second.snapshotIn(archive, loadLookup, registry, std::string(i.first));
        }catch(...){}
    }

    auto entityView = registry.view<InfoComponent>();
    for(auto e: entityView){
        InfoComponent& info = registry.get<InfoComponent>(e);
        if(info.entityType == EntityType::PrefabRoot){
            _Load(info.prefabPath.c_str(), e);
        }
    }

    LogWarning("LoadingScene: %s Succefu", path);
}

void Scene::_Load(const char* path, entt::entity prefab){
    LogWarning("LoadingPrefab: %s", path);
    //Assert(false);

    std::ifstream is(path);
    ODInputArchive archive(is);

    std::unordered_map<entt::entity, entt::entity> loadLookup;
    std::vector<entt::entity> entities;
    archive(cereal::make_nvp("Entities", entities));

    for(auto i: entities){
        if(i == entities[0]){
            loadLookup[i] = prefab;
        } else {
            loadLookup[i] = registry.create();
        }
    } 

    _LoadComponent<InfoComponent>(archive, loadLookup, registry, "InfoComponent");
    _LoadTransform(archive, loadLookup, registry, "TransformComponent", true);

    for(auto i: SceneManager::Get().componentsSerializer){
        try{
            i.second.snapshotIn(archive, loadLookup, registry, std::string(i.first));
        }catch(...){}
    }
    for(auto i: SceneManager::Get().coreComponentsSerializer){
        try{
            i.second.snapshotIn(archive, loadLookup, registry, std::string(i.first));
        }catch(...){}
    }

    //auto entityView = registry.view<InfoComponent>();
    for(auto e: loadLookup){
        if(e.first == entities[0]) continue;
        InfoComponent& info = registry.get<InfoComponent>(e.second);
        if(info.entityType == EntityType::PrefabRoot){
            _Load(info.prefabPath.c_str(), e.second);
        }
    }
}

Entity Scene::InstantiatePrefab(const char* path){
    LogWarning("LoadingPrefab: %s", path);
    //Assert(false);

    std::ifstream is(path);
    ODInputArchive archive(is);

    std::unordered_map<entt::entity, entt::entity> loadLookup;
    std::vector<entt::entity> entities;
    archive(cereal::make_nvp("Entities", entities));

    Entity root;

    for(auto i: entities){
        loadLookup[i] = registry.create();
        if(i == entities[0]) root = Entity(loadLookup[i], this);
    } 

    _LoadComponent<InfoComponent>(archive, loadLookup, registry, "InfoComponent");
    _LoadTransform(archive, loadLookup, registry, "TransformComponent", true);

    for(auto i: SceneManager::Get().componentsSerializer){
        try{
            i.second.snapshotIn(archive, loadLookup, registry, std::string(i.first));
        }catch(...){}
    }
    for(auto i: SceneManager::Get().coreComponentsSerializer){
        try{
            i.second.snapshotIn(archive, loadLookup, registry, std::string(i.first));
        }catch(...){}
    }

    //auto entityView = registry.view<InfoComponent>();
    for(auto e: loadLookup){
        if(e.first == entities[0]) continue;
        InfoComponent& info = registry.get<InfoComponent>(e.second);
        if(info.entityType == EntityType::PrefabRoot){
            _Load(info.prefabPath.c_str(), e.second);
        }
    }

    return root;
}

void Scene::_DestroyEntity(EntityId entity){
    TransformComponent& transform = registry.get<TransformComponent>(entity);

    for(auto i: transform.children){
        _DestroyEntity(i);
    }

    if(transform.hasParent){
        TransformComponent& parent = registry.get<TransformComponent>(transform.parent);
        //parent._children.clear();
        parent.children.erase(
            std::remove(
                parent.children.begin(), 
                parent.children.end(), 
                entity
            ), 
            parent.children.end()
        );
    }

    registry.destroy(entity);
}

#pragma endregion

}