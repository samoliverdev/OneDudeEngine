#include "OD/pch.h"
#include "UI.h"
#include "LoadScene.h"
#include "Standard/Module.h"
#include "Standard/UI/CoreUI.h"
#include <OD/Core/Application.h>
#include <OD/Core/Input.h>
#include <OD/Graphics/Graphics.h>
#include <OD/Graphics/Texture.h>
#include <OD/Graphics/Material.h>
#include <OD/Graphics/Font.h>
#include <OD/Editor/Editor.h>

using namespace OD;
using namespace Standard;

enum class SizingType{
    Fixed, Percent
};

struct SizingAxis{
    SizingType type = SizingType::Fixed;
    float value = 0;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, type);
        ArchiveDumpNVP(ar, value);
    }
};

#define FixedSize(x) SizingAxis{SizingType::Fixed, x}
#define PercentSize(x) SizingAxis{SizingType::Percent, x}

struct Sizing{
    SizingAxis width;
    SizingAxis height;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, width);
        ArchiveDumpNVP(ar, height);
    }
};

enum class LayoutDirection{ LeftToRight, TopToBotton};

struct Padding{
    uint16_t left;
    uint16_t right;
    uint16_t top;
    uint16_t bottom;

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, left);
        ArchiveDumpNVP(ar, right);
        ArchiveDumpNVP(ar, top);
        ArchiveDumpNVP(ar, bottom);
    }
};

struct LayoutConfig{
    Padding padding;
    uint16_t childGap;
    Vector2 childAlignment; //(0=left/top, 0.5=center, 1=right/bottom)

    template <class Archive>
    void serialize(Archive& ar){
        ArchiveDumpNVP(ar, padding);
        ArchiveDumpNVP(ar, childGap);
        ArchiveDumpNVP(ar, childAlignment);
    }
};

enum class LayoutMode{
    Absolute,
    Horizontal,
    Vertical
};

struct UIElement{
    int parent = -1;
    int firstChild = -1;
    int nextSibling = -1;
    // Optional: cached last child to speed linking
    int lastChild = -1;

    // Data
    uint32_t id = 0;

    Vector4 textColor;
    Vector4 backgroundColor;
    Vector4 padding = {0, 0, 0, 0};
    Vector2 pos = {0, 0};
    Sizing size;
    Vector2 anchor = {0, 0};
    Vector2 pivot = {0, 0};
    Vector2 textAlign = {0, 0};
    
    Ref<Texture2D> tex = nullptr;

    LayoutMode layoutMode = LayoutMode::Absolute;
    LayoutConfig layout;

    std::string text;
    float fontSize = 1;

    Vector2 childOffset = {0, 0};
    Vector2 finalPos;
    Vector2 finalTextPos;
    Vector2 finalSize;

    UIElement& Pos(Vector2 p){ pos = p; return *this; }
    UIElement& Size(Sizing s){ size = s; return *this; }
    UIElement& Anchor(Vector2 a){ anchor = a; return *this; }
    UIElement& Pivot(Vector2 p){ pivot = p; return *this; }
    UIElement& AnchorPivot(Vector2 v){ anchor = v; pivot = v; return *this;}
    UIElement& Color(Vector4 color){ backgroundColor = color; return *this; }
    UIElement& Vertical(float gap = 0){ layoutMode = LayoutMode::Vertical; layout.childGap = gap; return *this; }
    UIElement& Horizontal(float gap = 0){ layoutMode = LayoutMode::Horizontal; layout.childGap = gap; return *this; }
    UIElement& PaddingAll(uint16_t v){ layout.padding = {v, v, v, v}; return *this; }
    UIElement& Align(Vector2 a){ layout.childAlignment = a; return *this; }
    UIElement& Text(const std::string& _text, float size, Vector2 align = {0, 0}){ text = _text; fontSize = size; textAlign = align; return *this; }
};

enum class Scaling{
    None, ScreenMatch
};

struct UIContext{
    Scaling scaling = Scaling::None;
    Vector2 baseResolution = {1920, 1080}; //{1280, 720};
    Vector2 currentResolution = {1280, 720};

    std::vector<UIElement> elements;
    std::vector<int> stack;

    std::vector<int> _stack;
    std::vector<int> _children;

    UIElement* last = nullptr;
    int _last = 0;

    Ref<Texture2D> baseTex = nullptr;
    Ref<Font> baseFont = nullptr;

    float lastXOffset = 0;
    float lastYOffset = 0;

    float ComputeScale() const {
        float sx = currentResolution.x / baseResolution.x;
        float sy = currentResolution.y / baseResolution.y;
        return (sx + sy) * 0.5f;
        //return std::min(sx, sy);

        //float scale = std::min(sx, sy); Use the smaller value (preserve fit)
        //float scale = std::max(sx, sy); Use the bigger value (preserve readability)
    }

