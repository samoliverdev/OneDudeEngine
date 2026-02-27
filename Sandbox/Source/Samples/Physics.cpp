#include "OD/pch.h"
#include "Physics.h"
#include "Ultis/Ultis.h"
#include "Ultis/CameraMovement.h"
#include <OD/Core/Application.h>
#include <OD/Core/Input.h>
#include <OD/Graphics/Model.h>
#include <OD/Scene/SceneManager.h>
#include <OD/RenderPipeline/EnvironmentComponent.h>
#include <OD/RenderPipeline/CameraComponent.h>
#include <OD/RenderPipeline/LightComponent.h>
#include <OD/RenderPipeline/ModelRendererComponent.h>
#include <OD/Physics/PhysicsSystem.h>
#include <OD/LuaScripting/LuaScripts.h>
#include <OD/RenderPipeline/UIComponents.h>
#include <OD/Editor/Editor.h>
//#include <entt/entt.hpp>
#include <sol/sol.hpp>

void PhysicsCubeS::OnStart(){
    sol::state lua;
    int x = 0;
    lua.set_function("beep", [&x]{ ++x; });
    lua.script("beep()");
    Assert(x == 1);

    LogInfo("PhysicsCubeS OnStart");
    
    Assert(scene->IsValid(entity) == true);

    Ref<Model> cubeModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/Cube.obj");
    //cubeModel->SetShader(AssetManager::GetGlobal()->LoadShaderFromFile("res/Builtins/Shaders/Unlit.glsl"));
    //cubeModel->materials[0].SetTexture("mainTex", AssetManager::GetGlobal()->LoadTexture2D("res/textures/rock.jpg", false, OD::TextureFilter::Linear, false));
    //cubeModel->materials[0].SetVector4("color", Vector4(1, 1, 1, 1));
    //for(auto& i:cubeModel->materials) i = LoadMaterial1();

    ModelRendererComponent& renderer = scene->AddOrGetComponent<ModelRendererComponent>(entity);
    renderer.SetModel(cubeModel);
    for(auto& i: renderer.GetMaterialsOverride()) i = LoadMaterial1();

    RigidbodyComponent& physicObject = scene->AddOrGetComponent<RigidbodyComponent>(entity);
    physicObject.NeverSleep(true);
    
    //physicObject->boxShapeSize = {1,1,1};
    //physicObject->mass = 1;

    physicObject.SetShape(CollisionShape::BoxShape({1,1,1}));
    //physicObject->SetMass(1);
}

void PhysicsCubeS::OnUpdate(){
    ///*
    t += Application::DeltaTime();
    if(t > timeToDestroy){
        scene->DestroyEntity(entity);
        //LogInfo("ToDestroy");
    }
    //*/
}

void PhysicsCubeS::OnDestroy(){
    LogInfo("PhysicsCubeS OnDestroy");
}

