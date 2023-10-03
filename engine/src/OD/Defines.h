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

// logging macros
//#if defined(_DEBUG)
	#define _LOG(level, message, ...) \
        fprintf(stderr, "[%s] ", level); \
        fprintf(stderr, message, ##__VA_ARGS__); \
        fprintf(stderr, "\n");

	#define LogWarning(message, ...) _LOG("warning", message, ##__VA_ARGS__)
	#define LogError(message, ...) _LOG("error", message, ##__VA_ARGS__)
	#define LogInfo(message, ...) _LOG("info", message, ##__VA_ARGS__)
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
