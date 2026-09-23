#pragma once

#include "OD/Defines.h"
#include <cstddef>
#include <cstdint>

namespace OD::MemoryTracker {

struct FrameStats {
    std::uint64_t allocationCount = 0;
    std::uint64_t allocatedBytes = 0;
};

// Tracks CRT heap allocations made while a frame is active. Available in MSVC
// debug builds; other configurations safely return zero stats.
OD_API void Start();
OD_API void BeginFrame();
OD_API FrameStats EndFrame();

// Break into the debugger on an allocation whose requested size is at least
// this value. Pass zero to disable the size breakpoint.
OD_API void SetAllocationBreakpointSize(std::size_t minimumBytes);

}
