#include "Scene.h"
#include <fstream>
#include "Scripts.h"
#include "OD/OD.h"
#include "OD/Serialization/Serialization.h"
#include "OD/RenderPipeline/CameraComponent.h"

namespace entt{

template<typename Registry>
class Snapshot {
    static_assert(!std::is_const_v<Registry>, "Non-const registry type required");
    using traits_type = typename Registry::traits_type;

public:
    using registry_type = Registry;
    using entity_type = typename registry_type::entity_type;

    Snapshot(const registry_type &source) noexcept
        : reg{&source} {}

    Snapshot(Snapshot &&) noexcept = default;
    Snapshot &operator=(Snapshot &&) noexcept = default;

    template<typename Type, typename Archive>
    const Snapshot &get(Archive &archive, const id_type id = type_hash<Type>::value()) const {
        if(const auto *storage = reg->template storage<Type>(id); storage) {
            archive(static_cast<typename traits_type::entity_type>(storage->size()));

            if constexpr(std::is_same_v<Type, entity_type>) {
                archive(static_cast<typename traits_type::entity_type>(storage->in_use()));

                for(auto first = storage->data(), last = first + storage->size(); first != last; ++first) {
                    archive(*first);
                }
            } else {
                for(auto elem: storage->reach()) {
                    std::apply([&archive](auto &&...args) { (archive(std::forward<decltype(args)>(args)), ...); }, elem);
                }
            }
        } else {
            archive(typename traits_type::entity_type{});
        }

        return *this;
    }

    template<typename Type, typename Archive, typename It>
    const Snapshot &get(Archive &archive, It first, It last, const id_type id = type_hash<Type>::value()) const {
        static_assert(!std::is_same_v<Type, entity_type>, "Entity types not supported");

        if(const auto *storage = reg->template storage<Type>(id); storage && !storage->empty()) {
            archive(static_cast<typename traits_type::entity_type>(std::distance(first, last)));

            for(; first != last; ++first) {
                if(const auto entt = *first; storage->contains(entt)) {
                    archive(entt);
                    std::apply([&archive](auto &&...args) { (archive(std::forward<decltype(args)>(args)), ...); }, storage->get_as_tuple(entt));
                } else {
                    archive(static_cast<entity_type>(null));
                }
            }
        } else {
            archive(typename traits_type::entity_type{});
        }

        return *this;
    }

private:
    const registry_type *reg;
};

}

namespace OD{

#pragma region TransformComponent
Matrix4 TransformComponent::GlobalModelMatrix(){
    if(hasParent){
        TransformComponent& p = registry->get<TransformComponent>(parent);
        return p.GetLocalModelMatrix() * GetLocalModelMatrix();
    }
    return GetLocalModelMatrix();
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
#pragma endregion

#pragma region InfoComponent
#pragma endregion

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
    for(auto it = view.rbegin(); it != view.rend(); ++it){
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
    for(auto i: standSystems) delete i;
    for(auto i: rendererSystems) delete i;
    for(auto i: physicsSystems) delete i;
}

Scene* Scene::Copy(Scene* other){
    return new Scene(*other); 

    Scene* scene = new Scene();

    auto view = other->registry.view<entt::entity>();
    for(auto it = view.rbegin(); it != view.rend(); ++it){
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

    Renderer::Begin();
    for(auto& s: rendererSystems) s->Update();        
    Renderer::End();
}

void Scene::Save(const char* path){
    std::ofstream os(path);
    cereal::JSONOutputArchive output{os};

    entt::snapshot snapshotOut(registry);
    entt::Snapshot<entt::registry> snapshotOut2(registry);

    snapshotOut.get<entt::entity>(output);
    snapshotOut.get<TransformComponent>(output);
    snapshotOut.get<InfoComponent>(output);
    for(auto i: SceneManager::Get().componentsSerializer){
        i.second.snapshotOut(snapshotOut, output);
    }
    for(auto i: SceneManager::Get().coreComponentsSerializer){
        i.second.snapshotOut(snapshotOut, output);
    }
}

void Scene::Load(const char* path){
    std::ifstream storage(path);
    cereal::JSONInputArchive input{storage};

    entt::snapshot_loader snapshot(registry);

    snapshot.get<entt::entity>(input);
    snapshot.get<TransformComponent>(input);
    snapshot.get<InfoComponent>(input);

    for(auto e: registry.view<TransformComponent, InfoComponent>()){
        TransformComponent& trans = registry.get<TransformComponent>(e);
        InfoComponent& info = registry.get<InfoComponent>(e);

        trans.registry = &registry;
        info.id = e;
    }

    for(auto i: SceneManager::Get().componentsSerializer){
        i.second.snapshotIn(snapshot, input);
    }
    for(auto i: SceneManager::Get().coreComponentsSerializer){
        i.second.snapshotIn(snapshot, input);
    }
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

#pragma region SceneManager

SceneManager& SceneManager::Get(){
    static SceneManager sceneManager;
    return sceneManager;
}

SceneManager::SceneState SceneManager::GetSceneState(){ 
    return sceneState; 
}

bool SceneManager::InEditor(){ 
    return inEditor; 
}

Scene* SceneManager::GetActiveScene(){ 
    if(activeScene == nullptr) return NewScene();
    return activeScene; 
}

void SceneManager::SetActiveScene(Scene* s){ 
    activeScene = s; 
}

Scene* SceneManager::NewScene(){
    delete activeScene;
    activeScene = new Scene();
    return activeScene;
}

#pragma endregion

}