void PhysicsSample::OnInit(){
    LogInfo("%sGame Init %s", "\033[0;32m", "\033[0m");

    SceneManager::Get().RegisterScript<PhysicsCubeS>("PhysicsCubeS");
    SceneManager::Get().RegisterScript<CameraMovementScript>("CameraMovementScript");

    Ref<Scene> scene = SceneManager::Get().NewScene();

    /*scene->Load("test.scene");
    Application::AddModule<Editor>();
    return;*/

    /*Entity text = scene->AddEntity("Text");
    scene->GetComponent<TransformComponent>(text).LocalPosition(Vector3(25.0f, 25.0f, 0));
    TextRendererComponent& textRenderer = scene->AddComponent<TextRendererComponent>(text);
    textRenderer.text = "Ai meu cu!!!";
    textRenderer.color = {0.5f, 0.8f, 0.2f, 1.0f};
    textRenderer.font = Font::CreateFromFile("Engine/Fonts/OpenSans/static/OpenSans_Condensed-Bold.ttf");
    textRenderer.material = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Font.glsl"));*/

    /*Entity sprite = scene->AddEntity("Sprite");
    scene->GetComponent<TransformComponent>(sprite).LocalPosition(Vector3(0, 2, 0));
    SpriteRendererComponent& spriteRenderer = scene->AddComponent<SpriteRendererComponent>(sprite);
    spriteRenderer.sprite = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/character_1.png"); 
    spriteRenderer.color = {0.5f, 0.8f, 0.2f, 1.0f};
    //spriteRenderer.texture = CreateRef<Texture2D>("Sandbox/Textures/character_1.png", Texture2DSetting()); // Erro 
    spriteRenderer.material = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Sprite.glsl"));

    Entity canvas = scene->AddEntity("Canvas");
    CanvasComponent& can = scene->AddComponent<CanvasComponent>(canvas);

    Entity uiImage = scene->AddEntity("UIImage");
    RectTransformComponet& rect = scene->AddComponent<RectTransformComponet>(uiImage);
    rect.pos = {250, 250};
    rect.size = Vector2(300, 200);
    rect.anchors = Vector2(-1, -1);
    UIImageComponent& uiImageRenderer = scene->AddComponent<UIImageComponent>(uiImage);
    uiImageRenderer.sourceImage = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/image.jpg"); 
    uiImageRenderer.material = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Sprite.glsl"));
    scene->SetParent(canvas, uiImage);

    Entity uiImage2 = scene->AddEntity("UIImage2");
    RectTransformComponet& rect2 = scene->AddComponent<RectTransformComponet>(uiImage2);
    rect2.size = Vector2(100, 100);
    rect2.anchors = Vector2(1, 1);
    UIImageComponent& uiImageRenderer2 = scene->AddComponent<UIImageComponent>(uiImage2);
    uiImageRenderer2.sourceImage = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/block.png"); 
    uiImageRenderer2.material = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Sprite.glsl"));
    scene->SetParent(uiImage, uiImage2);

    Entity uiImage3 = scene->AddEntity("UIImage3");
    RectTransformComponet& rect3 = scene->AddComponent<RectTransformComponet>(uiImage3);
    rect3.pos = {25, 25};
    rect3.size = Vector2(100/2, 100/2);
    rect3.anchors = Vector2(1, 1);
    UIImageComponent& uiImageRenderer3 = scene->AddComponent<UIImageComponent>(uiImage3);
    uiImageRenderer3.sourceImage = AssetManager::Get().LoadAsset<Texture2D>("Sandbox/Textures/brickwall.jpg"); 
    uiImageRenderer3.material = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Sprite.glsl"));
    scene->SetParent(uiImage2, uiImage3);

    Entity uiText = scene->AddEntity("UiText");
    RectTransformComponet& rect4 = scene->AddComponent<RectTransformComponet>(uiText);
    rect4.pos = {0, 0};
    rect4.size = Vector2(2, 2);
    rect4.anchors = Vector2(1, 1);
    UITextComponent& uiTextRenderer = scene->AddComponent<UITextComponent>(uiText);
    uiTextRenderer.text = "Lolo";
    uiTextRenderer.color = {0.5f, 0.8f, 0.2f, 1.0f};
    uiTextRenderer.font = Font::CreateFromFile("Engine/Fonts/OpenSans/static/OpenSans_Condensed-Bold.ttf");
    uiTextRenderer.material = CreateRef<Material>(Shader::CreateFromFile("Engine/Shaders/Font.glsl"));
    scene->SetParent(uiImage3, uiText);*/

    Entity env = scene->AddEntity("Env");
    scene->AddComponent<EnvironmentComponent>(env).settings.ambient = Color{0.11f, 0.16f, 0.25f, 1};

    Entity light = scene->AddEntity("Light");
    LightComponent& lightComponent = scene->AddComponent<LightComponent>(light);
    lightComponent.color = {1,1,1};
    scene->GetComponent<TransformComponent>(light).Position(Vector3(-2, 4, -1));
    scene->GetComponent<TransformComponent>(light).LocalEulerAngles(Vector3(45, -125, 0));
    lightComponent.renderShadow = false;

    camera = scene->AddEntity("Camera");
    CameraComponent& cam = scene->AddComponent<CameraComponent>(camera);
    scene->GetComponent<TransformComponent>(camera).LocalPosition(Vector3(0, 15, 15));
    scene->GetComponent<TransformComponent>(camera).LocalEulerAngles(Vector3(-25, 0, 0));
    scene->AddComponent<ScriptComponent>(camera).AddScript<CameraMovementScript>()->moveSpeed = 60;
    //camMove.transform = &camera->transform();
    //camMove.moveSpeed = 60;
    //camMove.OnInit();
    cam.farClipPlane = 1000;

    Ref<Model> floorModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/plane.obj");
    Ref<Model> cubeModel = AssetManager::Get().LoadAsset<Model>("Sandbox/Models/Cube.obj");

    Entity floorEntity = scene->AddEntity("Floor");
    ModelRendererComponent& floorRenderer = scene->AddComponent<ModelRendererComponent>(floorEntity);
    floorRenderer.SetModel(floorModel);
    for(auto& i: floorRenderer.GetMaterialsOverride()) i = LoadFloorMaterial();
    //floorRenderer.GetMaterialsOverride()[1] = LoadFloorMaterial();
    RigidbodyComponent& floorEntityP = scene->AddComponent<RigidbodyComponent>(floorEntity);
    floorEntityP.SetShape(CollisionShape::BoxShape({25,0.1f,25}));
    floorEntityP.Mass(0);
    floorEntityP.SetType(RigidbodyComponent::Type::Static);
    floorEntityP.NeverSleep(true);
    //floorEntityP->entity()->transform().localEulerAngles({0,0,-25});

    Entity character2Entity = scene->AddEntity("MainCube");
    ModelRendererComponent& character2Renderer = scene->AddComponent<ModelRendererComponent>(character2Entity);
    character2Renderer.SetModel(cubeModel);
    for(auto& i: character2Renderer.GetMaterialsOverride()) i = LoadRockMaterial();
    //character2Renderer.GetMaterialsOverride()[1] = LoadRockMaterial();
    RigidbodyComponent& physicObject = scene->AddComponent<RigidbodyComponent>(character2Entity);
    physicObject.SetShape(CollisionShape::BoxShape({1,1,1}));
    physicObject.Mass(1);
    physicObject.NeverSleep(true);
    scene->GetComponent<TransformComponent>(character2Entity).Position({2, 13, 0});
    scene->GetComponent<TransformComponent>(character2Entity).Rotation(QuaternionIdentity);

    /*Entity character2Entity2 = scene->AddEntity("MainCube2");
    ModelRendererComponent& character2Renderer2 = character2Entity2.AddComponent<ModelRendererComponent>();
    character2Renderer2.SetModel(cubeModel);
    character2Renderer2.GetMaterialsOverride()[0] = LoadRockMaterial();
    RigidbodyComponent& physicObject2 = character2Entity2.AddComponent<RigidbodyComponent>();
    physicObject2.SetShape(CollisionShape::BoxShape({1,1,1}));
    physicObject2.Mass(1);
    physicObject2.NeverSleep(true);
    character2Entity2.GetComponent<TransformComponent>().Position({-3, 13, 0});
    character2Entity2.GetComponent<TransformComponent>().Rotation(QuaternionIdentity);
    JointComponent& joint = character2Entity2.AddComponent<JointComponent>();
    joint.pivot = Vector3{-3, 13, 0};
    joint.rb = character2Entity2.Id();*/

    /*Entity trigger = scene->AddEntity("Trigger");
    RigidbodyComponent& _trigger = scene->AddComponent<RigidbodyComponent>(trigger);
    _trigger.SetShape(CollisionShape::BoxShape({4,1,4}));
    _trigger.SetType(RigidbodyComponent::Type::Trigger);
    _trigger.NeverSleep(true);*/

    // Fixme: Not Work why play mode clone the scene and theirs system, Work only if Start Scene now
    /*scene->GetSystem<PhysicsSystem>()->AddOnTriggerEnterCallback([](Scene& scene, Entity trigger, Entity other){
        LogWarning("OnTrigger");
        scene.GetComponent<RigidbodyComponent>(other).ApplyImpulse(Vector3Up * 25.0f);
        //scene->GetComponent<RigidbodyComponent>(other).ApplyImpulse(Vector3Up * 25.0f);
    });*/

    Entity canvas = scene->AddEntity("Canvas");
    auto& canvasComp = scene->AddComponent<CanvasComponent>(canvas);
    canvasComp.scaleMode = CanvasComponent::ScaleMode::ScaleWithScreenSize;
    canvasComp.screenMatchMode = CanvasComponent::ScreenMatchMode::MatchWidthOrHeight;
    canvasComp.matchValue = 0.5f;
    scene->AddComponent<RectTransformComponent>(canvas).SetRectStretch(
        {0.0f, 0.0f}, // anchorMin
        {1.0f, 1.0f}, // anchorMax
        {0.0f, 0.0f}, // offsetMin
        {0.0f, 0.0f}, // offsetMax
        {0.0f, 0.0f}  // pivot
    );

    // Create panel
    Entity panel = scene->AddEntity("RootPanel");
    auto& panelRect = scene->AddComponent<RectTransformComponent>(panel);
    scene->AddComponent<UIImageComponent>(panel);
    panelRect.SetRect({1.0f, 0.5f}, {0.5f, 0.5f}, {-150, 0}, {200, 100});

    Entity button = scene->AddEntity("Button1");
    scene->AddComponent<RectTransformComponent>(button).SetRect({0.5f, 1.0f}, {0.5f, 0.5f}, {0, 0}, {100, 50});
    scene->AddComponent<UIImageComponent>(button).color = {1, 0.92, 0.016, 1};

    Entity text = scene->AddEntity("Text");
    scene->AddComponent<RectTransformComponent>(text).SetRect({0.5f, 0.5f}, {0.5f, 0.5f}, {0, 0}, {100, 50});
    auto& uiTex = scene->AddComponent<UITextComponent>(text);
    uiTex.text = "Test";
    uiTex.scale = 60;

    scene->SetParent(canvas, panel);
    scene->SetParent(panel, button);
    scene->SetParent(button, text);

    Entity luaScript = scene->AddEntity("LuaScript");
    LuaScriptComponent& _luaScript = scene->AddComponent<LuaScriptComponent>(luaScript);
    _luaScript.scriptPath = "Sandbox/LuaScripts/Test.lua";

    Entity luaScript2 = scene->AddEntity("LuaScript2");
    LuaScriptComponent& _luaScript2 = scene->AddComponent<LuaScriptComponent>(luaScript2);
    _luaScript2.scriptPath = "Sandbox/LuaScripts/Test2.lua";

    //scene->Save("res/scene1.scene");

    //scene->Save("test.scene", EntityNull);
    
    //scene->Start();
    Application::AddModule<Editor>(true);

    //LogInfo("Testdff!!!!!!!!!!!!");

    /*typedef Module* (*CreateInstanceFunc)();
    void* module = Platform::LoadDynamicLibrary("build/Release/dynamic_module.dll");
    CreateInstanceFunc func = (CreateInstanceFunc)Platform::LoadDynamicFunction(module, "CreateInstance");
    Application::AddModule(func());*/
}

