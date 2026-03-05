# pragma once

#include "../core.h"
#define GL_CALL(func) func; checkError();

void checkError();

inline bool checkTextureStatus(GLuint textureID) {
    if (!glIsTexture(textureID)) {
        std::cout << "纹理 " << textureID << " 不存在或已删除" << std::endl;
        return false;
    }

    GLint oldTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);

    glBindTexture(GL_TEXTURE_2D, textureID);

    GLint width, height;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

    GLenum error = glGetError();

    glBindTexture(GL_TEXTURE_2D, oldTexture);

    if (error != GL_NO_ERROR) {
        std::cout << "纹理 " << textureID << " 状态异常，错误码: " << error << std::endl;
        return false;
    }

    if (width == 0 || height == 0) {
        std::cout << "纹理 " << textureID << " 尺寸无效: " << width << "x" << height << std::endl;
        return false;
    }

    std::cout << "纹理 " << textureID << " 状态正常，尺寸: " << width << "x" << height << std::endl;
    return true;
}

inline bool checkCubemapStatus(GLuint textureID) {
    if (!glIsTexture(textureID)) {
        std::cout << "Cubemap " << textureID << " 不存在或已删除" << std::endl;
        return false;
    }

    GLint oldTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &oldTexture);

    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    bool valid = true;
    GLint width = 0, height = 0;
    GLenum errorBefore = glGetError();

    for (int i = 0; i < 6; ++i) {
        GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;

        GLint faceWidth = 0, faceHeight = 0;
        glGetTexLevelParameteriv(target, 0, GL_TEXTURE_WIDTH, &faceWidth);
        glGetTexLevelParameteriv(target, 0, GL_TEXTURE_HEIGHT, &faceHeight);

        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cout << "Cubemap 面 " << i << " 状态异常，错误码: " << error << std::endl;
            valid = false;
            continue;
        }

        if (faceWidth == 0 || faceHeight == 0) {
            std::cout << "Cubemap 面 " << i << " 尺寸无效: " << faceWidth << "x" << faceHeight << std::endl;
            valid = false;
            continue;
        }

        if (i == 0) {
            width = faceWidth;
            height = faceHeight;
        }
        else if (faceWidth != width || faceHeight != height) {
            std::cout << "Cubemap 各面尺寸不一致: 面0为 " << width << "x" << height
                << "，面" << i << "为 " << faceWidth << "x" << faceHeight << std::endl;
            valid = false;
        }
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, oldTexture);

    if (valid)
        std::cout << "Cubemap " << textureID << " 状态正常，面尺寸: " << width << "x" << height << std::endl;
    else
        std::cout << "Cubemap " << textureID << " 存在异常" << std::endl;

    return valid;
}


inline void queryBoundTextureDetails(GLenum textureUnit, GLenum textureType) {
    GLint oldTextureUnit;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &oldTextureUnit);

    glActiveTexture(textureUnit);

    GLint boundTexture = 0;
    switch (textureType) {
    case GL_TEXTURE_2D:
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);
        break;
    case GL_TEXTURE_3D:
        glGetIntegerv(GL_TEXTURE_BINDING_3D, &boundTexture);
        break;
    case GL_TEXTURE_CUBE_MAP:
        glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &boundTexture);
        break;
    }

    if (boundTexture == 0) {
        std::cout << "纹理单元 " << (textureUnit - GL_TEXTURE0)
            << " 没有绑定 " << textureType << " 纹理" << std::endl;
    }
    else {
        std::cout << "纹理单元 " << (textureUnit - GL_TEXTURE0)
            << " 绑定了纹理: " << boundTexture << std::endl;

        glBindTexture(textureType, boundTexture);

        GLint width, height, internalFormat;
        glGetTexLevelParameteriv(textureType, 0, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(textureType, 0, GL_TEXTURE_HEIGHT, &height);
        glGetTexLevelParameteriv(textureType, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);

        std::cout << "  尺寸: " << width << "x" << height << std::endl;
        std::cout << "  内部格式: " << internalFormat << std::endl;

        GLint minFilter, magFilter, wrapS, wrapT;
        glGetTexParameteriv(textureType, GL_TEXTURE_MIN_FILTER, &minFilter);
        glGetTexParameteriv(textureType, GL_TEXTURE_MAG_FILTER, &magFilter);
        glGetTexParameteriv(textureType, GL_TEXTURE_WRAP_S, &wrapS);
        glGetTexParameteriv(textureType, GL_TEXTURE_WRAP_T, &wrapT);

        std::cout << "  最小过滤: " << minFilter << std::endl;
        std::cout << "  最大过滤: " << magFilter << std::endl;
        std::cout << "  环绕模式S: " << wrapS << ", T: " << wrapT << std::endl;
    }

    glActiveTexture(oldTextureUnit);
}