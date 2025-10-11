#include "Animator.h"
#include "Ultis/CameraMovement.h"
#include "Ultis/Ultis.h"
#include <OD/Scene/SceneManager.h>
#include <OD/RenderPipeline/LightComponent.h>
#include <OD/RenderPipeline/CameraComponent.h>
#include <OD/RenderPipeline/ModelRendererComponent.h>
#include <OD/RenderPipeline/EnvironmentComponent.h>
#include <OD/Animation/Animator.h>
#include <OD/Physics/PhysicsSystem.h>
#include <OD/Editor/Editor.h>
#include <OD/Core/Application.h>
#include <entt/entt.hpp>
#include <assert.h>
#include <vector>
#include <cstddef>
#include <algorithm>
#include <execution>
#include <taskflow/algorithm/for_each.hpp>

/*template<typename T>
class ArenaAllocator{
public:
    void Init(size_t chunkSize);
    T* Alloc();
    void Free(T* p);
    void Reset();

private:
    struct Chunk{
        void* data;
        size_t curOffset;
    };
    struct FreeOffset{
        int chunkIndex;
        size_t offset;
    };

    std::vector<Chunk> chunks;
    std::vector<FreeOffset> freeOffsets;
};*/


template<typename T>
class ArenaAllocator {
public:
    // Initialize the allocator with a specified chunk size
    void Init(size_t chunkSize) {
        chunkCapacity = chunkSize;
        // Ensure at least one chunk is available
        if (chunks.empty()) {
            AddNewChunk();
        }
    }

    // Allocate memory for a single T object
    T* Alloc() {
        // First check if we have any free spots from previous deallocations
        if (!freeOffsets.empty()) {
            FreeOffset fo = freeOffsets.back();
            freeOffsets.pop_back();
            Chunk& chunk = chunks[fo.chunkIndex];
            return reinterpret_cast<T*>(static_cast<char*>(chunk.data) + fo.offset);
        }

        // If no free spots, allocate from current chunk or create new one
        Chunk& currentChunk = chunks.back();
        if (currentChunk.curOffset + sizeof(T) > chunkCapacity) {
            AddNewChunk();
            currentChunk = chunks.back();
        }

        T* result = reinterpret_cast<T*>(static_cast<char*>(currentChunk.data) + currentChunk.curOffset);
        currentChunk.curOffset += sizeof(T);
        return result;
    }

    // Free an allocated object
    void Free(T* p) {
        if (p == nullptr) return;

        // Find which chunk this pointer belongs to
        for (int i = 0; i < chunks.size(); ++i) {
            char* chunkStart = static_cast<char*>(chunks[i].data);
            char* chunkEnd = chunkStart + chunkCapacity;
            char* ptr = reinterpret_cast<char*>(p);

            if (ptr >= chunkStart && ptr < chunkEnd) {
                FreeOffset fo;
                fo.chunkIndex = i;
                fo.offset = static_cast<size_t>(ptr - chunkStart);
                freeOffsets.push_back(fo);
                return;
            }
        }
    }

    // Reset the allocator to initial state
    void Reset() {
        // Free all chunks except the first one
        for (size_t i = 1; i < chunks.size(); ++i) {
            delete[] static_cast<char*>(chunks[i].data);
        }
        if (!chunks.empty()) {
            chunks.resize(1);
            chunks[0].curOffset = 0;
        }
        freeOffsets.clear();
    }

    // Destructor
    ~ArenaAllocator() {
        for (auto& chunk : chunks) {
            delete[] static_cast<char*>(chunk.data);
        }
    }

public:// private:
    struct Chunk {
        void* data;
        size_t curOffset;
    };

    struct FreeOffset {
        int chunkIndex;
        size_t offset;
    };

    std::vector<Chunk> chunks;
    std::vector<FreeOffset> freeOffsets;
    size_t chunkCapacity = 0;

    // Helper function to add a new chunk
    void AddNewChunk() {
        void* newData = new char[chunkCapacity];
        Chunk newChunk{newData, 0};
        chunks.push_back(newChunk);
    }
};

class ArenaAllocator2 {
public:
    void Init(size_t chunkSize) {
        chunkCapacity = chunkSize;
        if (chunks.empty()) {
            AddNewChunk();
        }
    }

