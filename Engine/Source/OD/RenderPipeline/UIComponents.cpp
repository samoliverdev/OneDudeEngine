#include "OD/pch.h"
#include "UIComponents.h"
#include "OD/Core/ImGui.h"

namespace OD{

/*int GetAnchorPresetIndex(const Vector2& min, const Vector2& max, const Vector2& pivot) {
    struct Preset { Vector2 min, max, pivot; };
    static Preset presets[] = {
        {{0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f}}, // Top Left
        {{0.5f, 1.0f}, {0.5f, 1.0f}, {0.5f, 1.0f}}, // Top Center
        {{1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f}}, // Top Right
        {{0.0f, 0.5f}, {0.0f, 0.5f}, {0.0f, 0.5f}}, // Middle Left
        {{0.5f, 0.5f}, {0.5f, 0.5f}, {0.5f, 0.5f}}, // Middle Center
        {{1.0f, 0.5f}, {1.0f, 0.5f}, {1.0f, 0.5f}}, // Middle Right
        {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}}, // Bottom Left
        {{0.5f, 0.0f}, {0.5f, 0.0f}, {0.5f, 0.0f}}, // Bottom Center
        {{1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f}}, // Bottom Right
        {{0.0f, 1.0f}, {1.0f, 1.0f}, {0.5f, 1.0f}}, // Stretch Top
        {{0.0f, 0.5f}, {1.0f, 0.5f}, {0.5f, 0.5f}}, // Stretch Middle
        {{0.0f, 0.0f}, {1.0f, 0.0f}, {0.5f, 0.0f}}, // Stretch Bottom
        {{0.5f, 0.0f}, {0.5f, 1.0f}, {0.5f, 0.5f}}, // Stretch Vertical
        {{0.0f, 0.5f}, {1.0f, 0.5f}, {0.5f, 0.5f}}, // Stretch Horizontal
        {{0.0f, 0.0f}, {1.0f, 1.0f}, {0.5f, 0.5f}}, // Stretch Full
    };

    for (int i = 0; i < 15; ++i) {
        if (min == presets[i].min && max == presets[i].max && pivot == presets[i].pivot)
            return i;
    }
    return -1;
}

void RectTransformComponent::OnGui(Entity& e, Scene& scene) {
    RectTransformComponent& rect = scene.GetComponent<RectTransformComponent>(e);
    bool changed = false;

    if (ImGui::TreeNode("Rect Transform")) {
        static const char* anchorPresets[] = {
            "Top Left", "Top Center", "Top Right",
            "Middle Left", "Middle Center", "Middle Right",
            "Bottom Left", "Bottom Center", "Bottom Right",
            "Stretch Top", "Stretch Middle", "Stretch Bottom",
            "Stretch Vertical", "Stretch Horizontal", "Stretch Full"
        };
        int currentAnchorPreset = GetAnchorPresetIndex(rect.anchorMin, rect.anchorMax, rect.pivot);

        if (ImGui::BeginCombo("Anchor Preset", currentAnchorPreset >= 0 ? anchorPresets[currentAnchorPreset] : "Custom")) {
            for (int i = 0; i < IM_ARRAYSIZE(anchorPresets); ++i) {
                bool selected = (i == currentAnchorPreset);
                if (ImGui::Selectable(anchorPresets[i], selected)) {
                    currentAnchorPreset = i;
                    changed = true;
                    switch (i) {
                        case 0:  rect.anchorMin = {0.0f, 1.0f}; rect.anchorMax = {0.0f, 1.0f}; rect.pivot = {0.0f, 1.0f}; break;
                        case 1:  rect.anchorMin = {0.5f, 1.0f}; rect.anchorMax = {0.5f, 1.0f}; rect.pivot = {0.5f, 1.0f}; break;
                        case 2:  rect.anchorMin = {1.0f, 1.0f}; rect.anchorMax = {1.0f, 1.0f}; rect.pivot = {1.0f, 1.0f}; break;
                        case 3:  rect.anchorMin = {0.0f, 0.5f}; rect.anchorMax = {0.0f, 0.5f}; rect.pivot = {0.0f, 0.5f}; break;
                        case 4:  rect.anchorMin = {0.5f, 0.5f}; rect.anchorMax = {0.5f, 0.5f}; rect.pivot = {0.5f, 0.5f}; break;
                        case 5:  rect.anchorMin = {1.0f, 0.5f}; rect.anchorMax = {1.0f, 0.5f}; rect.pivot = {1.0f, 0.5f}; break;
                        case 6:  rect.anchorMin = {0.0f, 0.0f}; rect.anchorMax = {0.0f, 0.0f}; rect.pivot = {0.0f, 0.0f}; break;
                        case 7:  rect.anchorMin = {0.5f, 0.0f}; rect.anchorMax = {0.5f, 0.0f}; rect.pivot = {0.5f, 0.0f}; break;
                        case 8:  rect.anchorMin = {1.0f, 0.0f}; rect.anchorMax = {1.0f, 0.0f}; rect.pivot = {1.0f, 0.0f}; break;
                        case 9:  rect.anchorMin = {0.0f, 1.0f}; rect.anchorMax = {1.0f, 1.0f}; rect.pivot = {0.5f, 1.0f}; break;
                        case 10: rect.anchorMin = {0.0f, 0.5f}; rect.anchorMax = {1.0f, 0.5f}; rect.pivot = {0.5f, 0.5f}; break;
                        case 11: rect.anchorMin = {0.0f, 0.0f}; rect.anchorMax = {1.0f, 0.0f}; rect.pivot = {0.5f, 0.0f}; break;
                        case 12: rect.anchorMin = {0.5f, 0.0f}; rect.anchorMax = {0.5f, 1.0f}; rect.pivot = {0.5f, 0.5f}; break;
                        case 13: rect.anchorMin = {0.0f, 0.5f}; rect.anchorMax = {1.0f, 0.5f}; rect.pivot = {0.5f, 0.5f}; break;
                        case 14: rect.anchorMin = {0.0f, 0.0f}; rect.anchorMax = {1.0f, 1.0f}; rect.pivot = {0.5f, 0.5f}; break;
                    }
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Text("Anchors");
        changed |= ImGui::DragFloat2("Anchor Min", &rect.anchorMin.x, 0.01f, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::DragFloat2("Anchor Max", &rect.anchorMax.x, 0.01f, 0.0f, 1.0f, "%.2f");

        bool isFixed = rect.anchorMin == rect.anchorMax;

        ImGui::Separator();
        ImGui::Text("Pivot");
        Vector2 oldPivot = rect.pivot;
        changed |= ImGui::DragFloat2("Pivot", &rect.pivot.x, 0.01f, 0.0f, 1.0f, "%.2f");

        if (changed && oldPivot != rect.pivot && isFixed) {
            // Corrigir offsets para manter a mesma posição visual
            Vector2 size = rect.offsetMax - rect.offsetMin;
            Vector2 pos = rect.offsetMin + size * oldPivot;
            rect.offsetMin = pos - size * rect.pivot;
            rect.offsetMax = rect.offsetMin + size;
        }

        ImGui::Separator();
        if (isFixed) {
            Vector2 size = rect.offsetMax - rect.offsetMin;
            Vector2 pos = rect.offsetMin + size * rect.pivot;
            ImGui::Text("Unity-like Mode");
            changed |= ImGui::DragFloat2("Position", &pos.x, 1.0f, -10000.0f, 10000.0f, "%.1f");
            changed |= ImGui::DragFloat2("Size", &size.x, 1.0f, 0.0f, 10000.0f, "%.1f");
            if (changed) {
                rect.offsetMin = pos - size * rect.pivot;
                rect.offsetMax = rect.offsetMin + size;
            }
        } else {
            ImGui::Text("Offset Mode (Stretch)");
            changed |= ImGui::DragFloat2("Offset Min", &rect.offsetMin.x, 1.0f, -10000.0f, 10000.0f, "%.1f");
            changed |= ImGui::DragFloat2("Offset Max", &rect.offsetMax.x, 1.0f, -10000.0f, 10000.0f, "%.1f");
        }

        ImGui::TreePop();
    }
}*/

void CanvasComponent::CalculateCanvasScale(float width, float height){
    if (scaleMode == ScaleMode::ScaleWithScreenSize) {
        float scaleX = width / referenceResolution.x;
        float scaleY = height / referenceResolution.y;

        switch (screenMatchMode) {
            case ScreenMatchMode::MatchWidthOrHeight: {
                float logX = std::log2(scaleX);
                float logY = std::log2(scaleY);
                float logInterp = logX * (1.0f - matchValue) + logY * matchValue;
                scaleFactor = std::pow(2.0f, logInterp);
                break;
            }
            case ScreenMatchMode::MatchWidth:
                scaleFactor = scaleX;
                break;
            case ScreenMatchMode::MatchHeight:
                scaleFactor = scaleY;
                break;
            case ScreenMatchMode::Expand:
                scaleFactor = std::max(scaleX, scaleY);
                break;
        }
    } else {
        scaleFactor = defaultScale;  //1.0f;
    }
}

int GetAnchorPresetIndex(const Vector2& min, const Vector2& max) {
    struct Preset { Vector2 min, max; };
    static Preset presets[] = {
        {{0.0f, 1.0f}, {0.0f, 1.0f}}, // Top Left
        {{0.5f, 1.0f}, {0.5f, 1.0f}}, // Top Center
        {{1.0f, 1.0f}, {1.0f, 1.0f}}, // Top Right
        {{0.0f, 0.5f}, {0.0f, 0.5f}}, // Middle Left
        {{0.5f, 0.5f}, {0.5f, 0.5f}}, // Middle Center
        {{1.0f, 0.5f}, {1.0f, 0.5f}}, // Middle Right
        {{0.0f, 0.0f}, {0.0f, 0.0f}}, // Bottom Left
        {{0.5f, 0.0f}, {0.5f, 0.0f}}, // Bottom Center
        {{1.0f, 0.0f}, {1.0f, 0.0f}}, // Bottom Right
        {{0.0f, 1.0f}, {1.0f, 1.0f}}, // Stretch Top
        {{0.0f, 0.5f}, {1.0f, 0.5f}}, // Stretch Middle
        {{0.0f, 0.0f}, {1.0f, 0.0f}}, // Stretch Bottom
        {{0.5f, 0.0f}, {0.5f, 1.0f}}, // Stretch Vertical
        {{0.0f, 0.5f}, {1.0f, 0.5f}}, // Stretch Horizontal
        {{0.0f, 0.0f}, {1.0f, 1.0f}}, // Stretch Full
    };

    for (int i = 0; i < 15; ++i) {
        if (min == presets[i].min && max == presets[i].max)
            return i;
    }
    return -1;
}

int GetPivotPresetIndex(const Vector2& pivot) {
    struct Preset { Vector2 pivot; };
    static Preset presets[] = {
        {{0.5f, 0.5f}}, // Center
        {{0.0f, 1.0f}}, // Top Left
        {{0.5f, 1.0f}}, // Top Center
        {{1.0f, 1.0f}}, // Top Right
        {{0.0f, 0.5f}}, // Middle Left
        {{1.0f, 0.5f}}, // Middle Right
        {{0.0f, 0.0f}}, // Bottom Left
        {{0.5f, 0.0f}}, // Bottom Center
        {{1.0f, 0.0f}}, // Bottom Right
    };

    for (int i = 0; i < 9; ++i) {
        if (pivot == presets[i].pivot)
            return i;
    }
    return -1;
}

void RectTransformComponent::OnGui(Entity& e, Scene& scene) {
    RectTransformComponent& rect = scene.GetComponent<RectTransformComponent>(e);
    bool changed = false;

    if (ImGui::TreeNode("Rect Transform")) {
        // Anchor Preset Dropdown
        static const char* anchorPresets[] = {
            "Top Left", "Top Center", "Top Right",
            "Middle Left", "Middle Center", "Middle Right",
            "Bottom Left", "Bottom Center", "Bottom Right",
            "Stretch Top", "Stretch Middle", "Stretch Bottom",
            "Stretch Vertical", "Stretch Horizontal", "Stretch Full"
        };
        int currentAnchorPreset = GetAnchorPresetIndex(rect.anchorMin, rect.anchorMax);

        if (ImGui::BeginCombo("Anchor Preset", currentAnchorPreset >= 0 ? anchorPresets[currentAnchorPreset] : "Custom")) {
            for (int i = 0; i < IM_ARRAYSIZE(anchorPresets); ++i) {
                bool selected = (i == currentAnchorPreset);
                if (ImGui::Selectable(anchorPresets[i], selected)) {
                    currentAnchorPreset = i;
                    changed = true;
                    Vector2 originalPivot = rect.pivot;
                    Vector2 size = rect.offsetMax - rect.offsetMin;
                    Vector2 pos = rect.offsetMin + size * originalPivot; // Current position
                    switch (i) {
                        case 0:  rect.anchorMin = {0.0f, 1.0f}; rect.anchorMax = {0.0f, 1.0f}; break;
                        case 1:  rect.anchorMin = {0.5f, 1.0f}; rect.anchorMax = {0.5f, 1.0f}; break;
                        case 2:  rect.anchorMin = {1.0f, 1.0f}; rect.anchorMax = {1.0f, 1.0f}; break;
                        case 3:  rect.anchorMin = {0.0f, 0.5f}; rect.anchorMax = {0.0f, 0.5f}; break;
                        case 4:  rect.anchorMin = {0.5f, 0.5f}; rect.anchorMax = {0.5f, 0.5f}; break;
                        case 5:  rect.anchorMin = {1.0f, 0.5f}; rect.anchorMax = {1.0f, 0.5f}; break;
                        case 6:  rect.anchorMin = {0.0f, 0.0f}; rect.anchorMax = {0.0f, 0.0f}; break;
                        case 7:  rect.anchorMin = {0.5f, 0.0f}; rect.anchorMax = {0.5f, 0.0f}; break;
                        case 8:  rect.anchorMin = {1.0f, 0.0f}; rect.anchorMax = {1.0f, 0.0f}; break;
                        case 9:  rect.anchorMin = {0.0f, 1.0f}; rect.anchorMax = {1.0f, 1.0f}; break;
                        case 10: rect.anchorMin = {0.0f, 0.5f}; rect.anchorMax = {1.0f, 0.5f}; break;
                        case 11: rect.anchorMin = {0.0f, 0.0f}; rect.anchorMax = {1.0f, 0.0f}; break;
                        case 12: rect.anchorMin = {0.5f, 0.0f}; rect.anchorMax = {0.5f, 1.0f}; break;
                        case 13: rect.anchorMin = {0.0f, 0.5f}; rect.anchorMax = {1.0f, 0.5f}; break;
                        case 14: rect.anchorMin = {0.0f, 0.0f}; rect.anchorMax = {1.0f, 1.0f}; break;
                    }
                    // Adjust offsets to maintain position for non-stretch presets
                    if (i < 9) {
                        rect.offsetMin = pos - size * rect.pivot;
                        rect.offsetMax = rect.offsetMin + size;
                    }
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // Pivot Preset Dropdown
        static const char* pivotPresets[] = {
            "Center", "Top Left", "Top Center", "Top Right",
            "Middle Left", "Middle Right", "Bottom Left", "Bottom Center", "Bottom Right"
        };
        int currentPivotPreset = GetPivotPresetIndex(rect.pivot);

        if (ImGui::BeginCombo("Pivot Preset", currentPivotPreset >= 0 ? pivotPresets[currentPivotPreset] : "Custom")) {
            for (int i = 0; i < IM_ARRAYSIZE(pivotPresets); ++i) {
                bool selected = (i == currentPivotPreset);
                if (ImGui::Selectable(pivotPresets[i], selected)) {
                    currentPivotPreset = i;
                    changed = true;
                    Vector2 size = rect.offsetMax - rect.offsetMin;
                    Vector2 pos = rect.offsetMin + size * rect.pivot; // Current position
                    switch (i) {
                        case 0: rect.pivot = {0.5f, 0.5f}; break;
                        case 1: rect.pivot = {0.0f, 1.0f}; break;
                        case 2: rect.pivot = {0.5f, 1.0f}; break;
                        case 3: rect.pivot = {1.0f, 1.0f}; break;
                        case 4: rect.pivot = {0.0f, 0.5f}; break;
                        case 5: rect.pivot = {1.0f, 0.5f}; break;
                        case 6: rect.pivot = {0.0f, 0.0f}; break;
                        case 7: rect.pivot = {0.5f, 0.0f}; break;
                        case 8: rect.pivot = {1.0f, 0.0f}; break;
                    }
                    // Adjust offsets to maintain position
                    rect.offsetMin = pos - size * rect.pivot;
                    rect.offsetMax = rect.offsetMin + size;
                }
                if (selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Text("Anchors");
        changed |= ImGui::DragFloat2("Anchor Min", &rect.anchorMin.x, 0.01f, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::DragFloat2("Anchor Max", &rect.anchorMax.x, 0.01f, 0.0f, 1.0f, "%.2f");

        bool isFixed = rect.anchorMin == rect.anchorMax;

        ImGui::Separator();
        ImGui::Text("Pivot");
        Vector2 oldPivot = rect.pivot;
        changed |= ImGui::DragFloat2("Pivot", &rect.pivot.x, 0.01f, 0.0f, 1.0f, "%.2f");

        if (changed && oldPivot != rect.pivot && isFixed) {
            Vector2 size = rect.offsetMax - rect.offsetMin;
            Vector2 pos = rect.offsetMin + size * rect.pivot;//oldPivot;
            rect.offsetMin = pos - size * rect.pivot;
            rect.offsetMax = rect.offsetMin + size;
        }

        ImGui::Separator();
        if (isFixed) {
            Vector2 size = rect.offsetMax - rect.offsetMin;
            Vector2 pos = rect.offsetMin + size * rect.pivot;
            ImGui::Text("Unity-like Mode");
            changed |= ImGui::DragFloat2("Position", &pos.x, 1.0f, -10000.0f, 10000.0f, "%.1f");
            changed |= ImGui::DragFloat2("Size", &size.x, 1.0f, 0.0f, 10000.0f, "%.1f");
            if (changed) {
                rect.offsetMin = pos - size * rect.pivot;
                rect.offsetMax = rect.offsetMin + size;
            }
        } else {
            ImGui::Text("Offset Mode (Stretch)");
            changed |= ImGui::DragFloat2("Offset Min", &rect.offsetMin.x, 1.0f, -10000.0f, 10000.0f, "%.1f");
            changed |= ImGui::DragFloat2("Offset Max", &rect.offsetMax.x, 1.0f, -10000.0f, 10000.0f, "%.1f");
        }

        ImGui::TreePop();
    }
}

void CalculateUIElementRecursive(Scene& scene, Entity entity, Vector2 parentPos, Vector2 parentSize, float scaleFactor) {
    /*if (!scene.HasComponent<RectTransformComponent>(entity)) return;

    auto& rect = scene.GetComponent<RectTransformComponent>(entity);

    // Calculate anchor positions relative to parent
    Vector2 anchorPosMin = parentPos + parentSize * rect.anchorMin;
    Vector2 anchorPosMax = parentPos + parentSize * rect.anchorMax;

    // Apply scaled offsets
    Vector2 offsetMinScaled = rect.offsetMin * scaleFactor;
    Vector2 offsetMaxScaled = rect.offsetMax * scaleFactor;

    // Calculate final rect bounds
    Vector2 min = anchorPosMin + offsetMinScaled;
    Vector2 max = anchorPosMax + offsetMaxScaled;
    Vector2 size = max - min;
    Vector2 finalPos = min + size * rect.pivot;

    rect.finalSize = size;
    rect.finalPosition = finalPos;

    // Recurse for children
    auto& transform = scene.GetComponent<TransformComponent>(entity);
    for (Entity child : transform.Children()) {
        CalculateUIElementRecursive(scene, child, min, size, scaleFactor);
    }*/

    if (!scene.HasComponent<RectTransformComponent>(entity)) return;

    auto& rect = scene.GetComponent<RectTransformComponent>(entity);

    // 1. Calculate anchored area inside parent
    Vector2 anchorPosMin = parentPos + parentSize * rect.anchorMin;
    Vector2 anchorPosMax = parentPos + parentSize * rect.anchorMax;

    // 2. Apply scaled offsets
    Vector2 offsetMinScaled = rect.offsetMin * scaleFactor;
    Vector2 offsetMaxScaled = rect.offsetMax * scaleFactor;

    // 3. Compute UI rect in parent space
    Vector2 rectMin = anchorPosMin + offsetMinScaled;
    Vector2 rectMax = anchorPosMax + offsetMaxScaled;
    Vector2 size = rectMax - rectMin;

    // 4. Final position of the element (applying pivot)
    Vector2 finalPos = rectMin + size * rect.pivot;

    // 5. Store for rendering
    rect.finalSize = size;
    rect.finalPosition = finalPos;

    // ✅ FIX: Propagate real rect to children (not anchor area!)
    Vector2 childParentPos = rectMin;
    Vector2 childParentSize = size;

    // Recurse to children
    auto& transform = scene.GetComponent<TransformComponent>(entity);
    for (Entity child : transform.Children()) {
        CalculateUIElementRecursive(scene, child, childParentPos, childParentSize, scaleFactor);
    }
}

void RecalculateUI(Scene& scene, Camera& camera) {
    auto view = scene.GetRegistry().view<CanvasComponent, TransformComponent>();

    for (auto canvasEntity : view) {
        auto& canvas = view.get<CanvasComponent>(canvasEntity);
        canvas.CalculateCanvasScale(camera.width, camera.height);

        // Use screen size as the root size
        Vector2 rootSize = {camera.width, camera.height};
        Vector2 rootPos = {0, 0}; // Bottom-left origin

        // Handle canvas RectTransform (should span full screen)
        if (scene.HasComponent<RectTransformComponent>(canvasEntity)) {
            auto& rect = scene.GetComponent<RectTransformComponent>(canvasEntity);

            Vector2 anchorPosMin = rootPos + rootSize * rect.anchorMin;
            Vector2 anchorPosMax = rootPos + rootSize * rect.anchorMax;
            Vector2 offsetMinScaled = rect.offsetMin * canvas.scaleFactor;
            Vector2 offsetMaxScaled = rect.offsetMax * canvas.scaleFactor;
            Vector2 min = anchorPosMin + offsetMinScaled;
            Vector2 max = anchorPosMax + offsetMaxScaled;

            rect.finalSize = max - min;
            rect.finalPosition = min + rect.finalSize * rect.pivot;

            rootPos = min;
            rootSize = rect.finalSize;
        } else {
            // If no RectTransform, use full screen size
            rootSize = {camera.width, camera.height};
            rootPos = {0, 0};
        }

        // Recurse for children
        auto& transform = scene.GetComponent<TransformComponent>(canvasEntity);
        for (Entity child : transform.Children()) {
            CalculateUIElementRecursive(scene, child, rootPos, rootSize, canvas.scaleFactor);
        }
    }
}

}