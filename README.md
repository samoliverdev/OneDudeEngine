# My Toy Game Engine

Current Version 3.2.b

### How Compiler with Visual Studio

cmake -S . -B build
cmake --build build --config Release
./build/Release/game

# Lean References

* [Unity Engine](https://unity.com/)
* <https://learnopengl.com/>
* <https://catlikecoding.com/unity/tutorials/custom-srp/>
* <https://www.youtube.com/@TheCherno>
* [Book: Hands-On C++ Game Animation Programming](https://subscription.packtpub.com/search?query=hands%20on%20game%20animation%20programming)

## Roadmap Version 1.0
- [x] General
    - [ ] Engine Name
    - [x] AssetSystem
    - [x] DearImgui
    - [ ] Project System
    - [x] Dynamic Module Load 
    - [ ] Mod Support
    - [ ] Binary Asset Compile/Pack
    - [ ] Unit Tests
    - [ ] Sample / Stand Assets
- [x] Platforms
    - [x] Windows
    - [ ] Linux 
    - [ ] Android
- [x] Graphic
    - [x] Texture2D
    - [x] TextureArray
    - [ ] Texture3D
    - [x] Cubemap
    - [x] Font
    - [x] Framebuffer
    - [x] Shader
    - [x] MultiCompileShader
    - [x] Material
    - [x] Mesh
    - [x] Model
    - [x] Draw
    - [x] DrawInstanced
    - [ ] DrawIndirect
- [x] RenderPipeline
    - [x] Camera
    - [ ] Multi Camera | Split Screen
    - [ ] Render To Texture
    - [x] Mesh Renderer
    - [x] Model Renderer
    - [x] Skinned Model Renderer
    - [x] Lights
    - [x] Directional Light Shadow
    - [ ] Point Light Shadow
    - [x] Post Processing
    - [x] Gizmos
    - [ ] UISystem
        - [x] Text 
        - [ ] Panel
        - [ ] Achor
    - [ ] Enviroment Probe / Reflection Probe
    - [ ] Occlusion Culling
    - [ ] Relative Renderer
    - [ ] Particle System
    - [x] Forward Rendering
    - [ ] Deffered Rendering
    - [ ] Decal
- [x] Scripting
    - [x] Native Scripting
    - [ ] Lua Scripting
- [x] Physics
    - [x] Rigdbody
    - [x] Box, Sphere, Capsule Collider
    - [ ] Mesh Collider
    - [ ] Ragdoll
- [x] Animations
    - [x] Skinned
    - [ ] Skinned Skeleton Socket
    - [ ] IK
- [x] Scene
    - [x] ECS
    - [x] Save / Load
    - [x] Prefab
- [x] AI
    - [x] Navmesh
        - [x] Bake
        - [x] Simple Agent
        - [ ] Tile Bake
- [x] Audio
    - [x] Play Sound
    - [ ] Play 3d Sound
    - [ ] Multi Sound Listen
- [x] Profile
    - [x] Simple View
    - [ ] Tree View
- [x] Editor

### Planned

* Native Dear Imgui Backend
* Global Illuminations 
* Seamless C++ scripting | c++ hot reloading 
* 2D Game Features(Sprite, Tile map)
* Double Precesion
* GPU Driver Renderer
* Suport Vulkan