    template<typename T>
    T* Alloc() {
        // Check free slots first
        for (auto it = freeOffsets.begin(); it != freeOffsets.end(); ++it) {
            if (it->size == sizeof(T)) {  // Check if size matches
                FreeOffset fo = *it;
                freeOffsets.erase(it);
                Chunk& chunk = chunks[fo.chunkIndex];
                return reinterpret_cast<T*>(static_cast<char*>(chunk.data) + fo.offset);
            }
        }

        // No suitable free slot found, allocate new space
        Chunk& currentChunk = chunks.back();
        if (currentChunk.curOffset + sizeof(T) > chunkCapacity) {
            AddNewChunk();
            currentChunk = chunks.back();
        }

        T* result = reinterpret_cast<T*>(static_cast<char*>(currentChunk.data) + currentChunk.curOffset);
        currentChunk.curOffset += sizeof(T);
        return result;
    }

    template<typename T>
    void Free(T* p) {
        if (p == nullptr) return;

        for (int i = 0; i < chunks.size(); ++i) {
            char* chunkStart = static_cast<char*>(chunks[i].data);
            char* chunkEnd = chunkStart + chunkCapacity;
            char* ptr = reinterpret_cast<char*>(p);

            if (ptr >= chunkStart && ptr < chunkEnd) {
                FreeOffset fo;
                fo.chunkIndex = i;
                fo.offset = static_cast<size_t>(ptr - chunkStart);
                fo.size = sizeof(T);  // Store the size of the freed type
                freeOffsets.push_back(fo);
                return;
            }
        }
    }

    void Reset() {
        for (size_t i = 1; i < chunks.size(); ++i) {
            delete[] static_cast<char*>(chunks[i].data);
        }
        if (!chunks.empty()) {
            chunks.resize(1);
            chunks[0].curOffset = 0;
        }
        freeOffsets.clear();
    }

    ~ArenaAllocator2() {
        for (auto& chunk : chunks) {
            delete[] static_cast<char*>(chunk.data);
        }
    }

public:// private:
    struct Chunk {
        void* data;
        size_t curOffset;
    };

    struct FreeOffset {
        int chunkIndex;
        size_t offset;
        size_t size;  // Added size field to track allocation size
    };

    std::vector<Chunk> chunks;
    std::vector<FreeOffset> freeOffsets;
    size_t chunkCapacity = 0;

    void AddNewChunk() {
        void* newData = new char[chunkCapacity];
        Chunk newChunk{newData, 0};
        chunks.push_back(newChunk);
    }
};

template<typename T>
class _ArenaLinearAllocator {
public:
    // Static factory method to create and initialize an allocator
    static std::shared_ptr<ArenaLinearAllocator<T>> Create(int _chunkCapacity) {
        auto allocator = std::make_shared<ArenaLinearAllocator<T>>();
        allocator->Init(_chunkCapacity);
        return allocator;
    }

    // Initialize the allocator (for manual creation)
    void Init(int _chunkCapacity) {
        if (_chunkCapacity <= 0) {
            throw std::invalid_argument("Chunk capacity must be positive");
        }
        chunkCapacity = _chunkCapacity;
        chunks.emplace_back();
        Chunk& chunk = chunks.back();
        chunk.data = malloc(sizeof(T) * chunkCapacity); // Allocate slots * sizeof(T)
        if (!chunk.data) {
            throw std::bad_alloc();
        }
        chunk.curIndex = 0;
        chunk.usedIndexs.resize(chunkCapacity, false);
        curChunk = 0;
    }

    // Original Alloc: Returns raw T*
    template<typename... Args>
    T* Alloc(Args&&... args) {
        // Check for reusable free indices
        if (!freeIndexs.empty()) {
            FreeIndex free = freeIndexs.back();
            freeIndexs.pop_back();
            Chunk& chunk = chunks[free.chunkIndex];
            chunk.usedIndexs[free.index] = true;
            T* ptr = reinterpret_cast<T*>(chunk.data) + free.index; // Fastest cast
            new(ptr) T(std::forward<Args>(args)...); // Construct T
            return ptr;
        }

        // Use the current chunk
        Chunk& currentChunk = chunks[curChunk];
        if (currentChunk.curIndex + 1 > static_cast<int>(chunkCapacity)) {
            AddNewChunk();
            curChunk = chunks.size() - 1;
            currentChunk = chunks.back();
        }

        // Allocate from the current chunk
        currentChunk.usedIndexs[currentChunk.curIndex] = true;
        T* ptr = reinterpret_cast<T*>(currentChunk.data) + currentChunk.curIndex;
        new(ptr) T(std::forward<Args>(args)...); // Construct T
        ++currentChunk.curIndex;
        return ptr;
    }