void PhysicsSample::OnUpdate(float deltaTime){
    Ref<Scene> scene = SceneManager::Get().GetActiveScene();
    //scene->Update();
    if(scene->Running() == false) return;

    TransformComponent& camT = scene->GetComponent<TransformComponent>(camera);
    RayResult hit;
    //Throwing a Possible Null Expection Pointer Here
    if(scene->GetSystem<PhysicsSystem>()->Raycast(camT.Position(), camT.Back() * 1000.0f, hit)){
        LogInfo("Hitting: {}", scene->GetComponent<InfoComponent>(hit.entity).name);
    }

    /*Assert(scene->GetRegistry().ctx().get<PhysicsSystem*>() == scene->GetSystem<PhysicsSystem>());
    auto physicsSystem = scene->GetRegistry().ctx().get<PhysicsSystem*>();
    if(physicsSystem->Raycast(camT.Position(), camT.Back() * 1000.0f, hit)){
        LogInfo("Hitting: %s", hit.entity.GetComponent<InfoComponent>().name.c_str());
    }*/

    if(Input::IsKeyDown(KeyCode::R)){
        Entity e = SceneManager::Get().GetActiveScene()->AddEntity("PhysicsCube");
        scene->GetComponent<TransformComponent>(e).Position({2, 13, 0});
        scene->GetComponent<TransformComponent>(e).Rotation(QuaternionIdentity);
        scene->AddComponent<ScriptComponent>(e).AddScript<PhysicsCubeS>()->timeToDestroy = 100000000;
    }

    if(Input::IsKeyDown(KeyCode::T)){
        Entity e = SceneManager::Get().GetActiveScene()->InstantiatePrefab("Sandbox/test.prefab");
        scene->GetComponent<TransformComponent>(e).Position({2, 13, 0});
        scene->GetComponent<TransformComponent>(e).Rotation(QuaternionIdentity);
        scene->AddComponent<ScriptComponent>(e).AddScript<PhysicsCubeS>();
    }

    if(Input::IsKeyDown(KeyCode::A)){
        scene->Save("C:/Users/sam/Desktop/Test.scene", EntityNull);
    }
}   

