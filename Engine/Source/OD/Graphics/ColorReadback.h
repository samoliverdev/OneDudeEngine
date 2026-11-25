#pragma once
#include <cstdint>
#include <functional>
#include <vector>

namespace OD{

enum class ColorReadbackResult{ Success, Unsupported, InvalidAttachment, ReadFailed };

struct ColorReadback{
    ColorReadbackResult result = ColorReadbackResult::ReadFailed;
    int width = 0;
    int height = 0;
    // Tightly packed RGBA8, with the top row first.
    std::vector<uint8_t> rgba8;
};

using ColorReadbackCallback = std::function<void(ColorReadback)>;

}
