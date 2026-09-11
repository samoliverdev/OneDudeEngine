#pragma once
#include "OD/Defines.h"
#include "OD/Core/Math.h"
#include "OD/Core/Color.h"
#include "Camera.h"
#include "RendererTypes.h"
#include "OD/Gfx/Gfx.h"
//#include "Framebuffer.h"

//#define EnableExperimentalPerDrawCustomData 1

namespace sol{ class state; }

namespace OD {

const int MAX_BONES = 120;

class SubShader;
class Mesh;
class Model;
class Framebuffer;
class Font;
class Material;
class InstancingBuffer;
class UniformBuffer;
struct TextParams;

class GraphicsDevice;

struct GraphicsStats{
    int drawCalls;
    int vertices;
    int tris;
    int shaderBinds;
    int uniformSet;
    int uniformBufferUpdates;
    int materialSubmitDatas;
};

struct GraphicsDebug{
    enum class Type{DrawMesh, DrawMeshSkinned, DrawMeshInstancing, BindMaterial, BindShader, BeginPass, EndPass, UniformSet};

    struct Data{
        Type type;

        union{
            struct { Mesh* drawMesh; };
            struct { Mesh* drawMeshSkinned; };
            struct { Mesh* drawMeshSkinned; };
            struct { Material* bindMaterial; };
            struct { SubShader* bindSubShader; };
            struct { Framebuffer* pass; };
            struct { const char* uniformSet; };
        }; 
    };

    inline void DrawMesh(Mesh* mesh){
        datas.push_back({Type::DrawMesh});
        datas.back().drawMesh = mesh;
    }

    inline void DrawMeshSkinned(Mesh* mesh){
        datas.push_back({Type::DrawMeshSkinned});
        datas.back().drawMesh = mesh;
    }

    inline void DrawMeshInstancing(Mesh* mesh){
        datas.push_back({Type::DrawMeshInstancing});
        datas.back().drawMesh = mesh;
    }

    inline void BindMaterial(Material* mat){
        datas.push_back({Type::BindMaterial});
        datas.back().bindMaterial = mat;
    }

    inline void BindShader(SubShader* shader){
        datas.push_back({Type::BindShader});
        datas.back().bindSubShader = shader;
    }

    inline void BeginPass(Framebuffer* a){
        datas.push_back({Type::BeginPass});
        datas.back().pass = a;
    }

    inline void EndPass(){
        datas.push_back({Type::EndPass});
    }

    inline void UniformSet(const char* n){
        datas.push_back({Type::UniformSet});
        datas.back().uniformSet = n;
    }

    std::vector<Data> datas;
};

struct GPUMemoryStats{
    size_t texturesBytes = 0;
    size_t meshBytes = 0;
    size_t framebuffersBytes = 0;
    size_t buffersBytes = 0;
};
  
enum class OD_API_IMPORT RenderMode{SHADED, WIREFRAME};

struct OD_API alignas(16) PerDrawData {
    //std::vector<int> int_0;
    //std::vector<Vector4> vector4_0;

    std::array<Vector4, 2> vector4_0;
    //int int_0_Count = 0;

    std::array<int, 4> int_0;
    //int vector4_0_Count = 0;

    unsigned char vector4_0_Mask = 0;
    unsigned char int_0_Mask = 0;

    inline bool Vector4_0_HasMask(int index){ return (vector4_0_Mask & (1 << index)) != 0; }
    inline void Vector4_0_SetMask(int index, bool enable) noexcept {
        /*if(enable)
            vector4_0_Mask |= (1 << index);   // set bit
        else
            vector4_0_Mask &= ~(1 << index);  // clear bit*/

        const unsigned char bit = 1 << index;
        vector4_0_Mask = (vector4_0_Mask & ~bit) | (-static_cast<unsigned char>(enable) & bit);
    }
    
