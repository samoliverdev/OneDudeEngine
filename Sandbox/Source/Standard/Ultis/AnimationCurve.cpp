#include "AnimationCurve.h"
#include <algorithm>
#include <cmath>

namespace Standard{

// Helper functions for ImVec2 operations
ImVec2 operator+(const ImVec2& a, const ImVec2& b){
    return ImVec2(a.x + b.x, a.y + b.y);
}

ImVec2 operator+(const ImVec2& a, float b){
    return ImVec2(a.x + b, a.y + b);
}

ImVec2 operator-(const ImVec2& a, const ImVec2& b){
    return ImVec2(a.x - b.x, a.y - b.y);
}

ImVec2 operator-(const ImVec2& a, float b){
    return ImVec2(a.x - b, a.y - b);
}

ImVec2 operator*(const ImVec2& a, float scalar){
    return ImVec2(a.x * scalar, a.y * scalar);
}

ImVec2 operator/(const ImVec2& a, float scalar){
    return ImVec2(a.x / scalar, a.y / scalar);
}

// Add a keyframe
void AnimationCurve::AddKeyframe(float time, float value, CurveType type){
    keyframes.emplace_back(time, value, type);
    SortKeyframes();
}

// Evaluate the curve at a given time
float AnimationCurve::Evaluate(float time) const {
    if(keyframes.empty()) return 0.0f;
    if(keyframes.size() == 1) return keyframes[0].value;

    for(size_t i = 0; i < keyframes.size() - 1; ++i){
        const Keyframe& k0 = keyframes[i];
        const Keyframe& k1 = keyframes[i + 1];
        if(time >= k0.time && time <= k1.time){
            float t = (time - k0.time) / (k1.time - k0.time);
            switch(k0.curve_type){
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
                    //ImVec2 p1 = p0 + ImVec2(k0.out_tangent.x * tangent_scale, -k0.out_tangent.y * tangent_scale * value_range);
                    //ImVec2 p2 = p3 + ImVec2(k1.in_tangent.x * tangent_scale, -k1.in_tangent.y * tangent_scale * value_range);

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
                case CurveType::AutoSmooth: {
                    // Catmull-Rom style tangent generation

                    float m0;
                    float m1;

                    // Previous point
                    if(i > 0){
                        const Keyframe& kp = keyframes[i - 1];

                        m0 = (k1.value - kp.value) / (k1.time - kp.time);
                    } else {
                        // first keyframe
                        m0 = (k1.value - k0.value) / (k1.time - k0.time);
                    }

                    // Next point
                    if(i + 2 < keyframes.size()){
                        const Keyframe& kn = keyframes[i + 2];

                        m1 = (kn.value - k0.value) / (kn.time - k0.time);
                    } else {
                        // last keyframe
                        m1 = (k1.value - k0.value) / (k1.time - k0.time);
                    }

                    float dt = k1.time - k0.time;

                    // Cubic Hermite interpolation
                    float t2 = t * t;
                    float t3 = t2 * t;

                    float h00 =  2*t3 - 3*t2 + 1;
                    float h10 =      t3 - 2*t2 + t;
                    float h01 = -2*t3 + 3*t2;
                    float h11 =      t3 -   t2;

                    return
                        h00 * k0.value +
                        h10 * (m0 * dt) +
                        h01 * k1.value +
                        h11 * (m1 * dt);
                }
            }
        }
    }

    // Return nearest keyframe value if outside range
    if(time < keyframes[0].time) return keyframes[0].value;
    return keyframes.back().value;
}

// Sort keyframes by time
void AnimationCurve::SortKeyframes(){
    std::sort(
        keyframes.begin(), keyframes.end(),
        [](const Keyframe& a, const Keyframe& b){ 
            return a.time < b.time; 
        }
    );
}

void AnimationCurve::UpdateMinMax(){
    /*if (keyframes.empty()) return;

    float min_time = keyframes.front().time;
    float max_time = keyframes.front().time;
    float min_value = keyframes.front().value;
    float max_value = keyframes.front().value;

    for (const auto& kf : keyframes) {
        if (kf.time < min_time) min_time = kf.time;
        if (kf.time > max_time) max_time = kf.time;
        if (kf.value < min_value) min_value = kf.value;
        if (kf.value > max_value) max_value = kf.value;
    }

    // Add small padding
    float time_padding = (max_time - min_time) * 0.05f;
    float value_padding = (max_value - min_value) * 0.1f;

    if (fabsf(max_time - min_time) < 1e-6f) max_time = min_time + 1e-3f;
    if (fabsf(max_value - min_value) < 1e-6f) max_value = min_value + 1e-3f;

    minMaxTime.x = min_time - time_padding;
    minMaxTime.y = max_time + time_padding;
    minMaxValue.x = min_value - value_padding;
    minMaxValue.y = max_value + value_padding;*/

    if(keyframes.empty()){
        minMaxTime = {0.0f, 1.0f};
        minMaxValue = {0.0f, 1.0f};
        return;
    }

    float min_time  = keyframes.front().time;
    float max_time  = keyframes.front().time;
    float min_value = keyframes.front().value;
    float max_value = keyframes.front().value;

    for(const auto& kf : keyframes){
        if(kf.time < min_time) min_time  = kf.time;
        if(kf.time > max_time) max_time  = kf.time;
        if(kf.value < min_value) min_value = kf.value;
        if(kf.value > max_value) max_value = kf.value;
    }

    // Add small padding
    float time_padding = 0; //(max_time - min_time) * 0.05f;
    float value_padding = 0; //(max_value - min_value) * 0.1f;

    // Avoid zero range
    if(fabsf(max_time - min_time) < 1e-6f) max_time = min_time + 1e-3f;
    if(fabsf(max_value - min_value) < 1e-6f) max_value = min_value + 1e-3f;

    min_time -= time_padding;
    max_time += time_padding;
    min_value -= value_padding;
    max_value += value_padding;

    // Clamp to minimal default range
    if(min_time > 0.0f) min_time  = 0.0f;
    if(max_time < 1.0f) max_time  = 1.0f;
    if(min_value > 0.0f) min_value = 0.0f;
    if(max_value < 1.0f) max_value = 1.0f;

    minMaxTime = { min_time,  max_time  };
    minMaxValue = { min_value, max_value };
}

void AnimationCurve::OnGui(cereal::ImGuiArchive& ar){
    ImGui::PushID(this);

    //static bool show_curve_editor;
    DrawCurvePreview(*this, ImVec2(100, 50), &show_curve_editor);

    // Curve editor window
    if(show_curve_editor){
        std::string name = "Curve Editor##" + std::to_string((uintptr_t)this);
        ImGui::Begin(name.c_str(), &show_curve_editor, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking);

        //ImGui::Begin("Curve Editor", &show_curve_editor, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking);
        
        DrawAnimationCurveEditor(*this, ImVec2(400, 200));
        ImGui::End();
    }

    ImGui::PopID();
}

// Function to draw a small curve preview (only the curve, no dots)
void DrawCurvePreview(AnimationCurve& curve, ImVec2 size, bool* open_editor){
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
    //float min_time = -0.2f, max_time = 1.2f;
    //float min_value = -1.2f, max_value = 1.2f;
    float min_time = curve.minMaxTime.x, max_time = curve.minMaxTime.y;
    float min_value = curve.minMaxValue.x, max_value = curve.minMaxValue.y;

    // Draw curve
    const int curve_segments = 50;
    for(int i = 0; i < curve_segments; ++i){
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
    if(is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && open_editor){
        *open_editor = true;
    }

    ImGui::PushID(&curve);
    ImGui::InvisibleButton("curve_preview", canvas_size);
    ImGui::PopID();
}

#if 0
void DrawAnimationCurveEditor(AnimationCurve& curve, ImVec2 size){
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = size;
    canvas_size.x = std::max(canvas_size.x, 50.0f);
    canvas_size.y = std::max(canvas_size.y, 50.0f);

    // Reserve the area first
    //ImGui::InvisibleButton("curve_canvas", canvas_size);

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Background
    draw_list->AddRectFilled(canvas_pos, canvas_pos + canvas_size, IM_COL32(50, 50, 50, 255));

    // Selection state
    /*static int selected_keyframe = -1;
    static bool editing_in_tangent = false;
    static bool editing_out_tangent = false;*/

    int& selected_keyframe = curve.gui_selected_keyframe;
    bool& editing_in_tangent = curve.gui_editing_in_tangent;
    bool& editing_out_tangent = curve.gui_editing_out_tangent;

    bool is_dragging = (selected_keyframe >= 0 && ImGui::IsMouseDragging(ImGuiMouseButton_Left));

    // --- Update min/max only if user is not dragging/editing ---
    if(!is_dragging){
        //curve.UpdateMinMax();
    }

    float clamp_min_time  = curve.minMaxTime.x;
    float clamp_max_time  = curve.minMaxTime.y;
    float clamp_min_value = curve.minMaxValue.x;
    float clamp_max_value = curve.minMaxValue.y;

    // Padding for visual only
    float draw_pad_time  = 0.05f;
    float draw_pad_value = 0.1f;

    float draw_min_time  = clamp_min_time - draw_pad_time;
    float draw_max_time  = clamp_max_time + draw_pad_time;
    float draw_min_value = clamp_min_value - draw_pad_value;
    float draw_max_value = clamp_max_value + draw_pad_value;

    // INNER RECT (REAL EDITOR SPACE)
    ImVec2 inner_min(
        canvas_pos.x + (clamp_min_time - draw_min_time) / (draw_max_time - draw_min_time) * canvas_size.x,
        canvas_pos.y + (draw_max_value - clamp_max_value) / (draw_max_value - draw_min_value) * canvas_size.y
    );

    ImVec2 inner_max(
        canvas_pos.x + (clamp_max_time - draw_min_time) / (draw_max_time - draw_min_time) * canvas_size.x,
        canvas_pos.y + (draw_max_value - clamp_min_value) / (draw_max_value - draw_min_value) * canvas_size.y
    );

    ImVec2 inner_size = inner_max - inner_min;

    // Draw clamp rect
    draw_list->AddRect(inner_min, inner_max, IM_COL32(150, 150, 150, 255));

    // Border
    draw_list->AddRect(canvas_pos, canvas_pos + canvas_size, IM_COL32(100, 100, 100, 255));

    // Grid (inside inner rect)
    for(float t = clamp_min_time; t <= clamp_max_time; t += 0.1f){
        float x = inner_min.x + (t - clamp_min_time) / (clamp_max_time - clamp_min_time) * inner_size.x;
        draw_list->AddLine(ImVec2(x, inner_min.y), ImVec2(x, inner_max.y), IM_COL32(100, 100, 100, 50));
    }

    for(float v = clamp_min_value; v <= clamp_max_value; v += 0.1f){
        float y = inner_min.y + (clamp_max_value - v) / (clamp_max_value - clamp_min_value) * inner_size.y;
        draw_list->AddLine(ImVec2(inner_min.x, y), ImVec2(inner_max.x, y), IM_COL32(100, 100, 100, 50));
    }

    // Clip to inner rect
    draw_list->PushClipRect(inner_min, inner_max, true);

    /*// Draw curve
    const int segments = 100;
    for (int i = 0; i < segments; i++)
    {
        float t0 = clamp_min_time + (float)i / segments * (clamp_max_time - clamp_min_time);
        float t1 = clamp_min_time + (float)(i + 1) / segments * (clamp_max_time - clamp_min_time);

        float v0 = curve.Evaluate(t0);
        float v1 = curve.Evaluate(t1);

        ImVec2 p0(
            inner_min.x + (t0 - clamp_min_time) / (clamp_max_time - clamp_min_time) * inner_size.x,
            inner_min.y + (clamp_max_value - v0) / (clamp_max_value - clamp_min_value) * inner_size.y
        );

        ImVec2 p1(
            inner_min.x + (t1 - clamp_min_time) / (clamp_max_time - clamp_min_time) * inner_size.x,
            inner_min.y + (clamp_max_value - v1) / (clamp_max_value - clamp_min_value) * inner_size.y
        );

        draw_list->AddLine(p0, p1, IM_COL32(255, 255, 0, 255), 2.0f);
    }*/

    // Draw curve
    const int curve_segments = 100;
    for(int i = 0; i < curve_segments; ++i){
        float t0 = draw_min_time + (float)i / curve_segments * (draw_max_time - draw_min_time);
        float t1 = draw_min_time + (float)(i + 1) / curve_segments * (draw_max_time - draw_min_time);
        float v0 = curve.Evaluate(t0);
        float v1 = curve.Evaluate(t1);

        ImVec2 p0(canvas_pos.x + (t0 - draw_min_time) / (draw_max_time - draw_min_time) * canvas_size.x,
                  canvas_pos.y + (draw_max_value - v0) / (draw_max_value - draw_min_value) * canvas_size.y);
        ImVec2 p1(canvas_pos.x + (t1 - draw_min_time) / (draw_max_time - draw_min_time) * canvas_size.x,
                  canvas_pos.y + (draw_max_value - v1) / (draw_max_value - draw_min_value) * canvas_size.y);
        draw_list->AddLine(p0, p1, IM_COL32(255, 255, 0, 255), 2.0f);
    }

    draw_list->PopClipRect();

    // Draw keyframes + tangents
    for(size_t i = 0; i < curve.keyframes.size(); i++){
        const Keyframe& kf = curve.keyframes[i];

        ImVec2 kf_pos(
            inner_min.x + (kf.time - clamp_min_time) / (clamp_max_time - clamp_min_time) * inner_size.x,
            inner_min.y + (clamp_max_value - kf.value) / (clamp_max_value - clamp_min_value) * inner_size.y
        );

        if(kf.curve_type == CurveType::Smooth){
        //if (kf.curve_type == CurveType::Smooth || (i < curve.keyframes.size() - 1 && curve.keyframes[i + 1].curve_type == CurveType::Smooth)){
            ImVec2 in_pos  = kf_pos + kf.in_tangent;
            ImVec2 out_pos = kf_pos + kf.out_tangent;

            draw_list->AddLine(kf_pos, in_pos, IM_COL32(0, 255, 255, 255));
            draw_list->AddLine(kf_pos, out_pos, IM_COL32(0, 255, 255, 255));
            draw_list->AddCircleFilled(in_pos, 3.0f, IM_COL32(0, 255, 255, 255));
            draw_list->AddCircleFilled(out_pos, 3.0f, IM_COL32(0, 255, 255, 255));
        }

        uint32_t col = (selected_keyframe == (int)i) ? IM_COL32(255, 100, 100, 255) : IM_COL32(255, 0, 0, 255);
        draw_list->AddCircleFilled(kf_pos, 5.0f, col);
    }

    // Hover only inner rect
    //bool is_hovered = ImGui::IsMouseHoveringRect(inner_min, inner_max);

    ImVec2 expand(10, 10);
    bool is_hovered = ImGui::IsMouseHoveringRect(
        inner_min - expand,
        inner_max + expand
    );

    // SELECT
    /*if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        ImVec2 mouse = io.MousePos;

        selected_keyframe = -1;
        editing_in_tangent = false;
        editing_out_tangent = false;

        for (size_t i = 0; i < curve.keyframes.size(); i++)
        {
            const Keyframe& kf = curve.keyframes[i];

            ImVec2 kf_pos(
                inner_min.x + (kf.time - clamp_min_time) / (clamp_max_time - clamp_min_time) * inner_size.x,
                inner_min.y + (clamp_max_value - kf.value) / (clamp_max_value - clamp_min_value) * inner_size.y
            );

            if (ImLengthSqr(mouse - kf_pos) < 25.0f)
            {
                selected_keyframe = (int)i;
                break;
            }
        }
    }*/
   // SELECT keyframe or tangent
    if(is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
        ImVec2 mouse = io.MousePos;

        selected_keyframe = -1;
        editing_in_tangent = false;
        editing_out_tangent = false;

        bool hit = false;

        for(size_t i = 0; i < curve.keyframes.size(); i++){
            const Keyframe& kf = curve.keyframes[i];

            ImVec2 kf_pos(
                inner_min.x + (kf.time - clamp_min_time) / (clamp_max_time - clamp_min_time) * inner_size.x,
                inner_min.y + (clamp_max_value - kf.value) / (clamp_max_value - clamp_min_value) * inner_size.y
            );


            // First check tangents
            if(kf.curve_type == CurveType::Smooth){
                ImVec2 in_pos  = kf_pos + kf.in_tangent;
                ImVec2 out_pos = kf_pos + kf.out_tangent;


                if(ImLengthSqr(mouse - in_pos) < 64.0f){
                    selected_keyframe = (int)i;
                    editing_in_tangent = true;
                    editing_out_tangent = false;
                    hit = true;
                    break;
                }


                if(ImLengthSqr(mouse - out_pos) < 64.0f){
                    selected_keyframe = (int)i;
                    editing_in_tangent = false;
                    editing_out_tangent = true;
                    hit = true;
                    break;
                }
            }


            // Then check keyframe
            if(ImLengthSqr(mouse - kf_pos) < 64.0f){
                selected_keyframe = (int)i;
                editing_in_tangent = false;
                editing_out_tangent = false;
                hit = true;
                break;
            }
        }

        if(!hit){
            selected_keyframe = -1;
        }
    }

    // ADD
    if(is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
        ImVec2 m = io.MousePos;

        float time = clamp_min_time + (m.x - inner_min.x) / inner_size.x * (clamp_max_time - clamp_min_time);
        float value = clamp_max_value - (m.y - inner_min.y) / inner_size.y * (clamp_max_value - clamp_min_value);

        time  = std::clamp(time, clamp_min_time, clamp_max_time);
        value = std::clamp(value, clamp_min_value, clamp_max_value);

        curve.AddKeyframe(time, value, CurveType::Linear);
    }

    // DELETE
    if(is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)){
        ImVec2 m = io.MousePos;

        for(size_t i = 0; i < curve.keyframes.size(); i++){
            const Keyframe& kf = curve.keyframes[i];

            ImVec2 kf_pos(
                inner_min.x + (kf.time - clamp_min_time) / (clamp_max_time - clamp_min_time) * inner_size.x,
                inner_min.y + (clamp_max_value - kf.value) / (clamp_max_value - clamp_min_value) * inner_size.y
            );

            if(ImLengthSqr(m - kf_pos) < 25.0f){
                curve.keyframes.erase(curve.keyframes.begin() + i);
                if(selected_keyframe == (int)i) selected_keyframe = -1;
                break;
            }
        }
    }

    /*// DRAG
    if (is_dragging && selected_keyframe >= 0)
    {
        ImVec2 m = io.MousePos;
        Keyframe& kf = curve.keyframes[selected_keyframe];

        float time = clamp_min_time + (m.x - inner_min.x) / inner_size.x * (clamp_max_time - clamp_min_time);
        float value = clamp_max_value - (m.y - inner_min.y) / inner_size.y * (clamp_max_value - clamp_min_value);

        kf.time  = std::clamp(time, clamp_min_time, clamp_max_time);
        kf.value = std::clamp(value, clamp_min_value, clamp_max_value);

        curve.SortKeyframes();
    }*/

    // DRAG keyframe or tangent
    if(is_dragging && selected_keyframe >= 0){
        ImVec2 mouse = io.MousePos;

        Keyframe& kf = curve.keyframes[selected_keyframe];

        ImVec2 kf_pos(
            inner_min.x + (kf.time - clamp_min_time) / (clamp_max_time - clamp_min_time) * inner_size.x,
            inner_min.y + (clamp_max_value - kf.value) / (clamp_max_value - clamp_min_value) * inner_size.y
        );

        if(editing_in_tangent){
            kf.in_tangent = mouse - kf_pos;

            // keep tangent pointing left
            kf.in_tangent.x = std::min(kf.in_tangent.x, -1.0f);
        } else if (editing_out_tangent){
            kf.out_tangent = mouse - kf_pos;

            // keep tangent pointing right
            kf.out_tangent.x = std::max(kf.out_tangent.x, 1.0f);
        } else {
            float time =
                clamp_min_time +
                (mouse.x - inner_min.x) / inner_size.x *
                (clamp_max_time - clamp_min_time);


            float value =
                clamp_max_value -
                (mouse.y - inner_min.y) / inner_size.y *
                (clamp_max_value - clamp_min_value);


            kf.time = std::clamp(time, clamp_min_time, clamp_max_time);
            kf.value = std::clamp(value, clamp_min_value, clamp_max_value);

            curve.SortKeyframes();
        }
    }

    ImGui::PushID(&curve);
    ImGui::InvisibleButton("curve_canvas", canvas_size);
    ImGui::PopID();

    // UI
    if(selected_keyframe >= 0 && selected_keyframe < (int)curve.keyframes.size()){
        Keyframe& kf = curve.keyframes[selected_keyframe];

        ImGui::InputFloat("Time", &kf.time);
        ImGui::InputFloat("Value", &kf.value);

        kf.time  = std::clamp(kf.time, clamp_min_time, clamp_max_time);
        kf.value = std::clamp(kf.value, clamp_min_value, clamp_max_value);

        curve.SortKeyframes();

        const char* curve_types[] = { "Linear", "Constant", "Smooth", "AutoSmooth" };
        int current_type = (int)kf.curve_type;
        if(ImGui::Combo("Curve Type", &current_type, curve_types, IM_ARRAYSIZE(curve_types))){
            kf.curve_type = (CurveType)current_type;
        }

        if(kf.curve_type == CurveType::Smooth){
            ImGui::InputFloat2("In Tangent", &kf.in_tangent.x, "%.2f");
            ImGui::InputFloat2("Out Tangent", &kf.out_tangent.x, "%.2f");
            //kf.in_tangent.x  = std::clamp(kf.in_tangent.x, -50.0f, -5.0f);
            //kf.out_tangent.x = std::clamp(kf.out_tangent.x, 5.0f, 50.0f);
        }
    }

    ImGui::InputFloat2("MinMaxTime", &curve.minMaxTime.x);
    ImGui::InputFloat2("MinMaxValue", &curve.minMaxValue.x);
}
#else
void DrawAnimationCurveEditor(AnimationCurve& curve, ImVec2 size){
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = size;
    canvas_size.x = std::max(canvas_size.x, 50.0f);
    canvas_size.y = std::max(canvas_size.y, 50.0f);

    // Reserve the area first
    //ImGui::InvisibleButton("curve_canvas", canvas_size);

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Background
    draw_list->AddRectFilled(canvas_pos, canvas_pos + canvas_size, IM_COL32(50, 50, 50, 255));

    // Selection state
    /*static int selected_keyframe = -1;
    static bool editing_in_tangent = false;
    static bool editing_out_tangent = false;*/

    int& selected_keyframe = curve.gui_selected_keyframe;
    bool& editing_in_tangent = curve.gui_editing_in_tangent;
    bool& editing_out_tangent = curve.gui_editing_out_tangent;

    bool is_dragging = (selected_keyframe >= 0 && ImGui::IsMouseDragging(ImGuiMouseButton_Left));

    // --- Update min/max only if user is not dragging/editing ---
    if(!is_dragging){
        //if(curve.autoUpdateMinMax) curve.UpdateMinMax();
    }

    float clamp_min_time  = curve.minMaxTime.x;
    float clamp_max_time  = curve.minMaxTime.y;
    float clamp_min_value = curve.minMaxValue.x;
    float clamp_max_value = curve.minMaxValue.y;

    // Padding for visual only
    //float draw_pad_time  = 0.05f;
    //float draw_pad_value = 0.1f;

    float time_range  = clamp_max_time  - clamp_min_time;
    float value_range = clamp_max_value - clamp_min_value;
    // Prevent division issues
    if(time_range <= 0.0001f)  time_range  = 1.0f;
    if(value_range <= 0.0001f) value_range = 1.0f;
    // % padding (this keeps aspect consistent)
    float draw_pad_time  = time_range  * 0.05f;  // 5%
    float draw_pad_value = value_range * 0.1f;  // 5%

    float draw_min_time  = clamp_min_time - draw_pad_time;
    float draw_max_time  = clamp_max_time + draw_pad_time;
    float draw_min_value = clamp_min_value - draw_pad_value;
    float draw_max_value = clamp_max_value + draw_pad_value;

    // INNER RECT (REAL EDITOR SPACE)
    ImVec2 inner_min(
        canvas_pos.x + (clamp_min_time - draw_min_time) / (draw_max_time - draw_min_time) * canvas_size.x,
        canvas_pos.y + (draw_max_value - clamp_max_value) / (draw_max_value - draw_min_value) * canvas_size.y
    );

    ImVec2 inner_max(
        canvas_pos.x + (clamp_max_time - draw_min_time) / (draw_max_time - draw_min_time) * canvas_size.x,
        canvas_pos.y + (draw_max_value - clamp_min_value) / (draw_max_value - draw_min_value) * canvas_size.y
    );

    ImVec2 inner_size = inner_max - inner_min;

    auto TangentToScreen = [&](const ImVec2& tangent){
        return ImVec2(
            tangent.x / (clamp_max_time - clamp_min_time) * inner_size.x,
            -tangent.y / (clamp_max_value - clamp_min_value) * inner_size.y
        );
    };

    auto ScreenToTangent = [&](const ImVec2& delta){
        return ImVec2(
            delta.x / inner_size.x *
            (clamp_max_time - clamp_min_time),

            -delta.y / inner_size.y *
            (clamp_max_value - clamp_min_value)
        );
    };

    /////////////////
    /*// Draw numerical values for min/max time and value
    char buffer[32];
    // Min time (at x=0, bottom left)
    snprintf(buffer, sizeof(buffer), "%.1f", clamp_min_time);
    draw_list->AddText(ImVec2(canvas_pos.x, canvas_pos.y + canvas_size.y - 30.0f), IM_COL32(255, 255, 255, 255), buffer);
    // Max time (at x=max, bottom right)
    snprintf(buffer, sizeof(buffer), "%.1f", clamp_max_time);
    draw_list->AddText(ImVec2(canvas_pos.x + canvas_size.x - 20.0f, canvas_pos.y + canvas_size.y - 15.0f), IM_COL32(255, 255, 255, 255), buffer);
    // Min value (at y=max, bottom left)
    snprintf(buffer, sizeof(buffer), "%.1f", clamp_min_value);
    draw_list->AddText(ImVec2(canvas_pos.x, canvas_pos.y + canvas_size.y - 15.0f), IM_COL32(255, 255, 255, 255), buffer);
    // Max value (at y=0, top left)
    snprintf(buffer, sizeof(buffer), "%.1f", clamp_max_value);
    draw_list->AddText(ImVec2(canvas_pos.x, canvas_pos.y + 0), IM_COL32(255, 255, 255, 255), buffer);*/
    /////////////////

    // Draw clamp rect
    draw_list->AddRect(inner_min, inner_max, IM_COL32(150, 150, 150, 255));

    // Border
    draw_list->AddRect(canvas_pos, canvas_pos + canvas_size, IM_COL32(100, 100, 100, 255));

    // Grid (inside inner rect)
    /*for(float t = clamp_min_time; t <= clamp_max_time; t += 0.1f){
        float x = inner_min.x + (t - clamp_min_time) / (clamp_max_time - clamp_min_time) * inner_size.x;
        draw_list->AddLine(ImVec2(x, inner_min.y), ImVec2(x, inner_max.y), IM_COL32(100, 100, 100, 50));
    }

    for(float v = clamp_min_value; v <= clamp_max_value; v += 0.1f){
        float y = inner_min.y + (clamp_max_value - v) / (clamp_max_value - clamp_min_value) * inner_size.y;
        draw_list->AddLine(ImVec2(inner_min.x, y), ImVec2(inner_max.x, y), IM_COL32(100, 100, 100, 50));
    }*/

    // Clip to inner rect
    draw_list->PushClipRect(inner_min, inner_max, true);

    /*// Draw curve
    const int segments = 100;
    for (int i = 0; i < segments; i++)
    {
        float t0 = clamp_min_time + (float)i / segments * (clamp_max_time - clamp_min_time);
        float t1 = clamp_min_time + (float)(i + 1) / segments * (clamp_max_time - clamp_min_time);

        float v0 = curve.Evaluate(t0);
        float v1 = curve.Evaluate(t1);

        ImVec2 p0(
            inner_min.x + (t0 - clamp_min_time) / (clamp_max_time - clamp_min_time) * inner_size.x,
            inner_min.y + (clamp_max_value - v0) / (clamp_max_value - clamp_min_value) * inner_size.y
        );

        ImVec2 p1(
            inner_min.x + (t1 - clamp_min_time) / (clamp_max_time - clamp_min_time) * inner_size.x,
            inner_min.y + (clamp_max_value - v1) / (clamp_max_value - clamp_min_value) * inner_size.y
        );

        draw_list->AddLine(p0, p1, IM_COL32(255, 255, 0, 255), 2.0f);
    }*/

    // Draw curve
    const int curve_segments = 100;
    for(int i = 0; i < curve_segments; ++i){
        float t0 = draw_min_time + (float)i / curve_segments * (draw_max_time - draw_min_time);
        float t1 = draw_min_time + (float)(i + 1) / curve_segments * (draw_max_time - draw_min_time);
        float v0 = curve.Evaluate(t0);
        float v1 = curve.Evaluate(t1);

        ImVec2 p0(canvas_pos.x + (t0 - draw_min_time) / (draw_max_time - draw_min_time) * canvas_size.x,
                  canvas_pos.y + (draw_max_value - v0) / (draw_max_value - draw_min_value) * canvas_size.y);
        ImVec2 p1(canvas_pos.x + (t1 - draw_min_time) / (draw_max_time - draw_min_time) * canvas_size.x,
                  canvas_pos.y + (draw_max_value - v1) / (draw_max_value - draw_min_value) * canvas_size.y);
        draw_list->AddLine(p0, p1, IM_COL32(255, 255, 0, 255), 2.0f);
    }

    draw_list->PopClipRect();

    // Draw keyframes + tangents
    for(size_t i = 0; i < curve.keyframes.size(); i++){
        const Keyframe& kf = curve.keyframes[i];

        ImVec2 kf_pos(
            inner_min.x + (kf.time - clamp_min_time) / (clamp_max_time - clamp_min_time) * inner_size.x,
            inner_min.y + (clamp_max_value - kf.value) / (clamp_max_value - clamp_min_value) * inner_size.y
        );

        if(kf.curve_type == CurveType::Smooth){
        //if (kf.curve_type == CurveType::Smooth || (i < curve.keyframes.size() - 1 && curve.keyframes[i + 1].curve_type == CurveType::Smooth)){
            //ImVec2 in_pos  = kf_pos + kf.in_tangent;
            //ImVec2 out_pos = kf_pos + kf.out_tangent;
            ImVec2 in_pos  = kf_pos + TangentToScreen(kf.in_tangent);
            ImVec2 out_pos = kf_pos + TangentToScreen(kf.out_tangent);

            draw_list->AddLine(kf_pos, in_pos, IM_COL32(0, 255, 255, 255));
            draw_list->AddLine(kf_pos, out_pos, IM_COL32(0, 255, 255, 255));
            draw_list->AddCircleFilled(in_pos, 3.0f, IM_COL32(0, 255, 255, 255));
            draw_list->AddCircleFilled(out_pos, 3.0f, IM_COL32(0, 255, 255, 255));
        }

        uint32_t col = (selected_keyframe == (int)i) ? IM_COL32(255, 100, 100, 255) : IM_COL32(255, 0, 0, 255);
        draw_list->AddCircleFilled(kf_pos, 5.0f, col);
    }

    // Hover only inner rect
    //bool is_hovered = ImGui::IsMouseHoveringRect(inner_min, inner_max);

    ImVec2 expand(10, 10);
    bool is_hovered = ImGui::IsMouseHoveringRect(
        inner_min - expand,
        inner_max + expand
    );

    // SELECT
    /*if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        ImVec2 mouse = io.MousePos;

        selected_keyframe = -1;
        editing_in_tangent = false;
        editing_out_tangent = false;

        for (size_t i = 0; i < curve.keyframes.size(); i++)
        {
            const Keyframe& kf = curve.keyframes[i];

            ImVec2 kf_pos(
                inner_min.x + (kf.time - clamp_min_time) / (clamp_max_time - clamp_min_time) * inner_size.x,
                inner_min.y + (clamp_max_value - kf.value) / (clamp_max_value - clamp_min_value) * inner_size.y
            );

            if (ImLengthSqr(mouse - kf_pos) < 25.0f)
            {
                selected_keyframe = (int)i;
                break;
            }
        }
    }*/
   // SELECT keyframe or tangent
    if(is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
        ImVec2 mouse = io.MousePos;

        selected_keyframe = -1;
        editing_in_tangent = false;
        editing_out_tangent = false;

        bool hit = false;

        for(size_t i = 0; i < curve.keyframes.size(); i++){
            const Keyframe& kf = curve.keyframes[i];

            ImVec2 kf_pos(
                inner_min.x + (kf.time - clamp_min_time) / (clamp_max_time - clamp_min_time) * inner_size.x,
                inner_min.y + (clamp_max_value - kf.value) / (clamp_max_value - clamp_min_value) * inner_size.y
            );


            // First check tangents
            if(kf.curve_type == CurveType::Smooth){
                //ImVec2 in_pos  = kf_pos + kf.in_tangent;
                //ImVec2 out_pos = kf_pos + kf.out_tangent;
                ImVec2 in_pos  = kf_pos + TangentToScreen(kf.in_tangent);
                ImVec2 out_pos = kf_pos + TangentToScreen(kf.out_tangent);

                if(ImLengthSqr(mouse - in_pos) < 64.0f){
                    selected_keyframe = (int)i;
                    editing_in_tangent = true;
                    editing_out_tangent = false;
                    hit = true;
                    break;
                }


                if(ImLengthSqr(mouse - out_pos) < 64.0f){
                    selected_keyframe = (int)i;
                    editing_in_tangent = false;
                    editing_out_tangent = true;
                    hit = true;
                    break;
                }
            }


            // Then check keyframe
            if(ImLengthSqr(mouse - kf_pos) < 64.0f){
                selected_keyframe = (int)i;
                editing_in_tangent = false;
                editing_out_tangent = false;
                hit = true;
                break;
            }
        }

        if(!hit){
            selected_keyframe = -1;
        }
    }

    // ADD
    if(is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
        ImVec2 m = io.MousePos;

        float time = clamp_min_time + (m.x - inner_min.x) / inner_size.x * (clamp_max_time - clamp_min_time);
        float value = clamp_max_value - (m.y - inner_min.y) / inner_size.y * (clamp_max_value - clamp_min_value);

        time  = std::clamp(time, clamp_min_time, clamp_max_time);
        value = std::clamp(value, clamp_min_value, clamp_max_value);

        curve.AddKeyframe(time, value, CurveType::Linear);
    }

    // DELETE
    if(is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)){
        ImVec2 m = io.MousePos;

        for(size_t i = 0; i < curve.keyframes.size(); i++){
            const Keyframe& kf = curve.keyframes[i];

            ImVec2 kf_pos(
                inner_min.x + (kf.time - clamp_min_time) / (clamp_max_time - clamp_min_time) * inner_size.x,
                inner_min.y + (clamp_max_value - kf.value) / (clamp_max_value - clamp_min_value) * inner_size.y
            );

            if(ImLengthSqr(m - kf_pos) < 25.0f){
                curve.keyframes.erase(curve.keyframes.begin() + i);
                if(selected_keyframe == (int)i) selected_keyframe = -1;
                break;
            }
        }
    }