    // New Alloc: Returns shared_ptr<T>
    template<typename... Args>
    std::shared_ptr<T> AllocShared(Args&&... args) {
        T* ptr = Alloc(std::forward<Args>(args)...); // Reuse Alloc logic
        return std::shared_ptr<T>(ptr, [this](T* p) { this->FreeRaw(p); });
    }

    // Original Free: Takes raw T*
    void Free(T* p) {
        FreeRaw(p); // Delegate to FreeRaw
    }

    // New Free: Takes shared_ptr<T>
    void Free(std::shared_ptr<T>& sp) {
        if (!sp) return;
        FreeRaw(sp.get()); // Free the raw pointer
        sp.reset(); // Release ownership
    }

    // ForEach with raw T* (can change to shared_ptr if needed)
    void ForEach(std::function<void(T*)> func) {
        for (size_t i = 0; i < chunks.size(); ++i) {
            Chunk& chunk = chunks[i];
            for (size_t j = 0; j < chunk.usedIndexs.size(); ++j) {
                if (chunk.usedIndexs[j]) {
                    T* ptr = reinterpret_cast<T*>(chunk.data) + j;
                    func(ptr);
                }
            }
        }
    }

    void Reset() {
        for (size_t i = 0; i < chunks.size(); ++i) {
            Chunk& chunk = chunks[i];
            for (size_t j = 0; j < chunk.usedIndexs.size(); ++j) {
                if (chunk.usedIndexs[j]) {
                    T* ptr = reinterpret_cast<T*>(chunk.data) + j;
                    ptr->~T();
                }
            }
        }
        freeIndexs.clear();
        for (Chunk& chunk : chunks) {
            chunk.curIndex = 0;
            std::fill(chunk.usedIndexs.begin(), chunk.usedIndexs.end(), false);
        }
        curChunk = 0;
    }

    ~_ArenaLinearAllocator() {
        for (size_t i = 0; i < chunks.size(); ++i) {
            Chunk& chunk = chunks[i];
            for (size_t j = 0; j < chunk.usedIndexs.size(); ++j) {
                if (chunk.usedIndexs[j]) {
                    T* ptr = reinterpret_cast<T*>(chunk.data) + j;
                    ptr->~T();
                }
            }
            free(chunk.data);
        }
    }

public: // private:
    struct Chunk {
        void* data;
        int curIndex = 0; // Tracks slot index
        std::vector<bool> usedIndexs; // Tracks used slots (true = used, false = free)
    };

    struct FreeIndex {
        int chunkIndex;
        int index; // Slot index
    };

    // Internal free function for raw pointers (used by Free, AllocShared deleter)
    void FreeRaw(T* p) {
        if (!p) return;

        for (size_t i = 0; i < chunks.size(); ++i) {
            Chunk& chunk = chunks[i];
            T* chunkStart = reinterpret_cast<T*>(chunk.data);
            T* chunkEnd = chunkStart + chunkCapacity;
            if (p >= chunkStart && p < chunkEnd) {
                int index = static_cast<int>(p - chunkStart);
                if (index >= 0 && static_cast<size_t>(index) < chunk.usedIndexs.size() && chunk.usedIndexs[index]) {
                    p->~T(); // Call destructor
                    chunk.usedIndexs[index] = false;
                    freeIndexs.push_back({static_cast<int>(i), index});
                    if (i < curChunk) {
                        curChunk = i; // Move curChunk back
                    }
                } else {
                    throw std::runtime_error("Attempt to free unallocated or invalid pointer");
                }
                return;
            }
        }
        throw std::runtime_error("Pointer does not belong to any chunk");
    }

    void AddNewChunk() {
        chunks.emplace_back();
        Chunk& newChunk = chunks.back();
        newChunk.data = malloc(sizeof(T) * chunkCapacity); // Allocate slots * sizeof(T)
        if (!newChunk.data) {
            throw std::bad_alloc();
        }
        newChunk.curIndex = 0;
        newChunk.usedIndexs.resize(chunkCapacity, false);
    }

