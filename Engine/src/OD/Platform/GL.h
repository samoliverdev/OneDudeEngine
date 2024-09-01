#pragma once

#include <glad/glad.h>
#include <functional>

#define OPENGL_CHECK_ERRORS 1

GLenum glCheckError_(const char *file, int line, std::function<void()> callback = nullptr);

#if OPENGL_CHECK_ERRORS
#define glCheckError() glCheckError_(__FILE__, __LINE__)
#define glCheckError2(...) glCheckError_(__FILE__, __LINE__, __VA_ARGS__)
#else
#define glCheckError()
#define glCheckError2(...)
#endif