    /*// DRAG
    if (is_dragging && selected_keyframe >= 0)
    {
        ImVec2 m = io.MousePos;
        Keyframe& kf = curve.keyframes[selected_keyframe];

        float time = clamp_min_time + (m.x - inner_min.x) / inner_size.x * (clamp_max_time - clamp_min_time);
        float value = clamp_max_value - (m.y - inner_min.y) / inner_size.y * (clamp_max_value - clamp_min_value);

        kf.time  = std::clamp(time, clamp_min_time, clamp_max_time);
        kf.value = std::clamp(value, clamp_min_value, clamp_max_value);

        curve.SortKeyframes();
    }*/

    // DRAG keyframe or tangent
    if(is_dragging && selected_keyframe >= 0){
        ImVec2 mouse = io.MousePos;

        Keyframe& kf = curve.keyframes[selected_keyframe];

        ImVec2 kf_pos(
            inner_min.x + (kf.time - clamp_min_time) / (clamp_max_time - clamp_min_time) * inner_size.x,
            inner_min.y + (clamp_max_value - kf.value) / (clamp_max_value - clamp_min_value) * inner_size.y
        );

        if(editing_in_tangent){
            kf.in_tangent = ScreenToTangent(mouse - kf_pos);

            // keep left
            kf.in_tangent.x = std::min(kf.in_tangent.x, -0.001f);
        } else if(editing_out_tangent){
            kf.out_tangent = ScreenToTangent(mouse - kf_pos);

            // keep right
            kf.out_tangent.x = std::max(kf.out_tangent.x, 0.001f);
        } else {
            float time =
                clamp_min_time +
                (mouse.x - inner_min.x) / inner_size.x *
                (clamp_max_time - clamp_min_time);


            float value =
                clamp_max_value -
                (mouse.y - inner_min.y) / inner_size.y *
                (clamp_max_value - clamp_min_value);


            kf.time = std::clamp(time, clamp_min_time, clamp_max_time);
            kf.value = std::clamp(value, clamp_min_value, clamp_max_value);

            curve.SortKeyframes();
        }
    }