    std::vector<Chunk> chunks;
    std::vector<FreeIndex> freeIndexs;
    size_t chunkCapacity = 0; // Number of slots
    size_t curChunk = 0; // Current chunk index
};

template<typename Iter, typename Func>
void tf_for_each(tf::Taskflow& taskflow, Iter begin, Iter end, Func func) {
    size_t total = std::distance(begin, end);
    if (total == 0) return;

    const int num_threads = std::thread::hardware_concurrency();
    size_t chunk = (total + num_threads - 1) / num_threads;

    for (int t = 0; t < num_threads; ++t) {
        auto chunk_begin = std::next(begin, t * chunk);
        auto chunk_end   = (t + 1 < num_threads) 
                         ? std::next(begin, (t + 1) * chunk)
                         : end;

        if (chunk_begin == end) break; // stop if overshooting

        taskflow.emplace([=] {
            for (auto it = chunk_begin; it != chunk_end; ++it) {
                func(*it);
            }
        });
    }
}

template<typename Iter, typename Func>
void tf_for_each2(tf::Taskflow& taskflow, Iter begin, Iter end, Func func) {
    size_t total = std::distance(begin, end);
    if (total == 0) return;

    const int num_threads = std::thread::hardware_concurrency();
    size_t chunk = (total + num_threads - 1) / num_threads;

    // Local helper lambda to move iterators
    auto advance_iter = [](Iter it, size_t offset) {
        if constexpr (std::is_base_of_v<
            std::random_access_iterator_tag,
            typename std::iterator_traits<Iter>::iterator_category
        >) {
            LogInfo("Use Random Acess");
            return it + offset;  // fast for random-access
        } else {
            return std::next(it, offset); // safe for forward iterators
        }
    };

    for (int t = 0; t < num_threads; ++t) {
        auto chunk_begin = advance_iter(begin, t * chunk);
        auto chunk_end   = (t + 1 < num_threads)
                         ? advance_iter(begin, (t + 1) * chunk)
                         : end;

        if (chunk_begin == end) break;

        taskflow.emplace([=] {
            for (auto it = chunk_begin; it != chunk_end; ++it) {
                func(*it);
            }
        });
    }
}

