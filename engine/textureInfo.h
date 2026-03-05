#pragma once
#include "../core.h"

enum class TextureFormat {
    RGBA8,
    RGB8,
    R32F,
    R16F,
    Depth24,
    Depth24S8,
    RG16F,
    RGB16F,
    RGBA16F,
    RGBA32F,
    R8
    // add more...
};

struct GLFormatInfo {
    GLenum internal;
    GLenum format;
    GLenum type;
};

inline static GLFormatInfo GetGLFormat(TextureFormat f) {
    switch (f) {
    case TextureFormat::RGBA8: return { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };
    case TextureFormat::RGB8:  return { GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE };
    case TextureFormat::R8: return { GL_R8, GL_RED, GL_UNSIGNED_BYTE };
    case TextureFormat::R32F:  return { GL_R32F, GL_RED, GL_FLOAT };
    case TextureFormat::Depth24: return { GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT,
        GL_UNSIGNED_INT };
    case TextureFormat::RGBA16F: return { GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT };
    case TextureFormat::Depth24S8: return { GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL,
        GL_UNSIGNED_INT_24_8 };
    case TextureFormat::RG16F: return { GL_RG16F, GL_RG, GL_HALF_FLOAT };
    case TextureFormat::RGB16F: return { GL_RGB16F, GL_RGB, GL_HALF_FLOAT };
    case TextureFormat::RGBA32F: return { GL_RGBA32F, GL_RGBA, GL_FLOAT };
    case TextureFormat::R16F: return { GL_R16F, GL_RED, GL_HALF_FLOAT };
    default: return { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };
    }
}

struct FTextureDesc {
    GLFormatInfo mGLFormatInfo;
    GLuint mHeight, mWidth;
    bool mMipmap;
    FTextureDesc(GLFormatInfo inGLFormatInfo, GLuint inWidth, GLuint inHeight, bool inMipmap)
        : mGLFormatInfo{ inGLFormatInfo }, mWidth{ inWidth }, mHeight{ inHeight }, mMipmap{ inMipmap }
    {
    }
    FTextureDesc(GLFormatInfo inGLFormatInfo, GLuint inWidth, GLuint inHeight)
        : mGLFormatInfo{ inGLFormatInfo }, mWidth{ inWidth }, mHeight{ inHeight }, mMipmap{ true }
    {
    }
};