void PhysicsSample::OnRender(float deltaTime){
    //SceneManager::Get().GetActiveScene()->Draw();
    //scene->GetSystem<PhysicsSystem>()->ShowDebugGizmos();
}

namespace UI{

#include <algorithm>

static ImVec2 operator+(const ImVec2& a, const ImVec2& b){
    return {a.x + b.x, a.y + b.y};
}

//======================================================
//  CONFIG: YOUR BASE REFERENCE RESOLUTION
//======================================================
static ImVec2 GUI_BASE_RESOLUTION(1920, 1080);

//======================================================
//  SCALE UTILITIES
//======================================================
static inline float GUI_ScaleX(float x, const ImVec2& screen){
    return x * (screen.x / GUI_BASE_RESOLUTION.x);
}

static inline float GUI_ScaleY(float y, const ImVec2& screen){
    return y * (screen.y / GUI_BASE_RESOLUTION.y);
}

float GUI_ComputeImGuiScale(const ImVec2& screen){
    float sx = screen.x / GUI_BASE_RESOLUTION.x;
    float sy = screen.y / GUI_BASE_RESOLUTION.y;
    return (sx + sy) * 0.5f;

    //float scale = std::min(sx, sy); Use the smaller value (preserve fit)
    //float scale = std::max(sx, sy); Use the bigger value (preserve readability)
}

static inline ImVec2 GUI_Scale(const ImVec2& v, const ImVec2& screen){
    //return ImVec2(GUI_ScaleX(v.x, screen), GUI_ScaleY(v.y, screen));

    float scale = GUI_ComputeImGuiScale(screen);
    return ImVec2(v.x * scale, v.y * scale);
}

struct GUIAnchor{
    float ax;   // 0 = left, 0.5 = center, 1 = right
    float ay;   // 0 = top,  0.5 = center, 1 = bottom

