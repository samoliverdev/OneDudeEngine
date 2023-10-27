#pragma once

#include <glad/glad.h>
#include <functional>

GLenum glCheckError_(const char *file, int line, std::function<void()> callback = nullptr);

#define glCheckError() glCheckError_(__FILE__, __LINE__)
#define glCheckError2(...) glCheckError_(__FILE__, __LINE__, __VA_ARGS__)