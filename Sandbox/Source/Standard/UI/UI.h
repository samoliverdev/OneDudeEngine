#pragma once
#include <OD/Base.h>

namespace OD{
    class Texture2D;
    class Font;
}

using namespace OD;

namespace Standard{
namespace UI{

struct UIElement{
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

    struct Sizing{
        SizingAxis width;
        SizingAxis height;

        template <class Archive>
        void serialize(Archive& ar){
            ArchiveDumpNVP(ar, width);
            ArchiveDumpNVP(ar, height);
        }
    };

    enum class LayoutDirection{ 
        LeftToRight, TopToBotton
    };

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

    struct ClipElementConfig{
        bool horizontal; // Clip overflowing elements on the X axis.
        bool vertical; // Clip overflowing elements on the Y axis.
        Vector2 childOffset; // Offsets the x,y positions of all child elements. Used primarily for scrolling containers.
    };

    ClipElementConfig clip;

    int parent = -1;
    int firstChild = -1;
    int nextSibling = -1;
    // Optional: cached last child to speed linking
    int lastChild = -1;

    // Data
    uint32_t id = 0;

    Vector4 textColor = {1, 1, 1, 1};
    Vector4 backgroundColor = {0, 0, 1, 1};
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

    UIElement& Clip(ClipElementConfig _clip){ clip = _clip; return *this; }
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
    UIElement& TextColor(Vector4 color){ textColor = color; return *this; }
};

inline auto FixedSize(float x){ return UIElement::SizingAxis{UIElement::SizingType::Fixed, x}; }
inline auto PercentSize(float x){ return UIElement::SizingAxis{UIElement::SizingType::Percent, x}; }

struct UIContext{
    enum class Scaling{
        None, ScreenMatch
    };

    struct TraverseItem{
        int index;
        bool exit;
    };

    enum class UICommandType{
        ScissorBegin,
        ScissorEnd,
        Panel,
        Text
    };

    struct UICommand{
        UICommandType type;
        int element = -1;
    };

    Scaling scaling = Scaling::None;
    Vector2 baseResolution = {1920, 1080}; //{1280, 720};
    Vector2 currentResolution = {1280, 720};

    std::vector<UIElement> elements;
    std::vector<int> stack;
    std::vector<int> drawOrder;

    std::vector<TraverseItem> _stack;
    std::vector<int> _children;
    std::vector<UICommand> commands;

    UIElement* last = nullptr;
    int _last = 0;

    Ref<Texture2D> baseTex = nullptr;
    Ref<Font> baseFont = nullptr;
    
    int hoveredElement = -1;
    int pressedElement = -1;

    inline bool PointInElement(Vector2 mouse, const UIElement& el){
        Vector2 pos = el.finalPos; //scaling == Scaling::ScreenMatch ? UIScale(el.finalPos) : el.finalPos;
        Vector2 size = el.finalSize; //scaling == Scaling::ScreenMatch ? UIScale(el.finalSize) : el.finalSize;
        return mouse.x >= pos.x && mouse.x <= pos.x + size.x && mouse.y >= pos.y && mouse.y <= pos.y + size.y;
    }

    inline float ComputeScale() const {
        float sx = currentResolution.x / baseResolution.x;
        float sy = currentResolution.y / baseResolution.y;
        return (sx + sy) * 0.5f;
        //return std::min(sx, sy);

        //float scale = std::min(sx, sy); Use the smaller value (preserve fit)
        //float scale = std::max(sx, sy); Use the bigger value (preserve readability)
    }

    inline float UIScale(float x) const {
        return scaling == Scaling::None ? x : roundf(x*ComputeScale()); 
    } 

    inline Vector2 UIScale(Vector2 p) const { 
        float s = ComputeScale();
        return scaling == Scaling::None ? p : Vector2(roundf(p.x * s), roundf(p.y * s));
    } 

    inline Vector2 GetCanvasSize() const {
        //if(scaling == Scaling::ScreenMatch) return baseResolution;
        //if(scaling == Scaling::ScreenMatch) return baseResolution;
        return currentResolution;
    }

    inline UIElement& Last(){
        return elements[_last];
    }

    inline bool HasClip(const UIElement& el){
        return el.clip.horizontal || el.clip.vertical;
    }

    void Begin();

    int AddElement();

    UIElement& OpenElement();

    void CloseElement();

    Vector2 ComputeAnchorPoint(const Vector2& parentSize, const Vector2& anchor);

    Vector2 GetDefaultPivotFromAnchor(Vector2 anchor);

    Vector2 GetSize(UIElement::Sizing size, Vector2 parentSize);

    Vector2 MeasureLayoutChildren(int parentIndex);

    void ComputeElementLayout(int index);

    //////////////////
    void BuildCommands(int root);

    void BuildAllCommands();

    void UpdateHover();

    void DrawCommands();
};

}
}