    ImGui::PushID(&curve);
    ImGui::InvisibleButton("curve_canvas", canvas_size);
    ImGui::PopID();

    // UI
    if(selected_keyframe >= 0 && selected_keyframe < (int)curve.keyframes.size()){
        Keyframe& kf = curve.keyframes[selected_keyframe];

        bool updateMinMax = false;

        if(ImGui::InputFloat("Time", &kf.time)) updateMinMax = true;
        if(ImGui::InputFloat("Value", &kf.value)) updateMinMax = true;

        kf.time  = std::clamp(kf.time, clamp_min_time, clamp_max_time);
        kf.value = std::clamp(kf.value, clamp_min_value, clamp_max_value);

        curve.SortKeyframes();
        //if(updateMinMax && curve.autoUpdateMinMax) curve.UpdateMinMax();

        const char* curve_types[] = { "Linear", "Constant", "Smooth", "AutoSmooth" };
        int current_type = (int)kf.curve_type;
        if(ImGui::Combo("Curve Type", &current_type, curve_types, IM_ARRAYSIZE(curve_types))){
            kf.curve_type = (CurveType)current_type;
        }

        if(kf.curve_type == CurveType::Smooth){
            ImGui::InputFloat2("In Tangent", &kf.in_tangent.x, "%.2f");
            ImGui::InputFloat2("Out Tangent", &kf.out_tangent.x, "%.2f");
            //kf.in_tangent.x  = std::clamp(kf.in_tangent.x, -50.0f, -5.0f);
            //kf.out_tangent.x = std::clamp(kf.out_tangent.x, 5.0f, 50.0f);
        }
    } else {
        ImGui::InputFloat2("MinMaxTime", &curve.minMaxTime.x);
        ImGui::InputFloat2("MinMaxValue", &curve.minMaxValue.x);
        //ImGui::Checkbox("AutoUpdateMinMax", &curve.autoUpdateMinMax);
    }
}

#endif

}