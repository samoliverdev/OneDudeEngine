#include "OD/pch.h"
#include "MemoryTracker.h"

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#include <atomic>
#endif

namespace OD::MemoryTracker {

#if defined(_MSC_VER) && defined(_DEBUG)
namespace {
std::atomic<bool> trackingFrame{false};
std::atomic<std::uint64_t> frameAllocations{0};
std::atomic<std::uint64_t> frameBytes{0};
std::atomic<std::size_t> breakpointMinimumBytes{0};
std::atomic<bool> started{false};

int AllocationHook(int allocationType, void*, std::size_t size, int, long, const unsigned char*, int){
    if (allocationType != _HOOK_ALLOC && allocationType != _HOOK_REALLOC)
        return 1;

    const std::size_t breakpoint = breakpointMinimumBytes.load(std::memory_order_relaxed);
    if (breakpoint != 0 && size >= breakpoint){
        //_CrtDbgBreak();
        int dammyStopCode = 200;
    }

    if (trackingFrame.load(std::memory_order_relaxed)) {
        frameAllocations.fetch_add(1, std::memory_order_relaxed);
        frameBytes.fetch_add(size, std::memory_order_relaxed);
    }
    return 1;
}
}
#endif

void Start() {
#if defined(_MSC_VER) && defined(_DEBUG)
    bool expected = false;
    if (started.compare_exchange_strong(expected, true))
        _CrtSetAllocHook(AllocationHook);
#endif
}

void BeginFrame() {
#if defined(_MSC_VER) && defined(_DEBUG)
    Start();
    frameAllocations.store(0, std::memory_order_relaxed);
    frameBytes.store(0, std::memory_order_relaxed);
    trackingFrame.store(true, std::memory_order_release);
#endif
}

FrameStats EndFrame() {
    FrameStats result;
#if defined(_MSC_VER) && defined(_DEBUG)
    trackingFrame.store(false, std::memory_order_release);
    result.allocationCount = frameAllocations.load(std::memory_order_relaxed);
    result.allocatedBytes = frameBytes.load(std::memory_order_relaxed);
#endif
    return result;
}

void SetAllocationBreakpointSize(std::size_t minimumBytes) {
#if defined(_MSC_VER) && defined(_DEBUG)
    breakpointMinimumBytes.store(minimumBytes, std::memory_order_relaxed);
#else
    (void)minimumBytes;
#endif
}

}
