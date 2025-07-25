#include "Scene.h"
#include "SceneManager.h"
#include "Scripts.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Time.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Serialization/CerealImGui.h"
#include "OD/Graphics/Model.h"
#include "OD/Graphics/Graphics.h"
#include "OD/Serialization/Serialization.h"
#include "OD/RenderPipeline/CameraComponent.h"
#include "OD/RenderPipeline/MeshRendererComponent.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"
#include "OD/LuaScripting/LuaMetaUltis.h"
#include "OD/Core/Application.h"
#include <fstream>

#include "OD/Editor/Editor.h"

namespace OD{

GlobalSceneData globalSceneData;

int LayerMask::GetLayerByName(const std::string& name){
    Assert(globalSceneData.layerNames.size() == LayerMax);

    for(int i = 0; i < globalSceneData.layerNames.size(); i++){
        if(globalSceneData.layerNames[i] == name) return (1 << i);
    }
    return LayerNone;
}

GlobalSceneData& GetGlobalSceneData(){
    return globalSceneData;
}

#pragma region TransformComponent

void TransformComponent::SetGlobalAsDirty(){
    #ifdef ExperimentalTransformOptimzation
    globalIsDirty = true;
    for(auto& i: children){
        TransformComponent& t = registry->get<TransformComponent>(i);
        t.SetGlobalAsDirty();
    }
    #endif
}

void TransformComponent::UpdateGlobalTransformCacheIfNeeded(){
    #ifdef ExperimentalTransformOptimzation
    if(globalIsDirty){
        globalIsDirty = false;
        if(hasParent){
            TransformComponent& p = registry->get<TransformComponent>(parent);
            globalTransform.localModelMatrix = p.GlobalModelMatrix() * transform.GetLocalModelMatrix();
            globalTransform.localPosition = p.TransformPoint(LocalPosition());
            globalTransform.localRotation = p.Rotation() * LocalRotation();
            globalTransform.localScale = p.Scale() * LocalScale(); // Aqui está a escala acumulada
        } else {
            globalTransform.localModelMatrix = transform.GetLocalModelMatrix();
            globalTransform.localPosition = LocalPosition();
            globalTransform.localRotation = LocalRotation();
            globalTransform.localScale = LocalScale(); // sem pai, usa local diretamente
        }
    }
    #endif
}

const Matrix4& TransformComponent::GlobalModelMatrixReadSafe() const{
    #ifdef ExperimentalTransformOptimzation
    return globalTransform.localModelMatrix;
    #else
    Assert(false);
    return Matrix4Identity;
    #endif
}

const Matrix4 TransformComponent::GlobalModelMatrix(){
    /*Matrix4 result = transform.GetLocalModelMatrix();
    for(TransformComponent* p = registry->try_get<TransformComponent>(parent); p != nullptr; p = registry->try_get<TransformComponent>(p->parent)){
        result = p->GetLocalModelMatrix() * result;
    }
    return result;*/

    #ifdef ExperimentalTransformOptimzation

    /*if(globalIsDirty){
        globalIsDirty = false;
        if(hasParent){
            TransformComponent& p = registry->get<TransformComponent>(parent);
            globalTransform.localModelMatrix = p.GlobalModelMatrix() * transform.GetLocalModelMatrix();
            globalTransform.localPosition = p.TransformPoint(LocalPosition());
            globalTransform.localRotation = p.Rotation() * LocalRotation();
        } else {
            globalTransform.localModelMatrix = transform.GetLocalModelMatrix();
            globalTransform.localPosition = LocalPosition();
            globalTransform.localRotation = LocalRotation();
        }
    }*/
    UpdateGlobalTransformCacheIfNeeded();
    return globalTransform.localModelMatrix;

    #else

    if(hasParent){
        TransformComponent& p = registry->get<TransformComponent>(parent);
        return p.GlobalModelMatrix() * transform.GetLocalModelMatrix();
    }
    return transform.GetLocalModelMatrix();
    #endif
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

const Vector3& TransformComponent::PositionReadSafe() const{
    #ifdef ExperimentalTransformOptimzation
    return globalTransform.localPosition;
    #else 
    Assert(false);
    return Vector3Zero;
    #endif
}

Vector3 TransformComponent::Position(){ 
    #ifdef ExperimentalTransformOptimzation

    /*if(globalIsDirty){
        globalIsDirty = false;
        if(hasParent){
            TransformComponent& p = registry->get<TransformComponent>(parent);
            globalTransform.localPosition = p.TransformPoint(LocalPosition());
        } else {
            globalTransform.localPosition = LocalPosition();
        }
    }*/
    UpdateGlobalTransformCacheIfNeeded();
    return globalTransform.localPosition;

    #else 
    if(hasParent){
        TransformComponent& p = registry->get<TransformComponent>(parent);
        return p.TransformPoint(LocalPosition());
    }
    return LocalPosition();
    #endif
}

void TransformComponent::Position(Vector3 position){
    #ifdef ExperimentalTransformOptimzation
    SetGlobalAsDirty();
    #endif

    if(hasParent){
        TransformComponent& p = registry->get<TransformComponent>(parent);
        LocalPosition(p.InverseTransformPoint(position));
    } else {
        LocalPosition(position);
    }
}

Quaternion TransformComponent::Rotation(){
    #ifdef ExperimentalTransformOptimzation

    /*if(globalIsDirty){
        globalIsDirty = false;
        if(hasParent){
            TransformComponent& p = registry->get<TransformComponent>(parent);
            globalTransform.localRotation = p.Rotation() * LocalRotation();
        } else {
            globalTransform.localRotation = LocalRotation();
        }
    }*/
    UpdateGlobalTransformCacheIfNeeded();
    return globalTransform.localRotation;

    #else 

    if(hasParent){
        TransformComponent& p = registry->get<TransformComponent>(parent);
        return p.Rotation() * LocalRotation();
    } 
    return LocalRotation();
    #endif
}

void TransformComponent::Rotation(Quaternion rotation){
    #ifdef ExperimentalTransformOptimzation
    SetGlobalAsDirty();
    #endif

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
    #ifdef ExperimentalTransformOptimzation
    
    UpdateGlobalTransformCacheIfNeeded();
    return globalTransform.localScale;
    
    #else

    Transform t(GlobalModelMatrix());
    return t.LocalScale();
    #endif
}

bool TransformComponent::FindEntityInChildren(const std::string& name, Entity& out){
    for(auto& i: children){
        TransformComponent& child = registry->get<TransformComponent>(i);
        InfoComponent& info = registry->get<InfoComponent>(i);

        if(child.children.size() > 0){
            bool r = child.FindEntityInChildren(name, out);
            if(r == true) return true;
        }

        if(info.name == name){
            out = i;
            return true;
        }
    }

    return false;
}

void TransformComponent::CreateLuaBind(sol::state& lua){
    Scene::RegisterMetaComponent<TransformComponent>();
    lua.new_usertype<TransformComponent>(
        "TransformComponent",
        "TypeId", &entt::type_hash<TransformComponent>::value,
        sol::call_constructor,
        sol::factories([](){ return TransformComponent(); }),
        "LocalPosition", sol::overload(
            [](TransformComponent& t){ return t.LocalPosition(); },
            [](TransformComponent& t, Vector3 v){ t.LocalPosition(v); }
        ),
        "LocalEulerAngles", sol::overload(
            [](TransformComponent& t){ return t.LocalEulerAngles(); },
            [](TransformComponent& t, Vector3 v){ t.LocalEulerAngles(v); }
        ),
        "LocalScale", sol::overload(
            [](TransformComponent& t){ return t.LocalScale(); },
            [](TransformComponent& t, Vector3 v){ t.LocalScale(v); }
        )
    );
}

#pragma endregion

#pragma region InfoComponent

void InfoComponent::CreateLuaBind(sol::state& lua){
    Scene::RegisterMetaComponent<InfoComponent>();
    lua.new_usertype<InfoComponent>(
        "InfoComponent",
        "TypeId", &entt::type_hash<InfoComponent>::value,
        sol::call_constructor,
        sol::factories([](){ return InfoComponent(); }),
        "name", &InfoComponent::name,
        "tag", &InfoComponent::tag//,
        //"Id", [](InfoComponent& cmp){ return cmp.Id(); }
        //"Id", &InfoComponent::Id
    );
}

#pragma endregion

#pragma region Entity
//bool Entity::IsValid(){ return /*isValid*/ scene != nullptr && scene->registry.valid(id); }
/*
void Entity::CreateLuaBind(sol::state& lua){
    using namespace entt::literals;
    lua.new_usertype<Entity>(
        "Entity",
        "GetInfoComponent", &Entity::GetComponent<InfoComponent>,
        "GetTransformComponent", &Entity::GetComponent<TransformComponent>,
        "AddComponent", [](Entity& e, const sol::table& comp, sol::this_state s) -> sol::object{
            if(!comp.valid()) return sol::lua_nil_t{};
            const auto component = InvokeMetaFunction(GetIdType(comp), "_AddComponent"_hs, e, comp, s);
            return component ? component.cast<sol::reference>() : sol::lua_nil_t{};
        },
        "HasComponent", [](Entity& e, const sol::table& comp){
            const auto hasComp = InvokeMetaFunction(GetIdType(comp), "_HasComponent"_hs, e);
            return hasComp ? hasComp.cast<bool>() : false;
        },
        "GetComponent", [](Entity& e, const sol::table& comp, sol::this_state s){
            const auto component = InvokeMetaFunction(GetIdType(comp), "_GetComponent"_hs, e, s);
            return component ? component.cast<sol::reference>() : sol::lua_nil_t{};
        },
        "RemoveComponent", [](Entity& e, const sol::table& comp){
            InvokeMetaFunction(GetIdType(comp), "_RemoveComponent"_hs, e);
        },
        "GetScene", &Entity::GetScene,
        "Id", &Entity::Id,
        "IsValid", &Entity::IsValid
    );
}
*/
#pragma endregion

#pragma region Scene

Scene::Scene(bool withoutDefaultSystems){
    //LogInfo("NewScene");

    if(withoutDefaultSystems == true) return;
    
    for(auto i: SceneManager::Get().addSystemFuncs){
        //LogInfo("Adding system: %s", i.first);
        //i.second(*this);
        i(*this);
    }
}

Scene::Scene(Scene& other){
    for(auto& i: other.systemsAdd){
        //i.second(*this);
        i(*this);
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
    //registry.clear();
    /*for(System* i: standSystems) delete i;
    for(System* i: rendererSystems) delete i;
    for(System* i: physicsSystems) delete i;*/

    for(auto& i: systems) delete i.second;

    registry.clear();//INFO: Maybe this order fix same crashs
    
    systems.clear();
    standSystems.clear();
    animationSystems.clear();
    rendererSystems.clear();
    physicsSystems.clear();
    lateSystems.clear();
}

/*Entity Scene::AddEntity(std::string name){
    EntityId e = registry.create();
    
    InfoComponent& info = registry.emplace<InfoComponent>(e);
    info.name = name;
    info.id = e;
    
    TransformComponent& transform = registry.emplace<TransformComponent>(e);
    transform.registry = &registry;

    Assert(registry.valid(e));
    Assert(registry.any_of<TransformComponent>(e));

    return Entity(e, this);
}*/

Entity Scene::AddEntity(const std::string& name){
    Entity e = registry.create();
    
    InfoComponent& info = registry.emplace<InfoComponent>(e);
    info.name = name;
    //info.id = e;
    
    TransformComponent& transform = registry.emplace<TransformComponent>(e);
    transform.registry = &registry;

    Assert(registry.valid(e));
    Assert(registry.any_of<TransformComponent>(e));

    return e;
}

Entity Scene::_DuplicateEntity(Entity e, bool isRoot){
    Assert(IsValid(e) == true);

    TransformComponent& trans = registry.get<TransformComponent>(e);

    Entity other = registry.create();
    
    auto& t = registry.emplace_or_replace<TransformComponent>(other, trans);
    t.children.clear();

    if(isRoot && t.HasParent()){
        SetParent(t.parent, other);
    }

    registry.emplace_or_replace<InfoComponent>(other, registry.get<InfoComponent>(e));
    
    for(auto i: SceneManager::Get().coreComponentsSerializer){
        if(i.second.hasComponent(e, *this)) i.second.copyComponent(e, other, *this, *this);
    }
    for(auto i: SceneManager::Get().componentsSerializer){
        if(i.second.hasComponent(e, *this)) i.second.copyComponent(e, other, *this, *this);
    }

    for(auto i: trans.children){
        auto ne = _DuplicateEntity(i, false);
        SetParent(other, ne);
    }

    return other;
}

Entity Scene::_DuplicateEntity(Entity e, bool isRoot, Scene& source){
    Assert(source.IsValid(e) == true);

    TransformComponent& trans = source.registry.get<TransformComponent>(e);

    Entity other = registry.create();
    
    auto& t = registry.emplace_or_replace<TransformComponent>(other, trans);
    t.registry = &registry;
    t.children.clear();

    if(isRoot && t.HasParent()){
        //SetParent(t.parent, other);
        t.hasParent = false;
        t.parent = EntityNull;
    }

    registry.emplace_or_replace<InfoComponent>(other, source.registry.get<InfoComponent>(e));
    
    for(auto i: SceneManager::Get().coreComponentsSerializer){
        if(i.second.hasComponent(e, source)) i.second.copyComponent(e, other, source, *this);
    }
    for(auto i: SceneManager::Get().componentsSerializer){
        if(i.second.hasComponent(e, source)) i.second.copyComponent(e, other, source, *this);
    }

    for(auto i: trans.children){
        auto ne = _DuplicateEntity(i, false, source);
        SetParent(other, ne);
    }

    return other;
}

Entity Scene::DuplicateEntity(Entity e){
    return _DuplicateEntity(e, true);
    /*Assert(IsValid(e) == true);

    TransformComponent& trans = registry.get<TransformComponent>(e);

    Entity other = registry.create();
    
    auto& t = registry.emplace_or_replace<TransformComponent>(other, trans);
    t.children.clear();

    if(t.HasParent()){
        SetParent(t.parent, other);
    }

    registry.emplace_or_replace<InfoComponent>(other, registry.get<InfoComponent>(e));
    
    for(auto i: SceneManager::Get().coreComponentsSerializer){
        if(i.second.hasComponent(e, *this)) i.second.copyComponent(e, other, *this);
    }
    for(auto i: SceneManager::Get().componentsSerializer){
        if(i.second.hasComponent(e, *this)) i.second.copyComponent(e, other, *this);
    }

    for(auto i: trans.children){
        auto ne = DuplicateEntity(i);
        SetParent(other, ne);
    }

    return other;
    */
}

Entity Scene::InstantiatePrefab(const Prefab& prefab){
    return _DuplicateEntity(prefab.root, true, *prefab.scene);
}

void Scene::DestroyEntity(Entity entity){
    //if(registry.valid(entity) == false) return;
    //_DestroyEntity(entity);
    toDestroy.push_back(entity);
}

void Scene::DestroyEntityImmediate(Entity entity){
    if(registry.valid(entity) == false) return;
    _DestroyEntity(entity, true);
}

bool Scene::IsChildOf(Entity parent, Entity child){
    TransformComponent& _parent = registry.get<TransformComponent>(parent);

    for(auto i: _parent.children){
        if(i == child) return true;
        bool r = IsChildOf(i, child);
        if(r == true) return true;
    }

    return false;
}

void Scene::CleanParent(Entity e){
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

void Scene::SetParent(Entity parent, Entity child){
    if(parent == child){
        LogWarning("ERROR: Trying set parent with itself");
        return;
    }

    Assert(registry.valid(parent));
    Assert(registry.valid(child));
    Assert(registry.any_of<TransformComponent>(parent));
    Assert(registry.any_of<TransformComponent>(child));

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

    _parent.SetGlobalAsDirty();
    _child.SetGlobalAsDirty();
}

bool Scene::IsValid(Entity id){
    return registry.valid(id); 
}

Entity Scene::Instantiate(const Ref<Model> model, bool staticRenderer, int overrideLayer){
    if(model == nullptr){
        LogWarning("Trying instantiate a null model");
        return Entity();
    }

    Entity root = AddEntity("Root");

    for(auto i: model->renderTargets){
        Entity mesh = AddEntity(model->skeleton.GetJointName(i.bindPoseIndex));
        auto& meshRenderer = AddComponent<MeshRendererComponent>(mesh);
        auto& transform = GetComponent<TransformComponent>(mesh);
        if(staticRenderer) AddComponent<StaticRendererComponent>(mesh);

        if(overrideLayer != LayerNone && overrideLayer != LayerMax){
            auto& info = GetComponent<InfoComponent>(mesh);
            info.layer = (Layers)overrideLayer;
        }

        meshRenderer.material = model->materials[i.materialIndex];
        meshRenderer.mesh = model->meshs[i.meshIndex];
        auto targetMatrix = Transform(model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex));

        transform.LocalPosition(targetMatrix.LocalPosition());
        transform.LocalRotation(targetMatrix.LocalRotation());
        transform.LocalScale(targetMatrix.LocalScale());
        meshRenderer.UpdateAABB();

        SetParent(root, mesh);
    }

    return root;
}

/*Camera& Scene::GetMainCamera(){ 
    return mainCamera; 
}*/   

Entity Scene::GetMainCamera(){
    auto camView = registry.view<CameraComponent>(entt::exclude<SelfDisable>);
    for(auto e: camView){
        CameraComponent& cam = camView.get<CameraComponent>(e);
        if(cam.isMain == false) continue;
        return e; //Entity(e, this);
    }
    return EntityNull; //Entity();
}

Entity Scene::FindEntityByName(const std::string& name){
    auto infoView = registry.view<InfoComponent>();
    for(auto e: infoView){
        InfoComponent& info = infoView.get<InfoComponent>(e);
        if(info.name == name){
            return e;
        }
    }

    return EntityNull;
}

void Scene::Start(){
    Time::TimeScale(1);
    running = true;
}

void Scene::Update(){ 
    OD_PROFILE_SCOPE("Scene::Update");
    
    //INFO: Experimental Try Catch
    try{

    for(auto e: toDestroy){
        _DestroyEntity(e, true);
    }
    toDestroy.clear();

    //if(_running == false) return;

    /*for(auto s: physicsSystems) s->PhysicsUpdate();
    {
        OD_PROFILE_SCOPE("Scene::PhysicsUpdate::Sync");
        executor.run(taskflow).wait(); 
        taskflow.clear();
    }*/

    //if(running == false) return;
    for(auto s: standSystems){
        if(running == false && s->ExecuteAlways() == false) continue;
        s->Update();
    }
    {
        OD_PROFILE_SCOPE("Scene::Update::Sync");
        executor.run(taskflow).wait();
        taskflow.clear();
    }

    for(auto s: animationSystems){
        if(running == false && s->ExecuteAlways() == false) continue;
        s->AnimationUpdate();
    }
    {
        OD_PROFILE_SCOPE("Scene::AnimationUpdate::Sync");
        executor.run(taskflow).wait(); 
        taskflow.clear();
    }

    for(auto s: physicsSystems) s->PhysicsUpdate();
    {
        OD_PROFILE_SCOPE("Scene::PhysicsUpdate::Sync");
        executor.run(taskflow).wait(); 
        taskflow.clear();
    }

    for(auto s: lateSystems){
        if(running == false && s->ExecuteAlways() == false) continue;
        s->LateUpdate();
    }
    {
        OD_PROFILE_SCOPE("Scene::LateUpdate::Sync");
        executor.run(taskflow).wait();
        taskflow.clear();
    }

    }catch(...){
        Assert(false && "Scene::Update Catch Error"); //TODO: Make the scene stop and the Editor handle this too.
    }
}

void Scene::Draw(){
    OD_PROFILE_SCOPE("Scene::Draw");

    Graphics::Begin();
    for(auto& s: rendererSystems) s->Render();        
    Graphics::End();
}

void Scene::_AddEntityPrefab(entt::registry& registry, std::vector<entt::entity>& entities, std::vector<entt::entity>& allEntities, entt::entity entity, std::string prefabPath, bool isRoot){
    //entities.push_back(entity);

    //TODO: Revisar DontSave if a entity has not "DontSave" but the parent has
    if(registry.any_of<DontSave>(entity)) return;

    InfoComponent& infoComponent = registry.get<InfoComponent>(entity);

    if(isRoot == false && infoComponent.entityType == EntityType::PrefabRoot){
        allEntities.push_back(entity);
        return;
    }
    entities.push_back(entity);
    allEntities.push_back(entity);

    infoComponent.entityType = isRoot ? EntityType::PrefabRoot : EntityType::PrefabChild;
    infoComponent.prefabPath = prefabPath;

    TransformComponent& trans = registry.get<TransformComponent>(entity);
    for(auto e: trans.children){
        _AddEntityPrefab(registry, entities, allEntities, e, std::string(""));
    }
}

void Scene::Save(const char* path, Entity root){
    std::ofstream os(path);
    ODOutputArchive archive(os);

    std::vector<Entity> entities;
    std::vector<Entity> entitiesAll;

    /*auto entityView = registry.view<entt::entity>();
    std::vector<entt::entity> entities(entityView.begin(), entityView.end()); //std::vector<entt::entity> entities(entityView.rbegin(), entityView.rend());
    archive(cereal::make_nvp("Entities", entities));*/

    if(root == EntityNull){
        registry.sort<InfoComponent>([](const Entity lhs, const Entity rhs){
            return lhs < rhs;
        });

        //TODO: Revisar DontSave if a entity has not "DontSave" but the parent has
        auto entityView = registry.view<TransformComponent, InfoComponent>(entt::exclude<DontSave>);
        entityView.use<InfoComponent>();
        for(auto e: entityView){
            InfoComponent& infoComponent = registry.get<InfoComponent>(e);
            TransformComponent& transComponent = registry.get<TransformComponent>(e);
            if(infoComponent.entityType == EntityType::Stand){
                entities.push_back(e);
                entitiesAll.push_back(e);
            }
            if(infoComponent.entityType == EntityType::PrefabRoot){
                if(transComponent.hasParent){
                    InfoComponent& pp = registry.get<InfoComponent>(transComponent.parent);
                    if(pp.entityType == EntityType::Stand){
                        entitiesAll.push_back(e);
                    }
                } else {    
                    entitiesAll.push_back(e);
                }
            }
        }
    } else {
        //Assert(false);
        _AddEntityPrefab(registry, entities, entitiesAll, root, path, true);
        //entitiesAll = std::vector<Entity>(entities.begin(), entities.end());
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
        if(i == entities[0]) root = loadLookup[i]; //Entity(loadLookup[i], this);
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

    TransformComponent& rootTrans = GetComponent<TransformComponent>(root);
    rootTrans.parent = EntityNull;
    rootTrans.hasParent = false;

    return root;
}

void Scene::_Unpack(Entity e, bool all, bool unpackRoot){
    InfoComponent& info = GetComponent<InfoComponent>(e);
    TransformComponent& trans = GetComponent<TransformComponent>(e);

    if(info.entityType == EntityType::PrefabRoot && unpackRoot == true){
        info.entityType = EntityType::Stand;
    } else if(info.entityType == EntityType::PrefabChild){
        info.entityType = EntityType::Stand;
    }

    for(auto& child: trans.children){
        InfoComponent& chInfo = GetComponent<InfoComponent>(child);
        if(chInfo.entityType == EntityType::PrefabChild){
            _Unpack(child, all, true);
        } else if(chInfo.entityType == EntityType::PrefabRoot && all == true){
            _Unpack(child, all, true);
        } else {
            _Unpack(child, all, false);
        }
    }
}

void Scene::UnpackPrefab(Entity entity, bool all){
    _Unpack(entity, all, true);
}

void Scene::_DestroyEntity(Entity entity, bool removeFromParent){
    if(registry.valid(entity) == false) return;

    Assert(registry.valid(entity));
    Assert(registry.any_of<TransformComponent>(entity));
    TransformComponent& transform = registry.get<TransformComponent>(entity);

    for(auto i: transform.children){
        _DestroyEntity(i);
    }
    transform.children.clear();

    
    if(transform.hasParent && removeFromParent){
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

void Scene::CreateLuaBind(sol::state& lua){
    using namespace entt::literals;
    //lua["EntityNull"] = [](){ return 10; };
    lua.new_usertype<Scene>(
        "Scene",
        "Running", &Scene::Running,
        "AddEntity", &Scene::AddEntity,
        "DestroyEntity", &Scene::DestroyEntity,
        "DestroyEntityImmediate", &Scene::DestroyEntityImmediate,
        "IsChildOf", &Scene::IsChildOf, 
        "CleanParent", &Scene::CleanParent,
        "SetParent", &Scene::SetParent, //[](Scene& s, Entity parent, Entity child){ s.SetParent(parent, child); },
        "GetInfoComponent", &Scene::GetComponent<InfoComponent>,
        "GetTransformComponent", &Scene::GetComponent<TransformComponent>,
        "AddComponent", [](Scene& scene, Entity e, const sol::table& comp, sol::this_state s) -> sol::object{
            if(!comp.valid()) return sol::lua_nil_t{};
            const auto component = InvokeMetaFunction(GetIdType(comp), "_AddComponent"_hs, &scene, e, comp, s);
            return component ? component.cast<sol::reference>() : sol::lua_nil_t{};
        },
        "HasComponent", [](Scene& scene, Entity e, const sol::table& comp){
            const auto hasComp = InvokeMetaFunction(GetIdType(comp), "_HasComponent"_hs, &scene, e);
            return hasComp ? hasComp.cast<bool>() : false;
        },
        "GetComponent", [](Scene& scene, Entity e, const sol::table& comp, sol::this_state s){
            const auto component = InvokeMetaFunction(GetIdType(comp), "_GetComponent"_hs, &scene, e, s);
            return component ? component.cast<sol::reference>() : sol::lua_nil_t{};
        },
        "RemoveComponent", [](Scene& scene, Entity e, const sol::table& comp){
            InvokeMetaFunction(GetIdType(comp), "_RemoveComponent"_hs, &scene, e);
        },
        "IsValid", &Scene::IsValid
    );
}

void EntityHandle::CreateLuaBind(sol::state& lua){
    using namespace entt::literals;
    lua.new_usertype<EntityHandle>(
        "EntityHandle",
        "GetInfoComponent", &EntityHandle::GetComponent<InfoComponent>,
        "GetTransformComponent", &EntityHandle::GetComponent<TransformComponent>,
        "AddComponent", [](EntityHandle& e, const sol::table& comp, sol::this_state s) -> sol::object{
            if(!comp.valid()) return sol::lua_nil_t{};
            const auto component = InvokeMetaFunction(GetIdType(comp), "_AddComponent"_hs, e.scene, e.entity, comp, s);
            return component ? component.cast<sol::reference>() : sol::lua_nil_t{};
        },
        "HasComponent", [](EntityHandle& e, const sol::table& comp){
            const auto hasComp = InvokeMetaFunction(GetIdType(comp), "_HasComponent"_hs, e.scene, e.entity);
            return hasComp ? hasComp.cast<bool>() : false;
        },
        "GetComponent", [](EntityHandle& e, const sol::table& comp, sol::this_state s){
            const auto component = InvokeMetaFunction(GetIdType(comp), "_GetComponent"_hs, e.scene, e.entity, s);
            return component ? component.cast<sol::reference>() : sol::lua_nil_t{};
        },
        "RemoveComponent", [](EntityHandle& e, const sol::table& comp){
            InvokeMetaFunction(GetIdType(comp), "_RemoveComponent"_hs, e.scene, e.entity);
        },
        "GetScene", &EntityHandle::GetScene,
        "GetEntity", &EntityHandle::GetEntity,
        "IsValid", &EntityHandle::IsValid
    );
}

#pragma endregion

Prefab::Prefab(){

}

void Prefab::OnGui(){
    if(path.empty() == false || path != "Memory"){
		auto* editor = Application::GetModuleByType<Editor>();
		auto* framebuffer = editor->AssetPreviewFramebuffer();
		editor->SetPrefabAssetPreview(path);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

		float aspect = framebuffer->Width() / framebuffer->Height();
		ImGui::Image(framebuffer->ColorAttachmentId(0), ImVec2(viewportPanelSize.x, viewportPanelSize.x * aspect), ImVec2(0, 1), ImVec2(1, 0));
	} else {
		ImGui::Text("Can not preview this model!!!");
	}
}

bool Prefab::LoadFromFile(const std::string& inpath){
    path = inpath;
    if(scene != nullptr) delete scene;

    scene = new Scene(true);
    root = scene->InstantiatePrefab(path.c_str());

    return true;
}

std::vector<std::string> Prefab::GetFileAssociations(){
    return {".prefab"};
}

}