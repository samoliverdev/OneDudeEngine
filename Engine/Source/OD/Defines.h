#pragma once

//#define FINAL_BUILD

#define TestNewGPU_API

#ifdef _WIN32
    #define EXPORT_FN __declspec(dllexport)
#else 
    #define EXPORT_FN
#endif

#ifdef _WIN32
    #ifdef OD_BUILD_DLL
        #define OD_API __declspec(dllexport)
        #define OD_API_IMPORT
    #else
        #define OD_API __declspec(dllimport)
        #define OD_API_IMPORT __declspec(dllimport)
    #endif
#else 
    #define OD_API
    #define OD_API_IMPORT
#endif

#ifndef FINAL_BUILD
    #define OD_PROFILE 1
#else
    #define OD_PROFILE 0
#endif

#define FILE_MOVE_PAYLOAD "FILE_MOVE_PAYLOAD"
#define GRAPHIC_LOG_ERROR

#define INVALID_ID 0
#define MAX_DELTATIME 0.05f

#define InternalSystemsMulthread 1

#define DEFINE_COPY_MOVE_CONSTRUCTORS_FROM_COPY_MOVE_FUNC(ClassName)                           \
    ClassName(const ClassName& other) { Copy(other); }                     \
    ClassName& operator=(const ClassName& other) {                         \
        if (this != &other) Copy(other);                                   \
        return *this;                                                      \
    }                                                                      \
    ClassName(ClassName&& other) noexcept { Move(std::move(other)); }     \
    ClassName& operator=(ClassName&& other) noexcept {                     \
        if (this != &other) Move(std::move(other));                        \
        return *this;                                                      \
    }


#define DEFINE_COPY_MOVE_CONSTRUCTORS_FROM_COPY_MOVE(ClassName, COPY_BLOCK, MOVE_BLOCK)        \
    ClassName(const ClassName& other) {                                           \
        COPY_BLOCK                                                                \
    }                                                                             \
                                                                                  \
    ClassName& operator=(const ClassName& other) {                                \
        if (this != &other) { COPY_BLOCK }                                        \
        return *this;                                                             \
    }                                                                             \
                                                                                  \
    ClassName(ClassName&& other) noexcept {                                       \
        MOVE_BLOCK                                                                \
    }                                                                             \
                                                                                  \
    ClassName& operator=(ClassName&& other) noexcept {                            \
        if (this != &other) { MOVE_BLOCK }                                        \
        return *this;                                                             \
    }

#define COPY_OR_MOVE(field) \
    if constexpr (TO_COPY) field = other.field; else field = std::move(other.field);

//Be Very careful with this, becose entt move componet/copy on addEnt,createEnt,addComp,removeComp
//So on add or remove entity, can easy bug some pointers if is not copied
#define DEFINE_COPY_MOVE_CONSTRUCTORS_SHARED(ClassName, BODY)       \
    ClassName(const ClassName& other) {                             \
        constexpr bool TO_COPY = true;                                        \
        BODY                                                        \
    }                                                               \
    ClassName& operator=(const ClassName& other) {                  \
        if (this != &other) {                                       \
            constexpr bool TO_COPY = true;                                    \
            BODY                                                    \
        }                                                           \
        return *this;                                               \
    }                                                               \
    ClassName(ClassName&& other) noexcept {                         \
        constexpr bool TO_COPY = false;                                       \
        BODY                                                        \
    }                                                               \
    ClassName& operator=(ClassName&& other) noexcept {              \
        if (this != &other) {                                       \
            constexpr bool TO_COPY = false;                                   \
            BODY                                                    \
        }                                                           \
        return *this;                                               \
    }
