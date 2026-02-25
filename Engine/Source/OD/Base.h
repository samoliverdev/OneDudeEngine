#pragma once
#include "Defines.h"
#include <stdio.h>
#include <stdarg.h>
#include <memory>
#include <typeinfo>
#include <typeindex>
#include <queue> 

#include "OD/Core/Log.h"
//#include <spdlog/spdlog.h>

//#include <new>
//#include <cstddef>

//#undef NDEBUG
#include <assert.h>

// runtime assertion
//#define Assert assert

/*
#define Assert(expr)                                                        \
    do {                                                                    \
        if (!(expr)) {                                                      \
            fprintf(stderr, "Assertion failed: %s\n  File: %s\n  Line: %d\n", \
                    #expr, __FILE__, __LINE__);                             \
            abort();                                                        \
        }                                                                   \
    } while (0)
*/

#if defined(_MSC_VER)
    #define DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
    #include <signal.h>
    #define DEBUG_BREAK() raise(SIGTRAP)
#else
    #define DEBUG_BREAK() abort()
#endif

#ifndef FINAL_BUILD

//fprintf(stderr, "Assertion failed: %s, file %s, line %d\n", #expr, __FILE__, __LINE__);  
#define Assert(expr) \
    do { \
        if(!(expr)){ \
            LogFatal("Assertion failed: {}, file {}, line {}", #expr, __FILE__, __LINE__); \
            DEBUG_BREAK(); \
        } \
    } while (0)

#else

#define Assert(expr)   

#endif

//OD_API void* __cdecl operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line);
//OD_API void* __cdecl operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line);

static const char* LogColors[] = {
    "\033[\033[0m",    //Reset Info
    "\033[0;33m",      //Yellow Warning
    "\033[0;31m",      //Red Error
    "\033[0;36m",      //Cyan Fatal
};

// logging macros
//#if defined(_DEBUG)
/*#ifdef NDEBUG
    #define LogWarning
	#define LogError
	#define LogInfo
    #define LogFatal

    #define LogInfoExtra
	#define LogWarningExtra
	#define LogErrorExtra
    #define LogFatalExtra
#else*/
#if defined(__EMSCRIPTEN__)

	#define _LOG(level, colorIndex, ...) \
        printf("%s [%s] ", LogColors[colorIndex], level); \
        printf(__VA_ARGS__); \
        printf("\n");

    #define _LOG_Extra(level, colorIndex, ...) \
        printf("%s [%s] ", LogColors[colorIndex], level); \
        printf(__VA_ARGS__); \
        printf(" | %s %d", __FILE__, __LINE__); \
        printf("\n");
#else

    #define _LOG(level, colorIndex, ...) \
        fprintf(stderr, "%s [%s] ", LogColors[colorIndex], level); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n");

    #define _LOG_Extra(level, colorIndex, ...) \
        fprintf(stderr, "%s [%s] ", LogColors[colorIndex], level); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, " | %s %d", __FILE__, __LINE__); \
        fprintf(stderr, "\n");

#endif

/*
#ifndef FINAL_BUILD
    #define LogInfo(...) _LOG("info", 0, __VA_ARGS__)
	#define LogWarning(...) _LOG("warning", 1, __VA_ARGS__)
	#define LogError(...) _LOG("error", 2, __VA_ARGS__)
    #define LogFatal(...) _LOG("fatal", 3, __VA_ARGS__)

    #define LogInfoExtra(...) _LOG_Extra("info", 0, __VA_ARGS__)
	#define LogWarningExtra(...) _LOG_Extra("warning", 1, __VA_ARGS__)
	#define LogErrorExtra(...) _LOG_Extra("error", 2, __VA_ARGS__)
    #define LogFatalExtra(...) _LOG_Extra("fatal", 3, __VA_ARGS__)

    #define LogInfo(...)   ::OD::Log::GetLogger()->info(__VA_ARGS__)
    #define LogWarning(...)  ::OD::Log::GetLogger()->warn(__VA_ARGS__)
    #define LogError(...)  ::OD::Log::GetLogger()->error(__VA_ARGS__)
    #define LogFatal(...)  ::OD::Log::GetLogger()->critical(__VA_ARGS__)

    #define LogInfoExtra(...)   ::OD::Log::GetLogger()->info(__VA_ARGS__)
    #define LogWarningExtra(...)  ::OD::Log::GetLogger()->warn(__VA_ARGS__)
    #define LogErrorExtra(...)  ::OD::Log::GetLogger()->error(__VA_ARGS__)
    #define LogFatalExtra(...)  ::OD::Log::GetLogger()->critical(__VA_ARGS__)
#else
    #define LogInfo(...)
	#define LogWarning(...)
	#define LogError(...)
    #define LogFatal(...)

    #define LogInfoExtra(...)
	#define LogWarningExtra(...)
	#define LogErrorExtra(...)
    #define LogFatalExtra(...)
#endif
*/
//#endif

