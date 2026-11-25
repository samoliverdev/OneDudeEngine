#pragma once
#include "OD/Graphics/ColorReadback.h"
#include <algorithm>
#include <climits>

namespace OD{

inline ColorReadback ReadOpenGLColorTexture(unsigned int texture, int width, int height){
    if(width <= 0 || height <= 0 || width > INT_MAX / 4 / height || texture == 0) return {};
#ifdef GL_VERSION_3_3
    if(!glGetTexImage) return {ColorReadbackResult::Unsupported};
    ColorReadback image{ColorReadbackResult::Success, width, height};
    const int stride = width * 4;
    image.rgba8.resize(stride * height);

    GLint packBuffer;
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &packBuffer);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    const GLenum packStates[] = {GL_PACK_ALIGNMENT, GL_PACK_ROW_LENGTH, GL_PACK_SKIP_ROWS, GL_PACK_SKIP_PIXELS};
    GLint previous[4];
    for(int i = 0; i < 4; ++i){
        glGetIntegerv(packStates[i], &previous[i]);
        glPixelStorei(packStates[i], i == 0 ? 1 : 0);
    }
    GLint previousTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glBindTexture(GL_TEXTURE_2D, texture);
    GLint textureWidth = 0, textureHeight = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &textureWidth);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &textureHeight);
    const bool sizeMatches = textureWidth == width && textureHeight == height;
    if(sizeMatches) glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.rgba8.data());
    const GLenum error = glGetError();
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    for(int i = 0; i < 4; ++i) glPixelStorei(packStates[i], previous[i]);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(packBuffer));
    if(!sizeMatches || error != GL_NO_ERROR) return {};

    for(int row = 0; row < height / 2; ++row){
        auto top = image.rgba8.begin() + row * stride;
        std::swap_ranges(top, top + stride, image.rgba8.begin() + (height - 1 - row) * stride);
    }
    return image;
#else
    return {ColorReadbackResult::Unsupported};
#endif
}

}
