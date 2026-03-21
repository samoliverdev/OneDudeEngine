#pragma once
#include <cstddef>   // size_t, ptrdiff_t
#include <cstdlib>   // aligned_alloc, free
#include <new>       // std::bad_alloc

#ifdef _MSC_VER
#include <malloc.h>  // _aligned_malloc, _aligned_free
#endif

namespace OD{

/*template<typename T, size_t Alignment>
struct AlignedAllocator {
    using value_type = T;
    T* allocate(size_t n) {
        #ifdef _MSC_VER
        return static_cast<T*>(_aligned_malloc(n * sizeof(T), Alignment));
        #else
        return static_cast<T*>(std::aligned_alloc(Alignment, n * sizeof(T)));
        #endif
    }
    void deallocate(T* p, size_t) noexcept {
        #ifdef _MSC_VER
        _aligned_free(p);
        #else
        std::free(p);
        #endif
    }
    template<typename U> struct rebind { using other = AlignedAllocator<U, Alignment>; };
};*/

template<typename T, std::size_t Alignment>
struct AlignedAllocator {
    static_assert(Alignment >= alignof(void*), "Alignment must be >= alignof(void*)");
    static_assert((Alignment & (Alignment - 1)) == 0, "Alignment must be a power of two");

    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    AlignedAllocator() noexcept = default;

    // templated converting constructor required by the standard library
    template<typename U>
    constexpr AlignedAllocator(const AlignedAllocator<U, Alignment>&) noexcept {}

    // allocate n elements of T, rounding size up to a multiple of Alignment for std::aligned_alloc
    T* allocate(size_type n) {
        if (n == 0) return nullptr;

        size_type size = n * sizeof(T);

    #ifdef _MSC_VER
        void* p = _aligned_malloc(size, Alignment);
        if (!p) throw std::bad_alloc();
        return static_cast<T*>(p);
    #else
        // std::aligned_alloc requires size % alignment == 0
        size_type aligned_size = ((size + Alignment - 1) / Alignment) * Alignment;
        void* p = std::aligned_alloc(Alignment, aligned_size);
        if (!p) throw std::bad_alloc();
        return static_cast<T*>(p);
    #endif
    }

    void deallocate(T* p, size_type) noexcept {
        if (!p) return;
    #ifdef _MSC_VER
        _aligned_free(p);
    #else
        std::free(p);
    #endif
    }

    // rebind for older allocator usage (kept for compatibility)
    template<typename U>
    struct rebind { using other = AlignedAllocator<U, Alignment>; };

    // equality — many containers rely on this
    template<typename U>
    bool operator==(const AlignedAllocator<U, Alignment>&) const noexcept { return true; }

    template<typename U>
    bool operator!=(const AlignedAllocator<U, Alignment>&) const noexcept { return false; }
};

// Template alias for aligned vectors
template<typename T>
using AlignedVector = std::vector<T, AlignedAllocator<T, 16>>;


}