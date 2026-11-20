#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace FCB{

// The variable-size records from CmdTest's FastCommandBuffer, with a growing
// arena and insertion for ImGui. Only trivially copyable payloads are stored.
template<typename CommandType, size_t Alignment, typename CommandSizeType = uint16_t>
class FastCommandBuffer{
    static_assert(std::is_enum_v<CommandType>);
    static_assert(Alignment != 0 && (Alignment & (Alignment - 1)) == 0);

    struct Header{
        CommandType type;
        CommandSizeType totalSize;
    };

    static constexpr size_t AlignUp(size_t value){
        return (value + Alignment - 1) & ~(Alignment - 1);
    }

    static constexpr size_t PayloadOffset = sizeof(Header);
    static constexpr size_t AllocationAlignment =
        Alignment > alignof(std::max_align_t) ? Alignment : alignof(std::max_align_t);

    struct MemoryDeleter{
        void operator()(std::byte* memory) const noexcept{
            ::operator delete(memory, std::align_val_t(AllocationAlignment));
        }
    };

public:
    FastCommandBuffer() = default;
    FastCommandBuffer(FastCommandBuffer&& other) noexcept { *this = std::move(other); }

    FastCommandBuffer& operator=(FastCommandBuffer&& other) noexcept{
        if(this != &other){
            data = std::move(other.data);
            size = std::exchange(other.size, 0);
            capacity = std::exchange(other.capacity, 0);
        }
        return *this;
    }

    void Reset() noexcept { size = 0; }
    size_t GetUsedBytes() const noexcept { return size; }

    template<typename Payload>
    static Payload Read(const void* data){
        static_assert(std::is_trivially_copyable_v<Payload>);
        Payload payload;
        std::memcpy(&payload, data, sizeof(payload));
        return payload;
    }

    template<CommandType Type, typename Payload, typename... Args>
    void Emplace(Args&&... args){
        auto* memory = Allocate<Payload>(Type, size);
        Write<Payload>(memory, std::forward<Args>(args)...);
    }

    template<CommandType Type, typename Payload, typename... Args>
    void Insert(size_t offset, Args&&... args){
        assert(offset <= size);
        auto* memory = Allocate<Payload>(Type, offset);
        Write<Payload>(memory, std::forward<Args>(args)...);
    }

    template<typename Visitor>
    void ForEach(Visitor&& visitor) const{
        for(size_t offset = 0; offset < size;){
            auto* record = data.get() + offset;
            Header header;
            std::memcpy(&header, record, sizeof(header));
            visitor(header.type, record + PayloadOffset);
            offset += header.totalSize;
        }
    }

private:
    template<typename Payload, typename... Args>
    static void Write(std::byte* memory, Args&&... args){
        if constexpr(Alignment % alignof(Payload) == 0 && PayloadOffset % alignof(Payload) == 0){
            ::new(static_cast<void*>(memory)) Payload{std::forward<Args>(args)...};
        } else {
            const Payload payload{std::forward<Args>(args)...};
            std::memcpy(memory, &payload, sizeof(payload));
        }
    }

    template<typename Payload>
    std::byte* Allocate(CommandType type, size_t offset){
        static_assert(std::is_trivially_copyable_v<Payload>);
        constexpr size_t totalSize = AlignUp(PayloadOffset + sizeof(Payload));
        static_assert(totalSize <= std::numeric_limits<CommandSizeType>::max());

        if(totalSize > capacity - size) Grow(totalSize);
        auto* record = data.get() + offset;
        if(offset != size){
            std::memmove(record + totalSize, record, size - offset);
        }
        const Header header{type, static_cast<CommandSizeType>(totalSize)};
        std::memcpy(record, &header, sizeof(header));
        size += totalSize;
        return record + PayloadOffset;
    }

    void Grow(size_t requiredBytes){
        constexpr size_t maxCapacity = std::numeric_limits<size_t>::max() & ~(Alignment - 1);
        if(requiredBytes > maxCapacity - size) throw std::bad_array_new_length{};

        size_t newCapacity = capacity == 0 ? 64 * 1024 : capacity;
        const size_t requiredCapacity = size + requiredBytes;
        while(newCapacity < requiredCapacity){
            newCapacity = newCapacity <= maxCapacity / 2 ? newCapacity * 2 : requiredCapacity;
        }

        std::unique_ptr<std::byte, MemoryDeleter> newData(static_cast<std::byte*>(
            ::operator new(newCapacity, std::align_val_t(AllocationAlignment))));
        if(size != 0) std::memcpy(newData.get(), data.get(), size);
        data = std::move(newData);
        capacity = newCapacity;
    }

    std::unique_ptr<std::byte, MemoryDeleter> data;
    size_t size = 0;
    size_t capacity = 0;
};

}