    inline bool Int_0_HasMask(int index){ return (int_0_Mask & (1 << index)) != 0; }
    inline void Int_0_SetMask(int index, bool enable) noexcept {
        /*if(enable)
            int_0_Mask |= (1 << index);   // set bit
        else
            int_0_Mask &= ~(1 << index);  // clear bit*/

        const unsigned char bit = 1 << index;
        int_0_Mask = (int_0_Mask & ~bit) | (-static_cast<unsigned char>(enable) & bit);
    }
};

class OD_API Graphics {
    friend class Application;
    friend class Material;
public:
    static GraphicsStats& GetStats();
    static GPUMemoryStats& GetMemoryStats();
    static GraphicsDebug& GetGraphicsDebug();
    static void Begin();
    static void End();
    static bool HasBegin();

    static void SetCamera(Camera& camera);
    static Camera GetCamera();

    //static void SubmitGlobalData(void* data, size_t size);
    //static void SubmitRenderPipelineData(void* data, size_t size);

    static void BeginRenderToScreen(Vector4 clearColor = Vector4(0, 0, 0, 1));
    static void EndRenderToScreen();

    static void Clean(float r, float g, float b, float a);
    static void CleanColorOnly(float r, float g, float b, float a);
    static void CleanDepthOnly();
    static void SetViewport(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
    static void GetViewport(unsigned int*x, unsigned int* y, unsigned int* w, unsigned int* h);

    static void EnableScissor();
    static void DisableScissor();
    static void Scissor(unsigned int x, unsigned int y, int w, int h);

    static void DrawMesh(Mesh& mesh, Material& mat, Matrix4 modelMatrix, PerDrawData* perDrawData = nullptr);
    static void DrawMeshSkinned(Mesh& mesh, Material& mat, Matrix4 model, Matrix4* animMatrix, int count, PerDrawData* perDrawData = nullptr);
    static void DrawMeshSkinned(Mesh& mesh, Material& shader, Matrix4 model, UniformBuffer* data, int count, PerDrawData* perDrawData = nullptr);
    static void DrawMeshInstancing(Mesh& mesh, Material& mat, Matrix4* animMatrixs, int count);
    static void DrawMeshInstancing(Mesh& mesh, Material& shader, Matrix4x3* animMatrixs, int count);
    static void DrawMeshInstancing(Mesh& mesh, Material& mat, InstancingBuffer& buffer, int count);
    static void DrawModel(Model& model, Matrix4 modelMatrix);

    static void AddDrawLineCommand(Vector3 start, Vector3 end);
    static void DrawLinesComamnd(Vector3 color, int lineWidth);

    static void DrawLine(Vector3 start, Vector3 end, Vector3 color, int lineWidth);
    static void DrawLine(Matrix4 model, Vector3 start, Vector3 end, Vector3 color, int lineWidth);
    static void DrawWireCube(Matrix4 modelMatrix, Vector3 color, int lineWidth);

    static void DrawFullScreenQuad(Material& mat, Matrix4 modelMatrix);

    static void DrawText(Font& f, Material& s, std::string text, Matrix4 model, bool alignWithTop, const TextParams& params);

    static void DrawQuadPostProcessing(Framebuffer* src, Framebuffer* dst, Material& mat, int pass = 0);
    static void DrawQuadPostProcessing(Framebuffer* dst, Material& mat, int pass = 0);
    static void BlitFramebuffer(Framebuffer* src, Framebuffer* dst, int srcPass = 0);

    //TODO: Refacoty this, "remove bool clean = true", to avoid mistake bug when i no need to clean or need to clean
    static void BeginFramebuffer(Framebuffer& frambuffer, bool clean = true, Vector4 clearColor = Vector4(0, 0, 0, 1), int layer = 0, int mip = 0);
    static void EndFramebuffer();

    static void BeginGPUTime();
    static double EndGPUTime();

    static void CreateLuaBind(sol::state& lua);

    static GraphicsDevice* GetGraphicsDevice();

private:
    static void SelectGraphicsDevice();
    static void Initialize();
    static void Shutdown();
    static void _Begin();
    static void _End();

    static Gfx::BindGroup BindMaterial(Material& mat);
};

void GraphicsModuleInit();

}