void AnimatorSample::OnInit(){
    //ArenaAllocator<int> allocator;
    //allocator.Init(sizeof(int) * 1); // Initialize with 1KB chunks
    /*
    _ArenaLinearAllocator<int> allocator;
    allocator.Init(1);

    int* ptr1 = allocator.Alloc();
    *ptr1 = 42;
    int* ptr2 = allocator.Alloc();
    *ptr2 = 84;

    allocator.Free(ptr1);
    int* ptr3 = allocator.Alloc(); // Should reuse ptr1's space
    *ptr3 = 330;
    
    Assert(ptr1 == ptr3);
    Assert(allocator.chunks.size() == 2);
    Assert(allocator.freeIndexs.size() == 0);

    allocator.Reset(); // Reset to initial state

    ArenaAllocator2 allocator2;
    allocator2.Init(8); // 1024 bytes per chunk

    // Allocate different types
    int* intPtr = allocator2.Alloc<int>();
    *intPtr = 42;
    
    double* doublePtr = allocator2.Alloc<double>();
    *doublePtr = 3.14;
    
    char* charPtr = allocator2.Alloc<char>();
    *charPtr = 'A';

    // Use the values
    std::cout << *intPtr << " " << *doublePtr << " " << *charPtr << std::endl;

    // Free different types
    allocator2.Free(intPtr);
    //allocator2.Free(doublePtr);
    //allocator2.Free(charPtr);

    // Reuse freed space with different type
    float* floatPtr = allocator2.Alloc<float>();
    *floatPtr = 2.718;

    Assert((void*)intPtr == (void*)floatPtr);
    Assert(allocator2.chunks.size() == 2);

    std::cout << *floatPtr << std::endl;

    allocator2.Reset();
    */

    LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

    //Application::Vsync(false);

    SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

    Scene* scene = SceneManager::Get().NewScene();

    Entity env = scene->AddEntity("Env");
    scene->AddComponent<EnvironmentComponent>(env).settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

    Entity light = scene->AddEntity("Light");
    LightComponent& lightComponent = scene->AddComponent<LightComponent>(light);
    lightComponent.color = {1,1,1};
    lightComponent.renderShadow = false;
    scene->GetComponent<TransformComponent>(light).Position(Vector3(-2, 4, -1));
    scene->GetComponent<TransformComponent>(light).LocalEulerAngles(Vector3(45, -125, 0));

    camera = scene->AddEntity("Camera");
    CameraComponent& cam = scene->AddComponent<CameraComponent>(camera);
    scene->GetComponent<TransformComponent>(camera).LocalPosition(Vector3(0, 15, 15));
    scene->GetComponent<TransformComponent>(camera).LocalEulerAngles(Vector3(-25, 0, 0));
    scene->AddComponent<ScriptComponent>(camera).AddScript<CameraMovementScript>()->moveSpeed = 60;
    cam.farClipPlane = 1000;

    Ref<Model> floorModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/plane.glb");
    Ref<Model> cubeModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/Cube.glb");

    Entity floorEntity = scene->AddEntity("Floor");
    TransformComponent& floorTransform = scene->GetComponent<TransformComponent>(floorEntity);
    floorTransform.LocalScale(Vector3(5));
    ModelRendererComponent& floorRenderer = scene->AddComponent<ModelRendererComponent>(floorEntity);
    floorRenderer.SetModel(floorModel);
    floorRenderer.GetMaterialsOverride()[0] = LoadFloorMaterial();
    RigidbodyComponent& floorEntityP = scene->AddComponent<RigidbodyComponent>(floorEntity);
    floorEntityP.SetShape(CollisionShape::BoxShape({25,0.1f,25}));
    floorEntityP.Mass(0);
    floorEntityP.SetType(RigidbodyComponent::Type::Static);
    floorEntityP.NeverSleep(true);
    //floorEntityP->entity()->transform().localEulerAngles({0,0,-25});

    Ref<Model> charModel = AssetManager::Get().LoadAsset<Model>(
        //"res/Game/Animations/Walking.dae"
        "Sandbox/Animations/RumbaDancing3.glb"
        //"res/Game/Animations/SillyDancing.fbx"
        //"res/Game/Animations/UnarmedWalkForward.dae"
    );
    charModel->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/Lit.glsl"));

    
    /*OD::BoneMap bm = OD::RearrangeSkeleton(charModel->skeleton);
    //OD::RearrangeMesh(*charModel->meshs[0], bm);
    for(auto i: charModel->animationClips){
        clips.push_back(OD::OptimizeClip(*i));
        OD::RearrangeFastclip(clips[clips.size()-1], bm);
    }*/
    
    /*Entity charEntity = scene->AddEntity("Character");
    TransformComponent& charTrans = charEntity.GetComponent<TransformComponent>();
    charTrans.LocalScale(Vector3(0.01f));
    SkinnedModelRendererComponent& charRenderer = charEntity.AddComponent<SkinnedModelRendererComponent>();
    charRenderer.SetModel(charModel);
    //charRenderer.CreateSkeletonEntites(*scene, charEntity);
    charRenderer.SetAABB(Vector3(0,0.01f,0), Vector3(0.01f/2, 0.01f, 0.01f/4));
    charRenderer.UpdatePosePalette();
    LogInfo("CharModel Skeleton RestPose Size: %d", charModel->skeleton.GetRestPose().Size());
    Assert(charRenderer.posePalette.size() == charModel->skeleton.GetRestPose().Size());
    AnimatorComponent& charAnim = charEntity.AddComponent<AnimatorComponent>();
    charAnim.Play(charModel->animationClips[0].get());*/
    
    const int Size = 32*1.5f;
    for(int x = -(Size/2); x <= (Size/2); x++){
        for(int y = -(Size/2); y <= (Size/2); y++){
            Entity charEntity = scene->AddEntity("Character_" + std::to_string(x) + "_" + std::to_string(y));
            
            TransformComponent& charTrans = scene->GetComponent<TransformComponent>(charEntity);
            charTrans.Position(Vector3(x*2, 0, y*2));
            charTrans.LocalScale(Vector3(1));
            
            SkinnedModelRendererComponent& charRenderer = scene->AddComponent<SkinnedModelRendererComponent>(charEntity);
            charRenderer.SetModel(charModel);
            //charRenderer.SetAABB(Vector3(0,0.01f,0), Vector3(0.01f/2, 0.01f, 0.01f/4));
            charRenderer.UpdatePosePalette();
            //LogInfo("CharModel Skeleton RestPose Size: %d", charModel->skeleton.GetRestPose().Size());
            Assert(charRenderer.posePalette.size() == charModel->skeleton.GetRestPose().Size());
            
            AnimatorComponent& charAnim = scene->AddComponent<AnimatorComponent>(charEntity);
            charAnim.Play(charModel->animationClips[0].get() /*&clips[0]*/);
        }
    }
    
    //scene->Start();
    //RenderContext::GetSettings().enableGizmos = false;
    Application::AddModule<Editor>();

    LogInfo("AnimationCount: %zd", charModel->animationClips.size());

    //TODO: Add this patter to the Animator system to impruve cache acess
    /*auto view = scene->GetRegistry().group<InfoComponent, AnimatorComponent>();
    int aaa = std::distance(view.begin(), view.end());
    LogInfo("---Count: %d", aaa);  

	auto& entities = view.handle();
    size_t total_entities = entities.size(); // Should be 100
    size_t entities_per_task = total_entities / 4; // 25 entities per task
    size_t remaining_entities = total_entities % 4; // Handle any remainder
    size_t start_idx = 0;

    for(int i = 0; i < 4; i++){
        size_t count = entities_per_task + (i < remaining_entities ? 1 : 0);

        scene->GetTaskflow().emplace([&, i, count](){
        for(size_t i = start_idx; i < start_idx + count && i < entities.size(); ++i){
			auto entity = entities[i];
			InfoComponent& info = view.get<InfoComponent>(entity);
            AnimatorComponent& anim = view.get<AnimatorComponent>(entity);
            LogInfo("---Name: %s %d", info.name.c_str(), anim.enable == true ? 1 : 0);  
        }
        });
    }

    scene->GetExecutor().run(scene->GetTaskflow()).wait();
    scene->GetTaskflow().clear();*/

    /*auto view = scene->GetRegistry().view<InfoComponent, AnimatorComponent>();
    std::for_each(std::execution::par_unseq, view.begin(), view.end(), [&view](auto entity){
        InfoComponent& info = view.get<InfoComponent>(entity);
        AnimatorComponent& anim = view.get<AnimatorComponent>(entity);
        LogInfo("---Name: %s %d", info.name.c_str(), anim.enable == true ? 1 : 0);  
    });*/

    /*auto view = scene->GetRegistry().view<InfoComponent, AnimatorComponent>();
    auto task = scene->GetTaskflow().for_each(view.begin(), view.end(), [&view](auto entity){
        InfoComponent& info = view.get<InfoComponent>(entity);
        AnimatorComponent& anim = view.get<AnimatorComponent>(entity);
        LogInfo("---Name: %s %d", info.name.c_str(), anim.enable == true ? 1 : 0);  
    }, tf::GuidedPartitioner( 0));
    scene->GetExecutor().run(scene->GetTaskflow()).wait();
    scene->GetTaskflow().clear();*/

    /*auto view = scene->GetRegistry().group<InfoComponent, AnimatorComponent>();
    tf_for_each2(scene->GetTaskflow(), view.begin(), view.end(), [&view](auto entity){
        InfoComponent& info = view.get<InfoComponent>(entity);
        AnimatorComponent& anim = view.get<AnimatorComponent>(entity);
        //LogInfo("---Name: %s %d", info.name.c_str(), anim.enable == true ? 1 : 0);  
    });

    LogInfo("Task Count: %zd", scene->GetTaskflow().num_tasks());
    scene->GetExecutor().run(scene->GetTaskflow()).wait();
    scene->GetTaskflow().clear();*/
}

void AnimatorSample::OnUpdate(float deltaTime){
    //Scene* scene = SceneManager::Get().GetActiveScene();
    //scene->Update();
    //if(scene->Running() == false) return;
}   

void AnimatorSample::OnRender(float deltaTime){
    //SceneManager::Get().GetActiveScene()->Draw();
}

void AnimatorSample::OnGUI(){}
void AnimatorSample::OnResize(int width, int height){}
void AnimatorSample::OnExit(){}