    constexpr GUIAnchor(float x = 0, float y = 0) : ax(x), ay(y) {}

    // presets
    static GUIAnchor TopLeft()      { return {0.0f, 0.0f}; }
    static GUIAnchor TopRight()     { return {1.0f, 0.0f}; }
    static GUIAnchor BottomLeft()   { return {0.0f, 1.0f}; }
    static GUIAnchor BottomRight()  { return {1.0f, 1.0f}; }
    static GUIAnchor Center()       { return {0.5f, 0.5f}; }
    static GUIAnchor CenterLeft()   { return {0.0f, 0.5f}; }
    static GUIAnchor CenterRight()  { return {1.0f, 0.5f}; }
};

static ImVec2 GUI_ApplyAnchor(ImVec2 pos, ImVec2 size, const GUIAnchor& a, const ImVec2& screen){
    // anchorX moves reference position along screen width
    pos.x = screen.x * a.ax - size.x * a.ax + pos.x;

    // anchorY moves reference position along screen height
    pos.y = screen.y * a.ay - size.y * a.ay + pos.y;

    return pos;
}

struct GUIElement{
    ImVec2 position;     // base resolution units
    ImVec2 size;         // base resolution units
    GUIAnchor anchor = GUIAnchor::TopLeft();
    bool visible = true;

