#pragma once

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

#define INVALID_ID 0
#define MAX_DELTATIME 0.05f
