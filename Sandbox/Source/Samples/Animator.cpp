#include "Animator.h"
#include "Ultis/CameraMovement.h"
#include <assert.h>
#include "Ultis/Ultis.h"
#include <entt/entt.hpp>

#include <vector>
#include <cstddef>

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

void AnimatorSample::OnInit(){
    ArenaAllocator<int> allocator;
    allocator.Init(sizeof(int) * 1); // Initialize with 1KB chunks

    int* ptr1 = allocator.Alloc();
    *ptr1 = 42;
    int* ptr2 = allocator.Alloc();
    *ptr2 = 84;

    allocator.Free(ptr1);
    int* ptr3 = allocator.Alloc(); // Should reuse ptr1's space
    *ptr3 = 330;
    
    Assert(ptr1 == ptr3);
    Assert(allocator.chunks.size() == 2);
    Assert(allocator.freeOffsets.size() == 0);

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

    LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

    //Application::Vsync(false);

    SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

    Scene* scene = SceneManager::Get().NewScene();

    Entity env = scene->AddEntity("Env");
    scene->AddComponent<EnvironmentComponent>(env).settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

    Entity light = scene->AddEntity("Light");
    LightComponent& lightComponent = scene->AddComponent<LightComponent>(light);
    lightComponent.color = {1,1,1};
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
    charModel->SetShader(AssetManager::Get().LoadAsset<Shader>("Engine/Shaders/LitNewSyntax.glsl"));

    
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
            Entity charEntity = scene->AddEntity("Character");
            
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