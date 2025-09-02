#pragma once
#include <vector>
#include "OD/Core/ImGui.h"
#include "OD/Serialization/CerealImGui.h"
#include "OD/Serialization/Serialization.h"

using namespace OD;

namespace cereal {
    template<class Archive>
    void serialize(Archive& archive, ImVec2& vec) {
        archive(cereal::make_nvp("x", vec.x), cereal::make_nvp("y", vec.y));
    }
}

namespace Standard{

// Enum for curve types
enum class CurveType {
    Linear,
    Constant,
    Smooth
};

// Keyframe structure with tangents and curve type
struct Keyframe {
    float time;      // X-axis (time)
    float value;     // Y-axis (value)
    ImVec2 in_tangent; // 2D position of incoming tangent (relative to keyframe) //TODO: Change this to Vector2
    ImVec2 out_tangent; // 2D position of outgoing tangent (relative to keyframe)
    CurveType curve_type; // Type of curve after this keyframe
    Keyframe() = default;
    Keyframe(float t, float v, CurveType type = CurveType::Linear)
        : time(t), value(v), in_tangent(-10.0f, 0.0f), out_tangent(10.0f, 0.0f), curve_type(type) {}

    // Cereal serialization
    template<class Archive>
    void serialize(Archive& ar) {
        ArchiveDumpNVP(ar, time);
        ArchiveDumpNVP(ar, value);
        ArchiveDumpNVP(ar, in_tangent);
        ArchiveDumpNVP(ar, out_tangent);
        ArchiveDumpNVP(ar, curve_type);
        /*ar(
            cereal::make_nvp("time", time),
            cereal::make_nvp("value", value),
            cereal::make_nvp("in_tangent", in_tangent),
            cereal::make_nvp("out_tangent", out_tangent),
            cereal::make_nvp("curve_type", reinterpret_cast<int&>(curve_type))
        );*/
    }
};

// Animation curve class
class AnimationCurve {
public:
    std::vector<Keyframe> keyframes;

    // Add a keyframe
    void AddKeyframe(float time, float value, CurveType type = CurveType::Linear);

    // Evaluate the curve at a given time
    float Evaluate(float time) const;

    // Sort keyframes by time
    void SortKeyframes();

    // Cereal serialization
    template<class Archive>
    void serialize(Archive& ar) {
        ArchiveDumpNVP(ar, keyframes);
        //archive(cereal::make_nvp("keyframes", keyframes));
    }

    void OnGui(cereal::ImGuiArchive& ar);
};

void DrawCurvePreview(AnimationCurve& curve, ImVec2 size = ImVec2(100, 50), bool* open_editor = nullptr);

// Function to draw the curve editor
void DrawAnimationCurveEditor(AnimationCurve& curve, ImVec2 size = ImVec2(400, 200));

}