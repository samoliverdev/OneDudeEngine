#pragma once

#include <stdio.h>
#include <stdarg.h>
#include "pch.h"
/*#include <set>
#include <random>
#include <limits>
#include <vector>
#include <string>
#include <bitset>
#include <memory>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <functional>
#include <map>
#include <unordered_map>
#include <typeinfo>
#include <typeindex>*/

#undef NDEBUG
#include <assert.h>

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

#define OD_PROFILE 1
#define FILE_MOVE_PAYLOAD "FILE_MOVE_PAYLOAD"
#define GRAPHIC_LOG_ERROR

static const char* LogColors[] = {
    "\033[\033[0m",    //Reset Info
    "\033[0;33m",      //Yellow Warning
    "\033[0;31m",      //Red Error
    "\033[0;36m",      //Cyan Fatal
};

// logging macros
//#if defined(_DEBUG)
	#define _LOG(level, colorIndex, ...) \
        fprintf(stderr, "%s [%s] ", LogColors[colorIndex], level); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n");

    //fprintf(stderr, "\nat line number %d in file %s", __LINE__, __FILE__); \

    /*void _LOG(const char* level, int colorIndex, const char* message, ...){
        va_list args;
        va_start(args, message);
        fprintf(stderr, "%s [%s] ", LogColors[colorIndex], level);
        fprintf(stderr, message, args);
        fprintf(stderr, "\n");
        va_end(args);
    }*/
    

    #define LogInfo(...) _LOG("info", 0, __VA_ARGS__)
	#define LogWarning(...) _LOG("warning", 1, __VA_ARGS__)
	#define LogError(...) _LOG("error", 2, __VA_ARGS__)
    #define LogFatal(...) _LOG("fatal", 3, __VA_ARGS__)
	
//#else
//	#define LogWarning
//	#define LogError	
//	#define LogInfo
//#endif

// runtime assertion
#define Assert assert

// window size
#define INVALID_ID 0
#define MAX_DELTATIME 0.05f

namespace OD {

using uuid64 = size_t;
uuid64 generate_uuid();

///*
template<typename T>
using Scope = std::unique_ptr<T>;
template<typename T, typename ... Args>
constexpr Scope<T> CreateScope(Args&& ... args){
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<typename T>
using Ref = std::shared_ptr<T>;
template<typename T, typename ... Args>
constexpr Ref<T> CreateRef(Args&& ... args){
    return std::shared_ptr<T>(new T(std::forward<Args>(args)...));
}
//*/

/*
template<typename T>
using Scope = T*;
template<typename T, typename ... Args>
constexpr Scope<T> CreateScope(Args&& ... args){
    return new T(std::forward<Args>(args)...);
}

template<typename T>
using Ref = T*;
template<typename T, typename ... Args>
constexpr Ref<T> CreateRef(Args&& ... args){
    return new T(std::forward<Args>(args)...);
}
*/

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

#pragma once

#include <type_traits>

#define HAS_MEM_FUNC(func, name)                                        \
    template <typename T, typename = int> struct name : std::false_type {}; \
    template <typename T> struct name<T, decltype(&T::func, 0)> : std::true_type {};  

}