    // NEW: hierarchical UI
    GUIElement* parent = nullptr;
    std::vector<GUIElement*> children;

    GUIElement(){}
    GUIElement(ImVec2 pos, ImVec2 size, GUIAnchor anch = GUIAnchor::TopLeft())
        : position(pos), size(size), anchor(anch){}

    virtual ~GUIElement(){}

    void AddChild(GUIElement* child){
        child->parent = this; //shared_from_this();
        children.push_back(child);
    }

    //----------------------------------
    // Calculate final position
    // If has parent: relative to parent
    // Otherwise: relative to screen
    //----------------------------------
    ImVec2 CalcFinalPos(const ImVec2& screen){
        // Convert our position to pixel units
        ImVec2 scaledPos = GUI_Scale(position, screen);
        ImVec2 scaledSize = GUI_Scale(size, screen);

        // Check if has parent
        if(parent != nullptr) //auto p =  parent.lock())
        {
            // parent pixel position
            ImVec2 pFinal = parent->CalcFinalPos(screen);

            // parent pixel size
            ImVec2 pSizePx = GUI_Scale(parent->size, screen);

            // anchor relative to parent local rectangle
            ImVec2 anchored = GUI_ApplyAnchor(
                pFinal + scaledPos,
                scaledSize,
                anchor,
                pSizePx
            );    // parent area
            return anchored;
        } else {
            // Root element — use the screen size as reference
            ImVec2 anchored = GUI_ApplyAnchor(
                scaledPos,
                scaledSize,
                anchor,
                screen
            );
            return anchored;
        }
    }

    //----------------------------------
    // Main draw — base class draws nothing but calls children
    //----------------------------------
    virtual void Draw(ImDrawList* list, const ImVec2& screen){
        if (!visible) return;

        // Draw children
        for(auto& c : children)
            if(c) c->Draw(list, screen);
    }
};

// -----------------------------
static inline ImVec2 GUI_Unscale(const ImVec2& scaledPixels, const ImVec2& screen){
    return ImVec2(
        scaledPixels.x * (GUI_BASE_RESOLUTION.x / screen.x),
        scaledPixels.y * (GUI_BASE_RESOLUTION.y / screen.y)
    );
}

struct GUIText : public GUIElement{
    std::string text;
    ImU32 color = IM_COL32(255,255,255,255);
    float fontSize = 24.0f; // base size (units in BASE resolution)
    float wrapWidth = 0.0f; // 0 = no wrap

    GUIText(
        const std::string& txt,
        float fontSizeBase,
        ImVec2 pos,
        GUIAnchor anchor = GUIAnchor::TopLeft(),
        ImU32 color = IM_COL32(255,255,255,255)
    ): 
        GUIElement(pos, ImVec2(0,0), anchor),
        text(txt),
        color(color),
        fontSize(fontSizeBase),
        wrapWidth(0.0f)
    {}

