#include "OD/pch.h"
#include "UI.h"
#include "LoadScene.h"
#include "Standard/Module.h"
#include "Standard/UI/CoreUI.h"
#include <OD/Core/Application.h>
#include <OD/Graphics/Graphics.h>
#include <OD/Graphics/Texture.h>
#include <OD/Graphics/Material.h>
#include <OD/Editor/Editor.h>

using namespace OD;
using namespace Standard;

enum class LayoutDirection{ LeftToRight, TopToBotton};

struct UIElement{
    int parent = -1;
    int firstChild = -1;
    int nextSibling = -1;
    // Optional: cached last child to speed linking
    int lastChild = -1;

    // Data
    uint32_t id = 0;

    Vector4 backgroundColor;
    Vector4 padding = {0, 0, 0, 0};
    Vector2 pos = {0, 0};
    Vector2 size = {0, 0};
    Ref<Texture2D> tex = nullptr;
    float childGap = 0;
    LayoutDirection layoutDirection = LayoutDirection::LeftToRight;

    Vector2 finalPos;
    Vector2 finalSize;
};

struct UIContext{
    std::vector<UIElement> elements;
    std::vector<int> stack;

    UIElement* last = nullptr;

    Ref<Texture2D> baseTex = nullptr;

    float lastXOffset = 0;
    float lastYOffset = 0;

    void Begin(){
        elements.clear();
        stack.clear();
        last = nullptr;
    }

    int AddElement(){
        int index = (int)elements.size();
        elements.emplace_back();
        UIElement& el = elements.back();

        if (!stack.empty()){
            int parentIndex = stack.back();
            UIElement& parent = elements[parentIndex];

            el.parent = parentIndex;

            if(parent.firstChild == -1){
                parent.firstChild = index;
            } else {
                elements[parent.lastChild].nextSibling = index;
            }

            parent.lastChild = index;
        }

        last = &el;

        last->tex = baseTex;

        return index;
    }

    void OpenElement(){
        int index = AddElement();
        stack.push_back(index);
    }

    void CloseElement(){
        UIElement& element = elements[stack[stack.size()-1]];
        if(element.parent == -1) return;

        UIElement& parent = elements[element.parent];

        Vector2 size = element.size;
        size.x += parent.padding.x + parent.padding.z;
        size.y += parent.padding.y + parent.padding.w;
        if(parent.layoutDirection == LayoutDirection::LeftToRight){
            parent.size.x += size.x;
            parent.size.y = math::max(size.y, parent.size.y);
        } else {
            parent.size.x = math::max(size.x, parent.size.x);
            parent.size.y += size.y;
        }

        if(!stack.empty()) stack.pop_back();
    }

    void Traverse(int index){
        UIElement& el = elements[index];

        if(el.parent == -1){
            lastXOffset = el.padding.x;
            lastYOffset = el.padding.y;
        }

        el.finalPos = el.pos;
        el.finalSize = el.size;

        if(el.parent != -1){
            el.finalPos = el.pos + elements[el.parent].pos;
            //el.finalPos.x += elements[el.parent].padding.x;
            //el.finalPos.y += elements[el.parent].padding.y;
            el.finalPos.x += lastXOffset;
            el.finalPos.y += lastYOffset;
        }

        // Process element here (layout, draw, etc.)
        UI::DrawPanel(el.tex, el.finalPos, el.finalSize, 0, 0, el.backgroundColor);

        if(el.parent != -1){
            lastXOffset += el.size.x + elements[el.parent].childGap;
        }

        int child = el.firstChild;
        while (child != -1){
            Traverse(child);
            child = elements[child].nextSibling;
        }
    }
};

void UISample::OnInit(){
    LogInfo("Game Init");
    Application::Vsync(false);

    //Standard::ModuleInit();

    /*auto& SceneManager = SceneManager::Get();
    OD::Ref<OD::Scene> scene = SceneManager.NewScene();
    //Application::AddModule<Editor>();
    scene->Start();*/

    panelSprite = AssetManager::Get().LoadAsset<Texture2D>("Engine/Textures/White.jpg");

    UI::Init();
}

void UISample::OnUpdate(float deltaTime){}   

void UISample::OnRender(float deltaTime){
    Graphics::Begin();
    Graphics::SetViewport(0, 0, Application::ScreenWidth(), Application::ScreenHeight());
    Graphics::BeginRenderToScreen({0.0f, 0.0f, 0.0f, 0.0f});

    //----------UI----------
    auto uiCamera = Camera{
        OD::Matrix4Identity, 
        OD::math::ortho(0.0f, (float)Application::ScreenWidth(), 0.0f, (float)Application::ScreenHeight(), -10.0f, 10.0f)
    };
    uiCamera.width = Application::ScreenWidth();
    uiCamera.height = Application::ScreenHeight();

    UI::Begin(uiCamera);
    
    //UI::DrawPanel(panelSprite, {0, 0}, {300, 150});

    UIContext cy;
    cy.baseTex = panelSprite;
    cy.Begin();
    cy.OpenElement();
    cy.last->pos = {0, 0};
    cy.last->size = {0, 0};
    cy.last->padding = {32, 32, 32, 32};
    cy.last->childGap = 32;
    cy.last->backgroundColor = {0, 0, 255, 255};
        cy.OpenElement();
        cy.last->pos = {0, 0};
        cy.last->size = {300, 300};
        cy.last->backgroundColor = {255, 0, 255, 255};
        cy.CloseElement();

        cy.OpenElement();
        cy.last->pos = {0, 0};
        cy.last->size = {350, 200};
        cy.last->backgroundColor = {255, 255, 0, 255};
        cy.CloseElement();
    cy.CloseElement();
    cy.Traverse(0);
    
    UI::End();

    //----------------------
    Graphics::EndRenderToScreen();
    Graphics::End();
}

void UISample::OnExit(){
    UI::Shotdown();
}

void UISample::OnGUI(){}
void UISample::OnResize(int width, int height){}
