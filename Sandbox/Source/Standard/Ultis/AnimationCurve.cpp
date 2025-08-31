#include "AnimationCurve.h"
#include <algorithm>
#include <cmath>

namespace Standard{

// Helper functions for ImVec2 operations
ImVec2 operator+(const ImVec2& a, const ImVec2& b) {
    return ImVec2(a.x + b.x, a.y + b.y);
}

ImVec2 operator-(const ImVec2& a, const ImVec2& b) {
    return ImVec2(a.x - b.x, a.y - b.y);
}

ImVec2 operator*(const ImVec2& a, float scalar) {
    return ImVec2(a.x * scalar, a.y * scalar);
}


// Add a keyframe
void AnimationCurve::AddKeyframe(float time, float value, CurveType type) {
    keyframes.emplace_back(time, value, type);
    SortKeyframes();
}

/*
// Evaluate the curve at a given time
float AnimationCurve::Evaluate(float time) const {
    if (keyframes.empty()) return 0.0f;
    if (keyframes.size() == 1) return keyframes[0].value;

    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        const Keyframe& k0 = keyframes[i];
        const Keyframe& k1 = keyframes[i + 1];
        if (time >= k0.time && time <= k1.time) {
            float t = (time - k0.time) / (k1.time - k0.time);
            switch (k0.curve_type) {
                case CurveType::Linear:
                    return k0.value + t * (k1.value - k0.value);
                case CurveType::Constant:
                    return k0.value;
                case CurveType::Smooth: {
                    // Cubic Bezier interpolation
                    ImVec2 p0(k0.time, k0.value);
                    ImVec2 p3(k1.time, k1.value);
                    ImVec2 p1 = p0 + k0.out_tangent;
                    ImVec2 p2 = p3 + k1.in_tangent;
                    float u = t;
                    float u2 = u * u;
                    float u3 = u2 * u;
                    float w0 = 1 - 3*u + 3*u2 - u3; // (1-u)^3
                    float w1 = 3*u - 6*u2 + 3*u3;   // 3u(1-u)^2
                    float w2 = 3*u2 - 3*u3;         // 3u^2(1-u)
                    float w3 = u3;                  // u^3
                    return w0 * p0.y + w1 * p1.y + w2 * p2.y + w3 * p3.y;
                }
            }
        }
    }

    // Return nearest keyframe value if outside range
    if (time < keyframes[0].time) return keyframes[0].value;
    return keyframes.back().value;
}
*/

// Evaluate the curve at a given time
float AnimationCurve::Evaluate(float time) const {
    if (keyframes.empty()) return 0.0f;
    if (keyframes.size() == 1) return keyframes[0].value;

    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        const Keyframe& k0 = keyframes[i];
        const Keyframe& k1 = keyframes[i + 1];
        if (time >= k0.time && time <= k1.time) {
            float t = (time - k0.time) / (k1.time - k0.time);
            switch (k0.curve_type) {
                case CurveType::Linear:
                    return k0.value + t * (k1.value - k0.value);
                case CurveType::Constant:
                    return k0.value;
                case CurveType::Smooth: {
                    // Cubic Bezier interpolation
                    ImVec2 p0(k0.time, k0.value);
                    ImVec2 p3(k1.time, k1.value);
                    // Scale tangents to curve's time-value space and invert Y
                    float value_range = 2.0f; // max_value - min_value (1.0 - (-1.0))
                    float tangent_scale = 0.01f; // Reduce sensitivity (adjust as needed)
                    ImVec2 p1 = p0 + ImVec2(k0.out_tangent.x * tangent_scale, -k0.out_tangent.y * tangent_scale * value_range);
                    ImVec2 p2 = p3 + ImVec2(k1.in_tangent.x * tangent_scale, -k1.in_tangent.y * tangent_scale * value_range);
                    float u = t;
                    float u2 = u * u;
                    float u3 = u2 * u;
                    float w0 = 1 - 3*u + 3*u2 - u3; // (1-u)^3
                    float w1 = 3*u - 6*u2 + 3*u3;   // 3u(1-u)^2
                    float w2 = 3*u2 - 3*u3;         // 3u^2(1-u)
                    float w3 = u3;                  // u^3
                    return w0 * p0.y + w1 * p1.y + w2 * p2.y + w3 * p3.y;
                }
            }
        }
    }

    // Return nearest keyframe value if outside range
    if (time < keyframes[0].time) return keyframes[0].value;
    return keyframes.back().value;
}