namespace OD {

class IdPool{
public:
    IdPool() = default;

    int Pop(){
        if (!freeIds.empty()) {
            int id = freeIds.front();
            freeIds.pop();
            return id;
        }
        return nextId++;
    }

    void Push(int id){
        assert(id < nextId && "Trying to release invalid ID");
        freeIds.push(id);
    }

    int TotalIssued() const{
        return nextId;
    }

private:
    int nextId = 0;
    std::queue<int> freeIds;
};

template<typename T>
struct ReusableVector{
    std::vector<T> data;
    size_t count = 0;

    void clear(){ count = 0; }

    void push_back(const T& v){
        if (count >= data.size()) data.push_back(v);
        else data[count] = v;
        count++;
    }

    T& operator[](size_t i){ return data[i]; }
    const T& operator[](size_t i) const { return data[i]; }

    size_t size() const { return count; }

    typename std::vector<T>::iterator begin(){ return data.begin(); }
    typename std::vector<T>::iterator end(){ return data.begin() + count; }
};

using uuid64 = size_t;
uuid64 generate_uuid();

#if 1

template<typename T> using Scope = std::unique_ptr<T>;
template<typename T, typename... Args>
Scope<T> CreateScope(Args&&... args){ 
    //return std::unique_ptr<T>(new T(std::forward<Args>(args)...)); 
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T> using WeakRef = std::weak_ptr<T>;

template<typename T> using Ref = std::shared_ptr<T>;
template<typename T, typename... Args>
Ref<T> CreateRef(Args&&... args){ 
    //return std::shared_ptr<T>(new T(std::forward<Args>(args)...)); 
    return std::make_shared<T>(std::forward<Args>(args)...);
}

#else

template<typename T> using Scope = T*;
template<typename T, typename ... Args>
constexpr Scope<T> CreateScope(Args&& ... args){ return new T(std::forward<Args>(args)...); }

template<typename T> using Ref = T*;
template<typename T, typename ... Args>
constexpr Ref<T> CreateRef(Args&& ... args){ return new T(std::forward<Args>(args)...); }

#endif

inline const uint32_t GetUniquiTypeid(){
    static uint32_t type = 1u;
    return type++;
}

template<typename T>
inline const uint32_t Typeid(){
    static const uint32_t type = GetUniquiTypeid();
    return type;
}


#if 0

using Type = uint32_t;
template<typename T>
inline Type GetType(){
    static Type type = GetUniquiTypeid();
    return type;
}

#else

using Type = std::type_index;
template<typename T>
inline Type GetType(){
    return std::type_index(typeid(T));
}

#endif

#include <type_traits>

#define HAS_MEM_FUNC(func, name)                                        \
    template <typename T, typename = int> struct name : std::false_type {}; \
    template <typename T> struct name<T, decltype(&T::func, 0)> : std::true_type {};  

/*#define HAS_TEMPLATE_FUNC(func, name)                                        \
    template <typename T, typename = int> struct name : std::false_type {}; \
    template <typename T> struct name<T, decltype(&T::template func)> : std::true_type {};*/  //Not Working
   

}
