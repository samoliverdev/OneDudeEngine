#include "OD/pch.h"
#include "Scene.h"
#include "SceneMeta.h"
#include "Prefab.h"
#include "EntityHandle.h"
#include "SceneManager.h"
#include "Scripts.h"
#include "OD/Core/Lua.h"
#include "OD/Core/ImGui.h"
#include "OD/Core/Time.h"
#include "OD/Core/Instrumentor.h"
#include "OD/Core/ResourceStream.h"
//#include "OD/Serialization/SerializationFull.h"
#include "OD/Serialization/CerealImGui.h"
#include "OD/Graphics/Graphics.h"
#include "OD/RenderPipeline/CameraComponent.h"
#include "OD/RenderPipeline/MeshRendererComponent.h"
#include "OD/RenderPipeline/ModelRendererComponent.h"
#include "OD/LuaScripting/LuaMetaUltis.h"
#include "OD/Core/GlobalSettings.h"

namespace OD{

tf::Executor* executor = nullptr;
//tf::Taskflow taskflow;
GlobalSceneData globalSceneData;

void SceneModuleInit(){
    executor = new tf::Executor(); //TODO: call delete on executor
}

int LayerMask::GetLayerByName(const std::string& name){
    Assert(globalSceneData.layerNames.size() == Layers::LayerCount);

    for(int i = 0; i < globalSceneData.layerNames.size(); i++){
        if(globalSceneData.layerNames[i] == name) return i;
    }
    return LayerNone;
}

GlobalSceneData& GetGlobalSceneData(){
    return GlobalSettings::Get().Get<GlobalSceneData>();  //globalSceneData;
}

void GlobalSceneData::OnImGuiRender(){
    /*cereal::ImGuiArchive ar;
    ar(*this);*/

    for(int i = 0; i < LayerCount; i++){
        std::string id = "##Layer" + std::to_string(i);
        ImGui::DrawString(id.c_str(), layerNames[i]);
    }
}

#pragma region TransformComponent

void TransformComponent::SetGlobalAsDirty(){
    #ifdef ExperimentalTransformOptimzation
    Assert(false);
    globalIsDirty = true;
    for(auto& i: children){
        TransformComponent& t = registry->get<TransformComponent>(i);
        t.SetGlobalAsDirty();
    }
    #endif
}

void TransformComponent::UpdateAllTransformMatrix(Scene& scene){
    #ifdef ExperimentalTransformOptimzation
    ForEachWithTransformTaskflow(scene, [](Entity entity, TransformComponent& t) {
        t.UpdateGlobalTransformCacheIfNeeded(false);
    });

    /*ForEachWithTransformTaskflow<InfoComponent>(scene, [](Entity e, TransformComponent& t, InfoComponent& a){
        t.UpdateGlobalTransformCacheIfNeeded(false);
    });*/

    return;

    std::unordered_map<Entity, tf::Task> entityTasks;

    // First pass: create one task per entity
    auto view = scene.GetRegistry().view<TransformComponent>();
    for(auto entity : view){
        tf::Task task = scene.GetTaskflow().emplace([&, entity](){
            TransformComponent& t = view.get<TransformComponent>(entity);
            t.UpdateGlobalTransformCacheIfNeeded(false);
        }).name("UpdateTransform");

        entityTasks[entity] = task;
    }

    // Second pass: set up dependencies
    for(auto [entity, task] : entityTasks){
        TransformComponent& t = scene.GetComponent<TransformComponent>(entity);
        if(t.HasParent()){
            TransformComponent& p = scene.GetComponent<TransformComponent>(t.Parent());
            if(p.isCollection) continue;

            Entity parent = t.Parent();
            if(auto it = entityTasks.find(parent); it != entityTasks.end()){
                it->second.precede(task);  // parent -> child
            }
        }
    }

    scene.RunAllTaskAndSync();
    #endif
}

void TransformComponent::UpdateGlobalTransformCacheIfNeeded(bool updateChild){
    #ifdef ExperimentalTransformOptimzation
    //if(globalIsDirty){
        //globalIsDirty = false;
        if(hasParent && isCollection == false){
            Assert(registry->any_of<TransformComponent>(parent) == true);
            TransformComponent& p = registry->get<TransformComponent>(parent);
            //p.UpdateGlobalTransformCacheIfNeeded();
            #ifdef TransformLessDataOptimzation
            localModelMatrix = localTransform.GetModelMatrix();
            //globalModelMatrix = /*p.GlobalModelMatrix() **/ p.globalModelMatrix * localModelMatrix;
            globalModelMatrix = math::simdMul(p.globalModelMatrix, localModelMatrix);
            globalTransform.position = p.TransformPoint(LocalPosition());
            globalTransform.rotation = p.Rotation() * LocalRotation();
            globalTransform.scale = p.Scale() * LocalScale(); // Aqui está a escala acumulada
            #else
            globalTransform.modelMatrix = /*p.GlobalModelMatrix() **/ p.globalTransform.GetModelMatrix() * localTransform.GetModelMatrix();
            globalTransform.position = p.TransformPoint(LocalPosition());
            globalTransform.rotation = p.Rotation() * LocalRotation();
            globalTransform.scale = p.Scale() * LocalScale(); // Aqui está a escala acumulada
            #endif
        } else {
            #ifdef TransformLessDataOptimzation
            localModelMatrix = isCollection ? Matrix4Identity : localTransform.GetModelMatrix();
            globalModelMatrix = isCollection ? Matrix4Identity : localModelMatrix;
            globalTransform.position = isCollection ? Vector3Zero : LocalPosition();
            globalTransform.rotation = isCollection ? QuaternionIdentity : LocalRotation();
            globalTransform.scale = isCollection ? Vector3One : LocalScale(); // sem pai, usa local diretamente
            #else
            globalTransform.modelMatrix = isCollection ? Matrix4Identity : localTransform.GetModelMatrix();
            globalTransform.position = isCollection ? Vector3Zero : LocalPosition();
            globalTransform.rotation = isCollection ? QuaternionIdentity : LocalRotation();
            globalTransform.scale = isCollection ? Vector3One : LocalScale(); // sem pai, usa local diretamente
            #endif
        }

        if(updateChild == false) return;

        for(auto& i: children){
            registry->get<TransformComponent>(i).UpdateGlobalTransformCacheIfNeeded();
        }
    //}
    #endif
}

const Matrix4& TransformComponent::GlobalModelMatrixReadSafe() const{
    #ifdef ExperimentalTransformOptimzation
        #ifdef TransformLessDataOptimzation
        return globalModelMatrix;
        #else
        return globalTransform.modelMatrix;
        #endif
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
    //UpdateGlobalTransformCacheIfNeeded();
    if(isCollection) return Matrix4Identity;
    
        #ifdef TransformLessDataOptimzation
        return globalModelMatrix;
        #else
        return globalTransform.modelMatrix;
        #endif

    #else

    if(hasParent){
        TransformComponent& p = registry->get<TransformComponent>(parent);
        return p.GlobalModelMatrix() * localTransform.GetModelMatrix();
    }
    return localTransform.GetModelMatrix();
    #endif
}

Matrix4 TransformComponent::GetLocalModelMatrix(){ 
    #ifdef TransformLessDataOptimzation
    return localModelMatrix;
    #else
    return localTransform.GetModelMatrix(); 
    #endif
}

Vector3 TransformComponent::InverseTransformDirection(Vector3 dir){
    //TODO: change to Quaternion rot = Rotation(); to match unity, and make this function InverseTransformVector, the same for TransformDirection
    Matrix4 matrix4 = GlobalModelMatrix();
    
    return math::inverse(matrix4) * Vector4(dir.x, dir.y, dir.z, 0);
    //return math::simdMul(math::inverse(matrix4), Vector4(dir.x, dir.y, dir.z, 0));
}

Vector3 TransformComponent::TransformDirection(Vector3 dir){
    Matrix4 matrix4 = GlobalModelMatrix();

    return matrix4 * Vector4(dir.x, dir.y, dir.z, 0);
    //return math::simdMul(matrix4, Vector4(dir.x, dir.y, dir.z, 0));
}

Vector3 TransformComponent::InverseTransformPoint(Vector3 point){
    Matrix4 matrix4 = GlobalModelMatrix();

    return math::inverse(matrix4) * Vector4(point.x, point.y, point.z, 1);
    //return math::simdMul(math::inverse(matrix4), Vector4(point.x, point.y, point.z, 1));
}

Vector3 TransformComponent::TransformPoint(Vector3 point){
    Matrix4 matrix4 = GlobalModelMatrix();

    return matrix4 * Vector4(point.x, point.y, point.z, 1);
    //return math::simdMul(matrix4, Vector4(point.x, point.y, point.z, 1));
}

//Quaternion InverseTransformRot(Quaternion world, Quaternion rot){
//    return Quaternion::Inverse(world) * rot;
//}

const Vector3& TransformComponent::PositionReadSafe() const{
    #ifdef ExperimentalTransformOptimzation
    return globalTransform.position;
    #else 
    Assert(false);
    return Vector3Zero;
    #endif
}

Vector3 TransformComponent::Position(){ 
    #ifdef ExperimentalTransformOptimzation
    if(isCollection) return Vector3Zero;
    /*if(globalIsDirty){
        globalIsDirty = false;
        if(hasParent){
            TransformComponent& p = registry->get<TransformComponent>(parent);
            globalTransform.localPosition = p.TransformPoint(LocalPosition());
        } else {
            globalTransform.localPosition = LocalPosition();
        }
    }*/
    //UpdateGlobalTransformCacheIfNeeded();
    return globalTransform.position;

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
    //SetGlobalAsDirty();
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
    if(isCollection) return QuaternionIdentity;
    /*if(globalIsDirty){
        globalIsDirty = false;
        if(hasParent){
            TransformComponent& p = registry->get<TransformComponent>(parent);
            globalTransform.localRotation = p.Rotation() * LocalRotation();
        } else {
            globalTransform.localRotation = LocalRotation();
        }
    }*/
    //UpdateGlobalTransformCacheIfNeeded();
    return globalTransform.rotation;

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
    //SetGlobalAsDirty();
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
    if(isCollection) return Vector3One;
    
    //UpdateGlobalTransformCacheIfNeeded();
    return globalTransform.scale;
    
    #else

    Transform t(GlobalModelMatrix());
    return t.Scale();
    #endif
}

Vector3 TransformComponent::LocalPosition(){ 
    #ifdef ExperimentalTransformOptimzation
    if(isCollection) return Vector3Zero;
    #endif
    return localTransform.Position(); 
}

void TransformComponent::LocalPosition(Vector3 pos){ 
    localTransform.Position(pos); 
    #ifdef ExperimentalTransformOptimzation
    //SetGlobalAsDirty();
    UpdateGlobalTransformCacheIfNeeded();
    #endif
}

Vector3 TransformComponent::LocalEulerAngles(){ 
    #ifdef ExperimentalTransformOptimzation
    if(isCollection) return Vector3Zero;
    #endif

    #ifdef TransformLessDataOptimzation
    if(localEulerAnglesIsDirt){
        localEulerAnglesIsDirt = false;
        localEulerAngles = Mathf::Rad2Deg(math::eulerAngles(localTransform.rotation));
    }
    return localEulerAngles;
    #else
    return localTransform.EulerAngles(); 
    #endif
}

void TransformComponent::LocalEulerAngles(Vector3 euler){ 
    #ifdef TransformLessDataOptimzation
    localEulerAngles = euler;
    localTransform.rotation = Quaternion(Mathf::Deg2Rad(localEulerAngles));
    localEulerAnglesIsDirt = false;
    #else
    localTransform.EulerAngles(euler); 
    #endif

    #ifdef ExperimentalTransformOptimzation
    //SetGlobalAsDirty();
    UpdateGlobalTransformCacheIfNeeded();
    #endif
}

Quaternion TransformComponent::LocalRotation(){ 
    #ifdef ExperimentalTransformOptimzation
    if(isCollection) return QuaternionIdentity;
    #endif

    return localTransform.Rotation(); 
}

void TransformComponent::LocalRotation(Quaternion rot){ 
    #ifdef TransformLessDataOptimzation
    localEulerAngles = Mathf::Rad2Deg(math::eulerAngles(rot));
    localEulerAnglesIsDirt = false;
    #endif

    localTransform.Rotation(rot); 

    #ifdef ExperimentalTransformOptimzation
    //SetGlobalAsDirty();
    UpdateGlobalTransformCacheIfNeeded();
    #endif
}

Vector3 TransformComponent::LocalScale(){ 
    #ifdef ExperimentalTransformOptimzation
    if(isCollection) return Vector3One;
    #endif
    return localTransform.Scale(); 
}

void TransformComponent::LocalScale(Vector3 scale){
    localTransform.Scale(scale); 
    #ifdef ExperimentalTransformOptimzation
    //SetGlobalAsDirty();
    UpdateGlobalTransformCacheIfNeeded();
    #endif
}

void TransformComponent::SetLocalModelMatrix(Matrix4 matrix){ 
    #ifdef ExperimentalTransformOptimzation
    Assert(false && "Outdate");
    #endif
    localTransform = Transform(matrix); 
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

bool TransformComponent::MoveChildUp(Entity child){
    auto it = std::find(children.begin(), children.end(), child);
    if(it == children.end()) return false;

    size_t index = std::distance(children.begin(), it);

    if(index == 0) return false; // already first 

    std::swap(children[index], children[index - 1]);
    return true;
}

bool TransformComponent::MoveChildDown(Entity child){
    auto it = std::find(children.begin(), children.end(), child);
    if(it == children.end()) return false;

    size_t index = std::distance(children.begin(), it);

    if(index >= children.size() - 1) return false; // already last 

    std::swap(children[index], children[index + 1]);
    return true;
}

TransformComponent::operator Transform() {
    /*#ifdef ExperimentalTransformOptimzation
    return Transform(Position(), Rotation(), Scale()); 
    #else*/
    return Transform(GlobalModelMatrix());
    //#endif
}

Transform TransformComponent::ToTransform(){ 
    /*#ifdef ExperimentalTransformOptimzation
    return Transform(Position(), Rotation(), LocalScale()); 
    #else*/
    return Transform(Position(), Rotation(), Scale());
    //return Transform(GlobalModelMatrix()); 
    //#endif
}

void TransformComponent::CreateLuaBind(sol::state& lua){
    SceneMeta::RegisterMetaComponent<TransformComponent>();
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
    SceneMeta::RegisterMetaComponent<InfoComponent>();
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
    for(auto& i: SceneManager::Get().globalSystems){
        i.second->OnInit(*this);
    }
}

Scene::Scene(Scene& other){
    for(auto& i: other.systemsAdd){
        //i.second(*this);
        i(*this);
    }
    for(auto& i: SceneManager::Get().globalSystems){
        i.second->OnInit(*this);
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
    if(isPrefab) return;

    //TODO: Maybe mov
    if(taskflow.empty() == false){
        executor->wait_for_all();
        taskflow.clear();
    }

    registry.clear();//INFO: Maybe this order fix same crashs

    //TODO: Maybe move this to scene manager, becose this will call even for no stand scene usage, like in prefab and scene preview in editor, or other possible use of scene out of scene manager
    for(auto& i: SceneManager::Get().globalSystems){
        i.second->OnEnd(*this);
    }
    for(auto& i: systems){
        i.second->OnEnd(*this);
    }
    //

    //registry.clear();//INFO: Maybe this order fix same crashs

    //Delete Later call all OnEnd
    for(auto& i: systems){
        delete i.second;
    }

    //registry.clear();//INFO: Maybe this order fix same crashs
    
    systems.clear();
    standSystems.clear();
    animationSystems.clear();
    rendererSystems.clear();
    prePhysicsSystems.clear();
    fixedPhysicsSystems.clear();
    postPhysicsSystems.clear();
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

    registry.get<TransformComponent>(other).UpdateGlobalTransformCacheIfNeeded();
    return other;
}

Entity Scene::_DuplicateEntity(Entity e, bool isRoot, Scene& source){
    Assert(source.IsValid(e) == true);

    TransformComponent& trans = source.registry.get<TransformComponent>(e);

    Entity other = registry.create();
    
    auto& t = registry.emplace_or_replace<TransformComponent>(other, trans);
    t.registry = &registry;
    t.children.clear();

    //if(isRoot && t.HasParent()){
        //SetParent(t.parent, other);
        t.hasParent = false;
        t.parent = EntityNull;
    //}

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

    registry.get<TransformComponent>(other).UpdateGlobalTransformCacheIfNeeded();
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

    //_parent.SetGlobalAsDirty();
    //_child.SetGlobalAsDirty();
    _child.UpdateGlobalTransformCacheIfNeeded();
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

        if(overrideLayer != LayerNone && overrideLayer != Layers::LayerCount){
            auto& info = GetComponent<InfoComponent>(mesh);
            info.layer = (Layers)overrideLayer;
        }

        meshRenderer.material = model->materials[i.materialIndex];
        meshRenderer.mesh = model->meshs[i.meshIndex];
        auto targetMatrix = Transform(model->skeleton.GetBindPose().GetGlobalMatrix(i.bindPoseIndex));

        transform.LocalPosition(targetMatrix.Position());
        transform.LocalRotation(targetMatrix.Rotation());
        transform.LocalScale(targetMatrix.Scale());
        meshRenderer.UpdateAABB();

        SetParent(root, mesh);
    }

    TransformComponent& rootTrans = GetComponent<TransformComponent>(root);
    rootTrans.UpdateGlobalTransformCacheIfNeeded();

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

Entity Scene::FindEntityByNameInChildren(Entity parent, const std::string& name){
    TransformComponent& trans = GetComponent<TransformComponent>(parent);
    for(auto i: trans.Children()){
        InfoComponent& info = GetComponent<InfoComponent>(i);
        if(info.name == name){
            return i;
        } 

        TransformComponent& child = GetComponent<TransformComponent>(i);
        if(child.Children().size() > 0){
            Entity r = FindEntityByNameInChildren(i, name);
            if(r != EntityNull) return r;
        }
    }
    return EntityNull;
}

void Scene::Start(){
    if(running) return;

    Time::TimeScale(1);
    running = true;

    for(auto& i: SceneManager::Get().globalSystems){
        i.second->OnStart(*this);
    }
    for(auto& i: systems){
        i.second->OnStart(*this);
    }
}

void Scene::Stop(){
    if(running == false) return;

    running = false;

    for(auto& i: systems){
        i.second->OnStop(*this);
    }
    for(auto& i: SceneManager::Get().globalSystems){
        i.second->OnStop(*this);
    }
}

void Scene::Update(){ 
    OD_PROFILE_SCOPE("Scene::Update");

    //INFO: Experimental Try Catch
    //try{

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

    /*if(transIsDirty){
        transIsDirty = false;
        TransformComponent::UpdateAllTransformMatrix(*this);
    }*/

    //if(running == false) return;
    
    //--------Stand---------
    {
        OD_PROFILE_SCOPE("Scene::Update");
        for(auto s: standSystems){
            if(running == false && s->ExecuteAlways() == false) continue;
            s->Update(*this);
        }
        for(auto& s: SceneManager::Get().globalStandSystems){
            if(running == false && s->ExecuteAlways() == false) continue;
            s->Update(*this);
        }
    }
    {
        OD_PROFILE_SCOPE("Scene::Update::Sync");
        executor->run(taskflow).wait();
        taskflow.clear();
    }

    //--------Animation--------
    {
        OD_PROFILE_SCOPE("Scene::AnimationUpdate");
        for(auto s: animationSystems){
            if(running == false && s->ExecuteAlways() == false) continue;
            s->AnimationUpdate(*this);
        }
        for(auto& s: SceneManager::Get().globalAnimationSystems){
            if(running == false && s->ExecuteAlways() == false) continue;
            s->AnimationUpdate(*this);
        }
    }
    {
        OD_PROFILE_SCOPE("Scene::AnimationUpdate::Sync");
        executor->run(taskflow).wait(); 
        taskflow.clear();
    }

    //--------Pre Physic---------
    {
        OD_PROFILE_SCOPE("Scene::PrePhysicsUpdate");
        for(auto s: prePhysicsSystems) s->PrePhysicsUpdate(*this);
        for(auto s: SceneManager::Get().globalPrePhysicsSystems) s->PrePhysicsUpdate(*this);
    }
    {
        OD_PROFILE_SCOPE("Scene::PrePhysicsUpdate::Sync");
        executor->run(taskflow).wait(); 
        taskflow.clear();
    }

    //--------Fixed Physic---------
    float _delta = OD::Time::DeltaTime();
    float _fixedStep = OD::Time::FixedDelta();

    fixedUpdateAccumulator += _delta;
    
    #if ENABLE_FIXED
    {
    OD_PROFILE_SCOPE("Scene::FixedPhysicsUpdate");
    if(_fixedStep > 0.0f && _delta >= 0.0f){
        while(fixedUpdateAccumulator >= _fixedStep){ //INFO: This can be bug if Time::FixedDelta() return 0 
            for(auto s: SceneManager::Get().globalFixedPhysicsSystems) s->FixedPhysicsUpdate(*this);
            for(auto s: fixedPhysicsSystems) s->FixedPhysicsUpdate(*this);
            {
                //OD_PROFILE_SCOPE("Scene::FixedPhysicsUpdate::Sync");
                executor->run(taskflow).wait(); 
                taskflow.clear();
            }

            fixedUpdateAccumulator -= _fixedStep;
        }
    }/* else {
        // PAUSED or invalid fixed step → freeze interpolation safely
        fixedUpdateAccumulator = std::clamp(fixedUpdateAccumulator, 0.0f, _fixedStep > 0.0f ? _fixedStep : 0.0f);
    }*/
    }
    #else
    {
        OD_PROFILE_SCOPE("Scene::FixedPhysicsUpdate");
        for(auto s: SceneManager::Get().globalFixedPhysicsSystems) s->FixedPhysicsUpdate(*this);
        for(auto s: fixedPhysicsSystems) s->FixedPhysicsUpdate(*this);
        {
            //OD_PROFILE_SCOPE("Scene::FixedPhysicsUpdate::Sync");
            executor->run(taskflow).wait(); 
            taskflow.clear();
        }
        fixedUpdateAccumulator -= _delta;
    }
    #endif

    //--------Post Physic---------
    {
        OD_PROFILE_SCOPE("Scene::PostPhysicsUpdate");
        for(auto s: postPhysicsSystems) s->PostPhysicsUpdate(*this);
        for(auto s: SceneManager::Get().globalPostPhysicsSystems) s->PostPhysicsUpdate(*this);
    }
    {
        OD_PROFILE_SCOPE("Scene::PostPhysicsUpdate::Sync");
        executor->run(taskflow).wait(); 
        taskflow.clear();
    }
    
    //--------Late---------
    {
        OD_PROFILE_SCOPE("Scene::LateUpdate");
        for(auto s: lateSystems){
            if(running == false && s->ExecuteAlways() == false) continue;
            s->LateUpdate(*this);
        }
        for(auto& s: SceneManager::Get().globalLateSystems){
            if(running == false && s->ExecuteAlways() == false) continue;
            s->LateUpdate(*this);
        }
    }
    {
        OD_PROFILE_SCOPE("Scene::LateUpdate::Sync");
        executor->run(taskflow).wait();
        taskflow.clear();
    }
    
    //TransformComponent::UpdateAllTransformMatrix(*this);

    /*}catch(...){
        Assert(false && "Scene::Update Catch Error"); //TODO: Make the scene stop and the Editor handle this too.
    }*/
}

void Scene::Draw(){
    OD_PROFILE_SCOPE("Scene::Draw");

    Graphics::Begin();
    for(auto& s: rendererSystems) s->Render(*this);   
    for(auto& s: SceneManager::Get().globalRendererSystems) s->Render(*this);     
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
    //os << std::setprecision(std::numeric_limits<float>::max_digits10);
    ODOutputArchive archive(os, cereal::JSONOutputArchive::Options(9));

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
    ResourceStream as(path, ResourceManager::Get().GetPackages());
    ODInputArchive archive(as.GetStream());

    //std::ifstream is(path);
    //ODInputArchive archive(is);

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

    this->path = path;

    LogWarning("LoadingScene: {} Succefu", path);

    TransformComponent::UpdateAllTransformMatrix(*this);
}

void Scene::_Load(const char* path, entt::entity prefab){
    LogWarning("LoadingPrefab: {}", path);
    //Assert(false);

    ResourceStream as(path, ResourceManager::Get().GetPackages());
    ODInputArchive archive(as.GetStream());

    //std::ifstream is(path);
    //ODInputArchive archive(is);

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

    //_LoadComponent<InfoComponent>(archive, loadLookup, registry, "InfoComponent");
    _LoadComponent<InfoComponent>(archive, loadLookup, registry, "InfoComponent", [](InfoComponent& c){
        if(c.layer < 0 || c.layer >= LayerCount){
            c.layer = Layers::Layer0;
        }
    });
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
    //LogWarning("LoadingPrefab: %s", path);
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

    //_LoadComponent<InfoComponent>(archive, loadLookup, registry, "InfoComponent");
    _LoadComponent<InfoComponent>(archive, loadLookup, registry, "InfoComponent", [](InfoComponent& c){
        if(c.layer < 0 || c.layer >= LayerCount){
            c.layer = Layers::Layer0;
        }
    });
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
    rootTrans.UpdateGlobalTransformCacheIfNeeded();

    return root;
}

Entity Scene::InstantiatePrefab(const char* prefabPath, Package& package){
    void* data = nullptr;
    size_t size;
    if(package.ReadFileData(prefabPath, data, size) == false){
        package.FreeFileData(data);
        return EntityNull;
    }

    MemoryInputStream mem((char*)data, size);
    //cereal::PortableBinaryInputArchive archive{mem};
    //std::ifstream is(path);
    ODInputArchive archive(mem);

    std::unordered_map<entt::entity, entt::entity> loadLookup;
    std::vector<entt::entity> entities;
    archive(cereal::make_nvp("Entities", entities));

    Entity root = EntityNull;

    for(auto i: entities){
        loadLookup[i] = registry.create();
        if(i == entities[0]) root = loadLookup[i]; //Entity(loadLookup[i], this);
    } 

    //_LoadComponent<InfoComponent>(archive, loadLookup, registry, "InfoComponent");
    _LoadComponent<InfoComponent>(archive, loadLookup, registry, "InfoComponent", [](InfoComponent& c){
        if(c.layer < 0 || c.layer >= LayerCount){
            c.layer = Layers::Layer0;
        }
    });
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
    rootTrans.UpdateGlobalTransformCacheIfNeeded();

    package.FreeFileData(data);
    return root;
}

bool Scene::LoadFromFile(const std::string& path){
    Assert(false && "Not Implemented");
    return false;
}

bool Scene::LoadFromPackage(const std::string& path, Package& package){
    Assert(false && "Not Implemented");
    return false;
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

/*tf::Executor& Scene::GetExecutor(){ 
    return executor; 
}*/

tf::Taskflow& Scene::GetTaskflow(){ 
    return taskflow; 
}

void Scene::RunAllTaskAndSync(){
    executor->run(taskflow).wait();
    taskflow.clear();
}

#pragma endregion

}