    float UIScale(float x) const { 
        return roundf(x*ComputeScale()); 
    } 

    Vector2 UIScale(Vector2 p) const { 
        float s = ComputeScale();
        return { roundf(p.x * s), roundf(p.y * s) };
    } 

    Vector2 GetCanvasSize() const {
        //if(scaling == Scaling::ScreenMatch) return baseResolution;
        //if(scaling == Scaling::ScreenMatch) return baseResolution;
        return currentResolution;
    }

    UIElement& Last(){
        return elements[_last];
    }

    void Begin(){
        elements.clear();
        stack.clear();
        last = nullptr;
    }

    int AddElement(){
        int index = (int)elements.size();
        elements.emplace_back();
        UIElement& el = elements.back();

        if(!stack.empty()){
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

    UIElement& OpenElement(){
        int index = AddElement();
        stack.push_back(index);
        _last = index;
        return elements[index];
    }

    void CloseElement(){
        if(stack.empty()) return;
        stack.pop_back();
        return;

        UIElement& element = elements[stack[stack.size()-1]];
        if(element.parent == -1) return;

        UIElement& parent = elements[element.parent];

        /*Vector2 size = element.size;
        size.x += parent.padding.x + parent.padding.z;
        size.y += parent.padding.y + parent.padding.w;
        if(parent.layoutDirection == LayoutDirection::LeftToRight){
            parent.size.x += size.x;
            parent.size.y = math::max(size.y, parent.size.y);
        } else {
            parent.size.x = math::max(size.x, parent.size.x);
            parent.size.y += size.y;
        }*/

        if(!stack.empty()) stack.pop_back();
    }

    Vector2 ComputeAnchorPoint(const Vector2& parentSize, const Vector2& anchor){
        return Vector2(
            parentSize.x * anchor.x, // 0 left, .5 center, 1 right
            parentSize.y * anchor.y  // 0 top,  .5 center, 1 bottom
        );
    }

    Vector2 GetDefaultPivotFromAnchor(Vector2 anchor){
        return anchor;
    }

    Vector2 GetSize(Sizing size, Vector2 parentSize){
        float x = size.width.value;
        float y = size.height.value;

        if(size.width.type == SizingType::Percent){
            x = parentSize.x * size.width.value;
        }

        if(size.height.type == SizingType::Percent){
            y = parentSize.y * size.height.value;
        }

        return Vector2(x, y);
    };

    Vector2 MeasureLayoutChildren(int parentIndex){
        UIElement& parent = elements[parentIndex];

        float totalMain = 0;
        float maxCross = 0;
        int count = 0;

        int child = parent.firstChild;
        while(child != -1){
            UIElement& c = elements[child];
            Vector2 size = GetSize(c.size, parent.finalSize);

            if(parent.layoutMode == LayoutMode::Vertical){
                totalMain += /*c.*/size.x;
                maxCross = std::max(maxCross, /*c.*/size.y);
            } else {
                totalMain += /*c.*/size.y;
                maxCross = std::max(maxCross, /*c.*/size.x);
            }

            count++;
            child = c.nextSibling;
        }

        if(count > 1)
            totalMain += (count - 1) * parent.layout.childGap;

        if(parent.layoutMode == LayoutMode::Vertical)
            return { totalMain, maxCross };

        return { maxCross, totalMain };
    }

    void ComputeElementLayout(int index){
        UIElement& el = elements[index];

        if(el.parent != -1){
            UIElement& parent = elements[el.parent];
            el.finalSize = GetSize(el.size, parent.finalSize);
        } else {
            el.finalSize = GetSize(el.size, GetCanvasSize());
        }

        if(el.parent != -1){
            UIElement& parent = elements[el.parent];

            if(parent.layoutMode == LayoutMode::Absolute){
                //Vector2 anchorPoint = ComputeAnchorPoint(parent.finalSize, el.anchor);
                //el.finalPos = parent.finalPos + anchorPoint + el.pos;

                Vector2 anchorPoint = parent.finalPos + parent.finalSize * el.anchor;
                Vector2 pivotOffset = el.finalSize * el.pivot;
                el.finalPos = anchorPoint + el.pos - pivotOffset;
            } else {
                el.finalPos = parent.finalPos + parent.childOffset;

                if(parent.layoutMode == LayoutMode::Horizontal){
                    parent.childOffset.x += el.finalSize.x + parent.layout.childGap;
                }
                if(parent.layoutMode == LayoutMode::Vertical){
                    parent.childOffset.y += el.finalSize.y + parent.layout.childGap;
                }
            }
        } else {
            el.finalPos = el.pos;

            //INFO: This is not working, for now root dont support anchor and pivot for now
            /*Vector2 anchorPoint = GetCanvasSize() * el.anchor;
            Vector2 pivotOffset = el.finalSize * el.pivot;
            el.finalPos = anchorPoint + el.pos - pivotOffset;*/
        }

        if(!el.text.empty()){
            Vector2 textSize = baseFont->CalculateTextMetrics(el.text.c_str()).size * el.fontSize;
            //Vector2 textSize = UI::MeasureText(el.text.c_str(), baseFont, el.fontSize);

            el.finalTextPos = el.finalPos + (el.finalSize - textSize) * el.textAlign;

            Padding& p = el.layout.padding;
            Vector2 contentPos = el.finalPos + Vector2((float)p.left,(float)p.top);
            Vector2 contentSize = el.finalSize - Vector2((float)(p.left + p.right), (float)(p.top + p.bottom));

            Vector2 freeSpace = contentSize - textSize;

            freeSpace.x = std::max(0.0f, freeSpace.x);
            freeSpace.y = std::max(0.0f, freeSpace.y);

            el.finalTextPos = contentPos + freeSpace * el.textAlign;
        }

        el.childOffset = {0, 0};

        if(el.layoutMode != LayoutMode::Absolute){
            Padding& p = el.layout.padding;

            Vector2 contentPos = {
                (float)p.left,
                (float)p.top
            };

            Vector2 contentSize = {
                el.finalSize.x - (float)(p.left + p.right),
                el.finalSize.y - (float)(p.top + p.bottom)
            };

            Vector2 childrenSize = MeasureLayoutChildren(index);

            Vector2 freeSpace = contentSize - childrenSize;

            freeSpace.x = std::max(0.0f, freeSpace.x);
            freeSpace.y = std::max(0.0f, freeSpace.y);

            el.childOffset = contentPos + freeSpace * el.layout.childAlignment;
        }
    }

    void DrawElement(int index){
        UIElement& el = elements[index];

        // ----- Mouse Handling -----
        double mouseX, mouseY;
        Input::GetMousePosition(&mouseX, &mouseY);

        // Convert mouse Y (because UI Y axis is top-down)
        //mouseY = (double)cam.height - mouseY;

        //LogWarning("Mouse x: %f y:%f", mouseX, mouseY);

        //TODO: I think call UIScale on here is not the fully right way, becose percent and canvas size will not work well
        Vector2 pos = scaling == Scaling::ScreenMatch ? UIScale(el.finalPos) : el.finalPos;
        Vector2 size = scaling == Scaling::ScreenMatch ? UIScale(el.finalSize) : el.finalSize;

        // Hit test: simple AABB
        bool hovered = mouseX >= pos.x && mouseX <= (pos.x + size.x) && mouseY >= pos.y && mouseY <= (pos.y + size.y);
        bool clicked = hovered && Input::IsMouseButtonDown(MouseButton::Left);
            
        UI::DrawPanel(
            el.tex, pos, size, 0, 0, 
            hovered ? (el.backgroundColor + 0.25f) : el.backgroundColor
        );
        if(el.text.empty() == false){
            UI::DrawText(
                el.text.c_str(), baseFont, 
                scaling == Scaling::ScreenMatch ? UIScale(el.finalTextPos) : el.finalTextPos,
                scaling == Scaling::ScreenMatch ? UIScale(el.fontSize) : el.fontSize
            );
        }
    }

    void TraverseIterative(int root){
        //std::vector<int> _stack;
        _stack.clear();
        _stack.push_back(root);

        while(!_stack.empty()){
            int index = _stack.back();
            _stack.pop_back();

            UIElement& el = elements[index];

            // same code from Traverse(index)
            ComputeElementLayout(index);
            DrawElement(index);

            // Push children in reverse order so traversal stays first -> last
            //std::vector<int> children;
            _children.clear();

            int child = el.firstChild;
            while(child != -1){
                _children.push_back(child);
                child = elements[child].nextSibling;
            }

            for(int i = (int)_children.size() - 1; i >= 0; --i)
                _stack.push_back(_children[i]);
        }
    }

    void Traverse(int index){
        UIElement& el = elements[index];

        /*auto toSize = [](Sizing size, Vector2 parentSize){
            float x = size.width.value;
            float y = size.height.value;

            if(size.width.type == SizingType::Percent){
                x = parentSize.x * size.width.value;
            }

            if(size.height.type == SizingType::Percent){
                y = parentSize.y * size.height.value;
            }

            return Vector2(x, y);
        };

        if(el.parent != -1){
            UIElement& parent = elements[el.parent];
            el.finalSize = toSize(el.sizing, parent.finalSize);
        } else {
            el.finalSize = el.size;
        }

        if(el.parent != -1){
            UIElement& parent = elements[el.parent];

            if(parent.layoutMode == LayoutMode::Absolute){
                //Vector2 anchorPoint = ComputeAnchorPoint(parent.finalSize, el.anchor);
                //el.finalPos = parent.finalPos + anchorPoint + el.pos;

                Vector2 anchorPoint = parent.finalPos + parent.finalSize * el.anchor;
                Vector2 pivotOffset = el.finalSize * el.pivot;
                el.finalPos = anchorPoint + el.pos - pivotOffset;
            } else {
                el.finalPos = parent.finalPos + parent.childOffset;

                if(parent.layoutMode == LayoutMode::Horizontal){
                    parent.childOffset.x += el.finalSize.x + parent.layout.childGap;
                }
                if(parent.layoutMode == LayoutMode::Vertical){
                    parent.childOffset.y += el.finalSize.y + parent.layout.childGap;
                }
            }
        } else {
            el.finalPos = el.pos;
        }

        el.childOffset = {0, 0};

        if(el.layoutMode != LayoutMode::Absolute){
            Padding& p = el.layout.padding;

            Vector2 contentPos = {
                (float)p.left,
                (float)p.top
            };

            Vector2 contentSize = {
                el.finalSize.x - (float)(p.left + p.right),
                el.finalSize.y - (float)(p.top + p.bottom)
            };

            Vector2 childrenSize = MeasureLayoutChildren(index);

            Vector2 freeSpace = contentSize - childrenSize;

            freeSpace.x = std::max(0.0f, freeSpace.x);
            freeSpace.y = std::max(0.0f, freeSpace.y);

            el.childOffset = contentPos + freeSpace * el.layout.childAlignment;
        }

        // Process element here (layout, draw, etc.)
        UI::DrawPanel(el.tex, el.finalPos, el.finalSize, 0, 0, el.backgroundColor);*/

        ComputeElementLayout(index);
        DrawElement(index);

        int child = el.firstChild;
        while (child != -1){
            Traverse(child);
            child = elements[child].nextSibling;
        }
    }

    void TraverseAll(){
        for(int i = 0; i < (int)elements.size(); ++i){
            if(elements[i].parent == -1) TraverseIterative(i);
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
    font =  AssetManager::Get().LoadAsset<Font>("Engine/Fonts/OpenSans/static/OpenSans-Regular.ttf", FontSettings{8*3, FontType::MSDF});// OD::Font::CreateFromFile("SandboxGame/Fonts/Coolvetica/Coolvetica Rg Cond.otf", {8*3, FontType::MSDF});
    fontMat = OD::CreateRef<OD::Material>(OD::Shader::CreateFromFile("Engine/Shaders/FontMSDF.glsl"));
    fontMat->SetFloat("pxRange", font->MsdfPxRange());

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
    cy.baseFont = font;
    //cy.scaling = Scaling::ScreenMatch;
    cy.currentResolution = {uiCamera.width, uiCamera.height};
    cy.Begin();
    cy.OpenElement()
        .Pos({100, 100})
        .Size({FixedSize(300), FixedSize(300)})
        .Color({0, 0, 1, 1});
        cy.OpenElement()
            .Pos({-10, 10}).Size({FixedSize(100), FixedSize(200)}).AnchorPivot({1, 0})
            .Vertical(10).PaddingAll(10).Align({0, 0})
            .Color({1, 0, 1, 1});
            cy.OpenElement()
                .Size({FixedSize(50), FixedSize(50)})
                .Color({1, 1, 0, 1});
            cy.CloseElement();

            cy.OpenElement()
                .Size({FixedSize(50), PercentSize(0.15f)})
                .Color({1, 1, 0, 1});
            cy.CloseElement();

            cy.OpenElement()
                .Size({FixedSize(50), FixedSize(50)})
                .Color({1, 1, 0, 1});
            cy.CloseElement();
        cy.CloseElement();

        cy.OpenElement()
            .Pos({20, 0}).Size({FixedSize(100), PercentSize(0.75f)})
            .AnchorPivot({0, 0.5f}).Color({0.8f, 0.8f, 0.8f, 1});
        cy.CloseElement();
    cy.CloseElement();

    cy.OpenElement()
        .Pos({100, 500})
        //.AnchorPivot({1.0f, 0.5f})
        .Size({FixedSize(300), FixedSize(100)})
        .Text("Lolo", 75, {1.0f, 1.0f})
        .PaddingAll(5)
        .Color({1, 0, 0, 1});
    cy.CloseElement();

    //cy.Traverse(0);
    //cy.TraverseIterative(0);
    cy.TraverseAll();
    
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
