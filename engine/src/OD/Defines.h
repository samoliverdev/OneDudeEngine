#pragma once

#include <stdio.h>
#include <set>
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
#include <typeindex>

#undef NDEBUG
#include <assert.h>

#define OD_PROFILE 1

#define FILE_MOVE_PAYLOAD "FILE_MOVE_PAYLOAD"

static const char* LogColors[] = {
    "\033[\033[0m",    //Reset Info
    "\033[0;33m",      //Yellow Warning
    "\033[0;31m",      //Red Error
    "\033[0;36m",      //Cyan Fatal
};

// logging macros
//#if defined(_DEBUG)
	#define _LOG(level, colorIndex, message , ...) \
        fprintf(stderr, "%s [%s] ", LogColors[colorIndex], level); \
        fprintf(stderr, message, __VA_ARGS__); \
        fprintf(stderr, "\n");

    #define LogInfo(message, ...) _LOG("info", 0, message, __VA_ARGS__)
	#define LogWarning(message, ...) _LOG("warning", 1, message, __VA_ARGS__)
	#define LogError(message, ...) _LOG("error", 2, message,__VA_ARGS__)
    #define LogFatal(message, ...) _LOG("fatal", 3, message,__VA_ARGS__)
	
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


#if 1

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

}