// Sort keyframes by time
void AnimationCurve::SortKeyframes() {
    std::sort(keyframes.begin(), keyframes.end(),
        [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
}

// Function to draw a small curve preview (only the curve, no dots)
void DrawCurvePreview(AnimationCurve& curve, ImVec2 size, bool* open_editor) {
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = size;

    // Ensure canvas has a minimum size
    canvas_size.x = std::max(canvas_size.x, 50.0f);
    canvas_size.y = std::max(canvas_size.y, 30.0f);

    // Draw background
    draw_list->AddRectFilled(canvas_pos, canvas_pos + canvas_size, IM_COL32(50, 50, 50, 255));
    draw_list->AddRect(canvas_pos, canvas_pos + canvas_size, IM_COL32(100, 100, 100, 255));

    // Time and value ranges
    float min_time = 0.0f, max_time = 1.0f;
    float min_value = -1.0f, max_value = 1.0f;

    // Draw curve
    const int curve_segments = 50;
    for (int i = 0; i < curve_segments; ++i) {
        float t0 = min_time + (float)i / curve_segments * (max_time - min_time);
        float t1 = min_time + (float)(i + 1) / curve_segments * (max_time - min_time);
        float v0 = curve.Evaluate(t0);
        float v1 = curve.Evaluate(t1);

        ImVec2 p0(canvas_pos.x + (t0 - min_time) / (max_time - min_time) * canvas_size.x,
                  canvas_pos.y + (max_value - v0) / (max_value - min_value) * canvas_size.y);
        ImVec2 p1(canvas_pos.x + (t1 - min_time) / (max_time - min_time) * canvas_size.x,
                  canvas_pos.y + (max_value - v1) / (max_value - min_value) * canvas_size.y);
        draw_list->AddLine(p0, p1, IM_COL32(255, 255, 0, 255), 1.0f);
    }

    // Open editor on click
    bool is_hovered = ImGui::IsMouseHoveringRect(canvas_pos, canvas_pos + canvas_size);
    if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && open_editor) {
        *open_editor = true;
    }

    ImGui::InvisibleButton("curve_preview", canvas_size);
}

// Function to draw the curve editor
void DrawAnimationCurveEditor(AnimationCurve& curve, ImVec2 size) {
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = size;

    // Ensure canvas has a minimum size
    canvas_size.x = std::max(canvas_size.x, 50.0f);
    canvas_size.y = std::max(canvas_size.y, 50.0f);

    // Draw background
    draw_list->AddRectFilled(canvas_pos, canvas_pos + canvas_size, IM_COL32(50, 50, 50, 255));
    draw_list->AddRect(canvas_pos, canvas_pos + canvas_size, IM_COL32(100, 100, 100, 255));

    // Time and value ranges
    float min_time = 0.0f, max_time = 1.0f;
    float min_value = -1.0f, max_value = 1.0f;

    // Draw grid
    for (float t = min_time; t <= max_time; t += 0.1f) {
        float x = canvas_pos.x + (t - min_time) / (max_time - min_time) * canvas_size.x;
        draw_list->AddLine(ImVec2(x, canvas_pos.y), ImVec2(x, canvas_pos.y + canvas_size.y), IM_COL32(100, 100, 100, 50));
    }
    for (float v = min_value; v <= max_value; v += 0.1f) {
        float y = canvas_pos.y + (max_value - v) / (max_value - min_value) * canvas_size.y;
        draw_list->AddLine(ImVec2(canvas_pos.x, y), ImVec2(canvas_pos.x + canvas_size.x, y), IM_COL32(100, 100, 100, 50));
    }

    // Draw curve
    const int curve_segments = 100;
    for (int i = 0; i < curve_segments; ++i) {
        float t0 = min_time + (float)i / curve_segments * (max_time - min_time);
        float t1 = min_time + (float)(i + 1) / curve_segments * (max_time - min_time);
        float v0 = curve.Evaluate(t0);
        float v1 = curve.Evaluate(t1);

        ImVec2 p0(canvas_pos.x + (t0 - min_time) / (max_time - min_time) * canvas_size.x,
                  canvas_pos.y + (max_value - v0) / (max_value - min_value) * canvas_size.y);
        ImVec2 p1(canvas_pos.x + (t1 - min_time) / (max_time - min_time) * canvas_size.x,
                  canvas_pos.y + (max_value - v1) / (max_value - min_value) * canvas_size.y);
        draw_list->AddLine(p0, p1, IM_COL32(255, 255, 0, 255), 2.0f);
    }

    // Handle keyframe and tangent interaction
    static int selected_keyframe = -1;
    static bool editing_in_tangent = false;
    static bool editing_out_tangent = false;

    // Draw all keyframes and tangent handles
    for (size_t i = 0; i < curve.keyframes.size(); ++i) {
        const Keyframe& kf = curve.keyframes[i];
        ImVec2 kf_pos(canvas_pos.x + (kf.time - min_time) / (max_time - min_time) * canvas_size.x,
                      canvas_pos.y + (max_value - kf.value) / (max_value - min_value) * canvas_size.y);

        // Draw tangent handles (only for Smooth curve type)
        if (kf.curve_type == CurveType::Smooth || (i < curve.keyframes.size() - 1 && curve.keyframes[i + 1].curve_type == CurveType::Smooth)) {
            ImVec2 in_tangent_pos = kf_pos + kf.in_tangent;
            ImVec2 out_tangent_pos = kf_pos + kf.out_tangent;
            draw_list->AddLine(kf_pos, in_tangent_pos, IM_COL32(0, 255, 255, 255));
            draw_list->AddLine(kf_pos, out_tangent_pos, IM_COL32(0, 255, 255, 255));
            draw_list->AddCircleFilled(in_tangent_pos, 3.0f, IM_COL32(0, 255, 255, 255));
            draw_list->AddCircleFilled(out_tangent_pos, 3.0f, IM_COL32(0, 255, 255, 255));
        }

        // Draw keyframe (highlight if selected)
        uint32_t keyframe_color = (selected_keyframe == static_cast<int>(i)) ? IM_COL32(255, 100, 100, 255) : IM_COL32(255, 0, 0, 255);
        draw_list->AddCircleFilled(kf_pos, 5.0f, keyframe_color);
    }

    // Handle mouse interactions for selection and dragging
    bool is_hovered = ImGui::IsMouseHoveringRect(canvas_pos, canvas_pos + canvas_size);
    if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        ImVec2 mouse_pos = io.MousePos;
        bool hit_keyframe = false;

        // Check for keyframe or tangent selection
        for (size_t i = 0; i < curve.keyframes.size(); ++i) {
            const Keyframe& kf = curve.keyframes[i];
            ImVec2 kf_pos(canvas_pos.x + (kf.time - min_time) / (max_time - min_time) * canvas_size.x,
                          canvas_pos.y + (max_value - kf.value) / (max_value - min_value) * canvas_size.y);

            // Check if keyframe is clicked
            if (std::hypot(mouse_pos.x - kf_pos.x, mouse_pos.y - kf_pos.y) < 5.0f) {
                selected_keyframe = static_cast<int>(i);
                editing_in_tangent = false;
                editing_out_tangent = false;
                hit_keyframe = true;
                break;
            }

            // Check if tangent handles are clicked (only for Smooth)
            if (kf.curve_type == CurveType::Smooth) {
                ImVec2 in_tangent_pos = kf_pos + kf.in_tangent;
                ImVec2 out_tangent_pos = kf_pos + kf.out_tangent;
                if (std::hypot(mouse_pos.x - in_tangent_pos.x, mouse_pos.y - in_tangent_pos.y) < 5.0f) {
                    selected_keyframe = static_cast<int>(i);
                    editing_in_tangent = true;
                    editing_out_tangent = false;
                    hit_keyframe = true;
                    break;
                }
                if (std::hypot(mouse_pos.x - out_tangent_pos.x, mouse_pos.y - out_tangent_pos.y) < 5.0f) {
                    selected_keyframe = static_cast<int>(i);
                    editing_in_tangent = false;
                    editing_out_tangent = true;
                    hit_keyframe = true;
                    break;
                }
            }
        }

        // Deselect if clicked on canvas background
        if (!hit_keyframe) {
            selected_keyframe = -1;
            editing_in_tangent = false;
            editing_out_tangent = false;
        }
    }

    // Handle right-click to add a keyframe
    if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        ImVec2 mouse_pos = io.MousePos;
        float time = min_time + (mouse_pos.x - canvas_pos.x) / canvas_size.x * (max_time - min_time);
        float value = max_value - (mouse_pos.y - canvas_pos.y) / canvas_size.y * (max_value - min_value);
        curve.AddKeyframe(time, value, CurveType::Linear);
    }

    // Move selected keyframe or tangent
    if (selected_keyframe >= 0 && selected_keyframe < (int)curve.keyframes.size() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 mouse_pos = io.MousePos;
        float time = min_time + (mouse_pos.x - canvas_pos.x) / canvas_size.x * (max_time - min_time);
        float value = max_value - (mouse_pos.y - canvas_pos.y) / canvas_size.y * (max_value - min_value);
        time = std::clamp(time, min_time, max_time);
        value = std::clamp(value, min_value, max_value);

        Keyframe& kf = curve.keyframes[selected_keyframe];
        if (editing_in_tangent) {
            ImVec2 kf_pos(canvas_pos.x + (kf.time - min_time) / (max_time - min_time) * canvas_size.x,
                          canvas_pos.y + (max_value - kf.value) / (max_value - min_value) * canvas_size.y);
            kf.in_tangent = mouse_pos - kf_pos;
            kf.in_tangent.x = std::clamp(kf.in_tangent.x, -50.0f, -5.0f); // Prevent extreme x-values
        } else if (editing_out_tangent) {
            ImVec2 kf_pos(canvas_pos.x + (kf.time - min_time) / (max_time - min_time) * canvas_size.x,
                          canvas_pos.y + (max_value - kf.value) / (max_value - min_value) * canvas_size.y);
            kf.out_tangent = mouse_pos - kf_pos;
            kf.out_tangent.x = std::clamp(kf.out_tangent.x, 5.0f, 50.0f); // Prevent extreme x-values
        } else {
            kf.time = time;
            kf.value = value;
            curve.SortKeyframes();
        }
    }

    // Delete keyframe with middle click
    if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
        ImVec2 mouse_pos = io.MousePos;
        for (size_t i = 0; i < curve.keyframes.size(); ++i) {
            const Keyframe& kf = curve.keyframes[i];
            ImVec2 kf_pos(canvas_pos.x + (kf.time - min_time) / (max_time - min_time) * canvas_size.x,
                          canvas_pos.y + (max_value - kf.value) / (max_value - min_value) * canvas_size.y);
            if (std::hypot(mouse_pos.x - kf_pos.x, mouse_pos.y - kf_pos.y) < 5.0f) {
                curve.keyframes.erase(curve.keyframes.begin() + i);
                if (selected_keyframe == static_cast<int>(i)) {
                    selected_keyframe = -1; // Deselect if the deleted keyframe was selected
                } else if (selected_keyframe > static_cast<int>(i)) {
                    selected_keyframe--; // Adjust index if a keyframe before the selected one was deleted
                }
                break;
            }
        }
    }

    // Create an invisible button to capture input
    ImGui::InvisibleButton("curve_canvas", canvas_size);

    // UI for selected keyframe (always show if a keyframe is selected)
    if (selected_keyframe >= 0 && selected_keyframe < (int)curve.keyframes.size()) {
        Keyframe& kf = curve.keyframes[selected_keyframe];
        ImGui::Text("Selected Keyframe:");
        ImGui::InputFloat("Time", &kf.time, 0.01f, 0.1f);
        ImGui::InputFloat("Value", &kf.value, 0.01f, 0.1f);
        kf.time = std::clamp(kf.time, min_time, max_time);
        kf.value = std::clamp(kf.value, min_value, max_value);
        curve.SortKeyframes();

        // Curve type selection
        const char* curve_types[] = { "Linear", "Constant", "Smooth" };
        int current_type = (int)kf.curve_type;
        if (ImGui::Combo("Curve Type", &current_type, curve_types, IM_ARRAYSIZE(curve_types))) {
            kf.curve_type = (CurveType)current_type;
        }

        // Tangent controls (only for Smooth)
        if (kf.curve_type == CurveType::Smooth) {
            ImGui::InputFloat2("In Tangent", &kf.in_tangent.x, "%.2f");
            ImGui::InputFloat2("Out Tangent", &kf.out_tangent.x, "%.2f");
            kf.in_tangent.x = std::clamp(kf.in_tangent.x, -50.0f, -5.0f);
            kf.out_tangent.x = std::clamp(kf.out_tangent.x, 5.0f, 50.0f);
        }

        // Delete keyframe button
        if (ImGui::Button("Delete Keyframe")) {
            curve.keyframes.erase(curve.keyframes.begin() + selected_keyframe);
            selected_keyframe = -1; // Deselect after deletion
        }
    }

    // Debug: Display number of keyframes
    ImGui::Text("Keyframes: %zu", curve.keyframes.size());
}

}