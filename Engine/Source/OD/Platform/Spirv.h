#pragma once
#include "OD/Graphics/RendererTypes.h"

namespace OD{

bool SpirvReflect(int set, int bind, void* data, size_t size, UniformBufferDef& out);

}