    virtual void Draw(ImDrawList* list, const ImVec2& screen) override{
        /*if (!visible) return;

        // 1) compute scaled font size in pixels (scale by Y to keep aspect)
        float scaledFontPx = GUI_ScaleY(fontSize, screen);

        // 2) measure text size (in pixels) using ImFont (respects scaledFontPx)
        ImFont* font = ImGui::GetFont(); // current font
        // Use ImFont::CalcTextSizeA to measure precisely at the wanted pixel size
        ImVec2 textSizePx = font->CalcTextSizeA(
            scaledFontPx,
            (wrapWidth > 0.0f) ? GUI_ScaleX(wrapWidth, screen) : FLT_MAX,
            0.0f,
            text.c_str()
        );

        // 3) update this element size (store in BASE resolution units)
        // Convert measured pixel size back into base-resolution units so the element's
        // size member remains expressed in the same base units as other elements.
        this->size = GUI_Unscale(textSizePx, screen);

        // 4) compute final position using the element base-position + anchor
        // CalcFinalPos() uses GUI_Scale(position, screen) and GUI_Scale(size, screen)
        ImVec2 scaledPos = GUI_Scale(position, screen);
        ImVec2 scaledSize = textSizePx; // we already have size in pixels
        ImVec2 finalPos = GUI_ApplyAnchor(scaledPos, scaledSize, anchor, screen);

        // 5) submit text using the measured font size
        // Use ImDrawList::AddText overload that takes font & size for consistent measurement
        list->AddText(font, scaledFontPx, finalPos, color, text.c_str());
        */

        if(!visible) return;

        float scaledFontPx = GUI_ScaleY(fontSize, screen);

        ImFont* font = ImGui::GetFont();
        ImVec2 textSizePx = font->CalcTextSizeA(
            scaledFontPx, 
            FLT_MAX, 
            0.0f, 
            text.c_str()
        );

        // element size stored in BASE resolution units
        size = GUI_Unscale(textSizePx, screen);

        // NEW — parent-aware position
        ImVec2 finalPos = CalcFinalPos(screen);

        // draw text
        list->AddText(font, scaledFontPx, finalPos, color, text.c_str());

        // draw children (if any)
        GUIElement::Draw(list, screen);

    }
};

//======================================================
//  GUI BOX RECT
//======================================================
struct GUIBox : public GUIElement{
    ImU32 color;
    float rounding;

    GUIBox(ImVec2 pos, ImVec2 size, GUIAnchor anchor, ImU32 color, float rounding = 6.0f)
        : GUIElement(pos, size, anchor), color(color), rounding(rounding){}

    virtual void Draw(ImDrawList* list, const ImVec2& screen) override{
        if(!visible) return;

        ImVec2 pos = CalcFinalPos(screen);
        ImVec2 s = GUI_Scale(size, screen);

        list->AddRectFilled(
            pos, ImVec2(pos.x + s.x, pos.y + s.y),
            color, rounding
        );

        GUIElement::Draw(list, screen);                           
    }
};

//======================================================
//  GUI BAR (HEALTH, AMMO, ETC.)
//======================================================
struct GUIBar : public GUIElement{
    float fraction = 1.0f;
    ImU32 bgColor;
    ImU32 fillColor;
    float rounding;

    GUIBar(
        ImVec2 pos, ImVec2 size, GUIAnchor anchor,
        ImU32 bgColor, ImU32 fillColor
    ): 
        GUIElement(pos, size, anchor),
        bgColor(bgColor), fillColor(fillColor),
        rounding(3.0f)
    {}

    virtual void Draw(ImDrawList* list, const ImVec2& screen) override{
        if(!visible) return;

        ImVec2 pos = CalcFinalPos(screen);
        ImVec2 s = GUI_Scale(size, screen);

        list->AddRectFilled(pos, ImVec2(pos.x + s.x, pos.y + s.y), bgColor, rounding); // Background
        list->AddRectFilled(pos, ImVec2(pos.x + s.x * fraction, pos.y + s.y), fillColor, rounding); // Fill
    }
};

static ImVec2 GUI_FixOffsetForAnchor(const ImVec2& pos, const GUIAnchor& anchor){
    ImVec2 result = pos;
    if(anchor.ax > 0.5f) result.x = -pos.x; // If anchor.x is on the RIGHT ( > 0.5 ), invert X offset
    if(anchor.ay > 0.5f) result.y = -pos.y; // If anchor.y is on the BOTTOM ( > 0.5 ), invert Y offset
    return result;
}

static ImVec2 GUI_FixOffsetForAnchor2(const ImVec2& pos, const GUIAnchor& a){
    // Smooth continuous mapping:
    // anchor = 0   → +1
    // anchor = 0.5 → 0
    // anchor = 1   → -1
    float sx = (0.5f - a.ax) * 2.0f;
    float sy = (0.5f - a.ay) * 2.0f;
    return ImVec2(pos.x * sx, pos.y * sy);
}

class GUIImguiWindow : public GUIElement{
public:
    std::string name;
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    GUIImguiWindow(
        const std::string& winName,
        ImVec2 pos,
        ImVec2 size,
        GUIAnchor anchor
    ):GUIElement(pos, size, anchor), name(winName){
        //name = winName;
        /*this->position = localPos;
        this->size = size;
        this->anchor = anchor;*/
    }

