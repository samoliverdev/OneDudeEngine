#include "UI.h"
#include "Standard/2D/Renderer2D.h"
#include <OD/Core/Application.h>
#include <OD/Core/Input.h>
#include <OD/Graphics/Graphics.h>
#include <OD/Graphics/Texture.h>
#include <OD/Graphics/Material.h>
#include <OD/Graphics/Font.h>

namespace Standard{
namespace UI{

void UIContext::Begin(){
    commands.clear();
    hoveredElement = -1;

    elements.clear();
    stack.clear();
    last = nullptr;
}

int UIContext::AddElement(){
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

UIElement& UIContext::OpenElement(){
    int index = AddElement();
    stack.push_back(index);
    _last = index;
    return elements[index];
}

void UIContext::CloseElement(){
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

Vector2 UIContext::ComputeAnchorPoint(const Vector2& parentSize, const Vector2& anchor){
    return Vector2(
        parentSize.x * anchor.x, // 0 left, .5 center, 1 right
        parentSize.y * anchor.y  // 0 top,  .5 center, 1 bottom
    );
}

Vector2 UIContext::GetDefaultPivotFromAnchor(Vector2 anchor){
    return anchor;
}

Vector2 UIContext::_GetSize(UIElement::Sizing size, Vector2 parentSize){
    float x = UIScale(size.width.value);
    float y = UIScale(size.height.value);

    if(size.width.type == UIElement::SizingType::Percent){
        x = parentSize.x * size.width.value;
    }

    if(size.height.type == UIElement::SizingType::Percent){
        y = parentSize.y * size.height.value;
    }

    return Vector2(x, y);
};

Vector2 UIContext::GetSize(int index, Vector2 parentSize){
    UIElement& el = elements[index];

    float x = UIScale(el.size.width.value);
    float y = UIScale(el.size.height.value);

    if(el.size.width.type == UIElement::SizingType::Percent){
        x = parentSize.x * el.size.width.value;
    }

    if(el.size.height.type == UIElement::SizingType::Percent){
        y = parentSize.y * el.size.height.value;
    }

    if(el.size.width.type == UIElement::SizingType::FitText){
        x = (baseFont->CalculateTextMetrics(el.text.c_str()).size * UIScale(el.fontSize)).x;
    }

    if(el.size.height.type == UIElement::SizingType::FitText){
        y = (baseFont->CalculateTextMetrics(el.text.c_str()).size * UIScale(el.fontSize)).y;
    }

    if(el.size.width.type == UIElement::SizingType::FitLayout){
        x = GetChildrenContentSize(index).x;
    }

    if(el.size.height.type == UIElement::SizingType::FitLayout){
        y = GetChildrenContentSize(index).y;
    }

    return Vector2(x, y);
}

/*
Vector2 UIContext::MeasureLayoutChildren(int parentIndex){
    UIElement& parent = elements[parentIndex];

    float totalMain = 0;
    float maxCross = 0;
    int count = 0;

    int child = parent.firstChild;
    while(child != -1){
        UIElement& c = elements[child];
        Vector2 size = GetSize(c.size, parent.finalSize);

        if(parent.layoutMode == UIElement::LayoutMode::Vertical){
            totalMain += size.x;
            maxCross = std::max(maxCross, size.y);
        } else {
            totalMain += size.y;
            maxCross = std::max(maxCross, size.x);
        }

        count++;
        child = c.nextSibling;
    }

    if(count > 1)
        totalMain += (count - 1) * parent.layout.childGap;

    if(parent.layoutMode == UIElement::LayoutMode::Vertical)
        return { totalMain, maxCross };

    return { maxCross, totalMain };
}
*/
Vector2 UIContext::MeasureLayoutChildren(int parentIndex){
    UIElement& parent = elements[parentIndex];

    float totalMain = 0;
    float maxCross = 0;
    int count = 0;

    int child = parent.firstChild;
    while(child != -1){
        UIElement& c = elements[child];
        Vector2 size = GetSize(child, parent.finalSize); //GetSize(c.size, parent.finalSize);

        if(parent.layoutMode == UIElement::LayoutMode::Vertical){
            totalMain += size.y;                 // vertical = sum height
            maxCross = std::max(maxCross, size.x); // cross = max width
        } else if(parent.layoutMode == UIElement::LayoutMode::Horizontal){
            totalMain += size.x;                 // horizontal = sum width
            maxCross = std::max(maxCross, size.y); // cross = max height
        }

        count++;
        child = c.nextSibling;
    }

    if(count > 1)
        totalMain += (count - 1) * UIScale(parent.layout.childGap);

    if(parent.layoutMode == UIElement::LayoutMode::Vertical)
        return { maxCross, totalMain };

    return { totalMain, maxCross };
}

Vector2 UIContext::GetChildrenContentSize(int parentIndex){
    UIElement& parent = elements[parentIndex];

    Vector2 childrenSize = MeasureLayoutChildren(parentIndex);

    UIElement::Padding& p = parent.layout.padding;

    childrenSize.x += UIScale(p.left + p.right);
    childrenSize.y += UIScale(p.top + p.bottom);

    return childrenSize;
}

void UIContext::ComputeElementLayout(int index){
    UIElement& el = elements[index];

    if(el.parent != -1){
        UIElement& parent = elements[el.parent];
        el.finalSize = GetSize(index, parent.finalSize); //GetSize(el.size, parent.finalSize);
    } else {
        el.finalSize = GetSize(index, GetCanvasSize()); //GetSize(el.size, GetCanvasSize());
    }

    if(el.parent != -1){
        UIElement& parent = elements[el.parent];

        if(parent.layoutMode == UIElement::LayoutMode::Absolute){
            //Vector2 anchorPoint = ComputeAnchorPoint(parent.finalSize, el.anchor);
            //el.finalPos = parent.finalPos + anchorPoint + el.pos;

            Vector2 anchorPoint = parent.finalPos + parent.finalSize * el.anchor;
            Vector2 pivotOffset = el.finalSize * el.pivot;
            el.finalPos = anchorPoint + UIScale(el.pos) - pivotOffset;
        } else {
            /*el.finalPos = parent.finalPos + parent.childOffset;

            if(parent.layoutMode == UIElement::LayoutMode::Horizontal){
                parent.childOffset.x += el.finalSize.x + UIScale(parent.layout.childGap);
            }
            if(parent.layoutMode == UIElement::LayoutMode::Vertical){
                parent.childOffset.y += el.finalSize.y + UIScale(parent.layout.childGap);
            }*/

            UIElement::Padding& p = parent.layout.padding;
            Vector2 contentSize = {
                parent.finalSize.x - (UIScale(p.left) + UIScale(p.right)),
                parent.finalSize.y - (UIScale(p.top) + UIScale(p.bottom))
            };

            if(parent.layoutMode == UIElement::LayoutMode::Horizontal){
                el.finalPos.x = parent.finalPos.x + parent.childOffset.x;
                el.finalPos.y = parent.finalPos.y + UIScale(p.top) + (contentSize.y - el.finalSize.y) * parent.layout.childAlignment.y;
                parent.childOffset.x += el.finalSize.x + UIScale(parent.layout.childGap);
            }

            if(parent.layoutMode == UIElement::LayoutMode::Vertical){
                el.finalPos.x = parent.finalPos.x + UIScale(p.left) + (contentSize.x - el.finalSize.x) * parent.layout.childAlignment.x;
                el.finalPos.y = parent.finalPos.y + parent.childOffset.y;
                parent.childOffset.y += el.finalSize.y + UIScale(parent.layout.childGap);
            }
        }
    } else {
        el.finalPos = UIScale(el.pos);

        //INFO: This is not working, for now root dont support anchor and pivot for now
        Vector2 anchorPoint = GetCanvasSize() * el.anchor;
        Vector2 pivotOffset = el.finalSize * el.pivot;
        el.finalPos = anchorPoint + UIScale(el.pos) - pivotOffset;
    }

    if(!el.text.empty()){
        Vector2 textSize = baseFont->CalculateTextMetrics(el.text.c_str()).size * UIScale(el.fontSize);
        //Vector2 textSize = UI::MeasureText(el.text.c_str(), baseFont, el.fontSize);

        el.finalTextPos = el.finalPos + (el.finalSize - textSize) * el.textAlign;

        UIElement::Padding& p = el.layout.padding;
        Vector2 contentPos = el.finalPos + Vector2((float)UIScale(p.left),(float)UIScale(p.top));
        Vector2 contentSize = el.finalSize - Vector2((float)UIScale(p.left + p.right), (float)UIScale(p.top + p.bottom));

        Vector2 freeSpace = contentSize - textSize;

        freeSpace.x = std::max(0.0f, freeSpace.x);
        freeSpace.y = std::max(0.0f, freeSpace.y);

        el.finalTextPos = contentPos + freeSpace * el.textAlign;
    }

    el.childOffset = {0, 0};

    if(el.layoutMode != UIElement::LayoutMode::Absolute){
        UIElement::Padding& p = el.layout.padding;

        Vector2 contentPos = {
            (float)UIScale(p.left),
            (float)UIScale(p.top)
        };

        Vector2 contentSize = {
            el.finalSize.x - (float)(UIScale(p.left) + UIScale(p.right)),
            el.finalSize.y - (float)(UIScale(p.top) + UIScale(p.bottom))
        };

        Vector2 childrenSize = MeasureLayoutChildren(index);

        Vector2 freeSpace = contentSize - childrenSize;

        freeSpace.x = std::max(0.0f, freeSpace.x);
        freeSpace.y = std::max(0.0f, freeSpace.y);

        el.childOffset = contentPos + freeSpace * el.layout.childAlignment;
    }

    el.childOffset += UIScale(el.clip.childOffset);
}

//////////////////
void UIContext::BuildCommands(int root){
    _stack.clear();
    _stack.push_back({root, false});

    while(!_stack.empty()){
        TraverseItem item = _stack.back();
        _stack.pop_back();

        int index = item.index;
        UIElement& el = elements[index];

        if(item.exit){
            if(HasClip(el)) commands.push_back({UICommandType::ScissorEnd, index});
            continue;
        }
        
        ComputeElementLayout(index);
        
        //------Building Cmds------
        if(HasClip(el))
            commands.push_back({UICommandType::ScissorBegin, index});

        commands.push_back({UICommandType::Panel, index});

        if(!el.text.empty())
            commands.push_back({UICommandType::Text, index});
        //-------------------

        // exit after children
        _stack.push_back({index, true});

        _children.clear();

        int child = el.firstChild;
        while(child != -1){
            _children.push_back(child);
            child = elements[child].nextSibling;
        }

        for(int i = (int)_children.size() - 1; i >= 0; --i)
            _stack.push_back({_children[i], false});
    }
}

void UIContext::BuildAllCommands(){
    commands.clear();

    for(int i = 0; i < (int)elements.size(); ++i){
        if(elements[i].parent == -1){
            BuildCommands(i);
        }
    }
}

void UIContext::UpdateHover(){
    hoveredElement = -1;

    double mx, my;
    Input::GetMousePosition(&mx, &my);
    Vector2 mouse = {(float)mx, (float)my};

    for(UICommand& cmd : commands){
        if(cmd.type != UICommandType::Panel)
            continue;

        UIElement& el = elements[cmd.element];

        if(PointInElement(mouse, el))
            hoveredElement = cmd.element;
    }

    if(Input::IsMouseButtonDown(MouseButton::Left))
        pressedElement = hoveredElement;
}

void UIContext::DrawCommands(){
    for(UICommand& cmd: commands){
        UIElement& el = elements[cmd.element];

        switch(cmd.type){
            case UICommandType::ScissorBegin:{
                Graphics::EnableScissor();

                int x = (int)roundf(el.finalPos.x);
                int y = (int)roundf(el.finalPos.y);
                int w = (int)roundf(el.finalSize.x);
                int h = (int)roundf(el.finalSize.y);

                // OpenGL bottom-left scissor
                int correctedY = (int)currentResolution.y - (y + h);

                Graphics::Scissor(x, correctedY, w, h);
                break;
            }

            case UICommandType::ScissorEnd:{
                Graphics::DisableScissor();
                break;
            }

            case UICommandType::Panel:{
                Renderer2D::DrawPanel(
                    el.tex,
                    el.finalPos,
                    el.finalSize,
                    {0, 0},
                    el.backgroundColor //cmd.element == hoveredElement ? el.backgroundColor + 0.25f : el.backgroundColor
                );
                break;
            }

            case UICommandType::Text:{
                Renderer2D::DrawText(
                    el.text.c_str(),
                    baseFont,
                    el.finalTextPos,
                    UIScale(el.fontSize),
                    {0, 0},
                    el.textColor
                );
                break;
            }
        }
    }
}

}
}