    std::function<void()> drawContents;

    /*void Draw(ImDrawList* list, const ImVec2& screen) override{
        ImVec2 pos = CalcFinalPos(screen);
        ImVec2 s = GUI_Scale(size, screen);

        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(s);

        ImGui::Begin(name.c_str(), nullptr, flags);
        if(drawContents) drawContents();
        ImGui::End();
    }*/

    void Draw(ImDrawList* list, const ImVec2& screen){
        ImVec2 pos = CalcFinalPos(screen);
        ImVec2 s   = GUI_Scale(size, screen);

        // Our custom scale factor based on screen size
        float scale = GUI_ComputeImGuiScale(screen); //CalcScale(screen);

        ImGuiStyle& style = ImGui::GetStyle();

        // Backup style
        ImGuiStyle backup = style;

        // Apply window-only scaling
        style.ScaleAllSizes(scale);
        //ImGui::GetIO().FontGlobalScale = scale * 2;
        //customFont->Scale = scale * 2;
        //ImGui::PushFont(customFont);

        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(s);

        ImGui::Begin(name.c_str(), nullptr, flags);

        if(drawContents)
            drawContents();  // default ImGui widgets now appear scaled

        ImGui::End();

        // Restore style so other windows aren't affected
        style = backup;
        //ImGui::GetIO().FontGlobalScale = 1.0f;
        //ImGui::PopFont();
    }
};
}

void PhysicsSample::OnGUI(){
    using namespace UI;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();// GetForegroundDrawList();
    ImVec2 screen = ImGui::GetIO().DisplaySize;

    auto pos = ImVec2(30, 30);   // always positive
    auto anchor = GUIAnchor(0, 0.5f); // bottom-right
    pos = GUI_FixOffsetForAnchor2(pos, anchor);
    GUIBox panel(pos, ImVec2(300, 100), anchor, IM_COL32(0,0,0,160));
    
    // Health label
    GUIText tHealth("HEALTH", 30, ImVec2(30, 10), GUIAnchor::TopLeft(), IM_COL32(255,160,0,255));

    // Health bar
    GUIBar health({30, 50}, ImVec2(300 - 60, 30), GUIAnchor::TopLeft(), IM_COL32(0,0,0,100), IM_COL32(255,160,0,255));
    health.fraction = 100.0f / 100.f;

    panel.AddChild(&tHealth);
    panel.AddChild(&health);
    panel.Draw(dl, screen);


    //bool t = true;
    //ImGui::ShowDemoWindow(&t);

    /*
    ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    static bool b = true;
    ImGui::ShowDemoWindow(&b);

    //ImGuiWindowFlags window_flags = 0;
    //window_flags |= ImGuiWindowFlags_NoBackground;
    //window_flags |= ImGuiWindowFlags_NoTitleBar;

    static bool b2 = true;
    ImGui::Begin("Entities", &b2);

    auto view = scene->GetRegistry().view<TransformComponent, InfoComponent>();
    for(auto e: view){
        TransformComponent& transform = view.get<TransformComponent>(e);
        InfoComponent& info = view.get<InfoComponent>(e);

        ImGui::Text(info.name.c_str());
    }

    ImGui::End();
    */
}

void PhysicsSample::OnResize(int width, int height){}
void PhysicsSample::OnExit(){}