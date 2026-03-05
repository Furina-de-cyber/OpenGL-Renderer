#pragma once

#include "../core.h"

class FUniformBuffer {
public:
    explicit FUniformBuffer(const std::string& inName)
        : mName{ inName }
    {
        glGenBuffers(1, &mHandle);
    }

    ~FUniformBuffer() {
        glDeleteBuffers(1, &mHandle);
    }

    void bind() const { glBindBuffer(GL_UNIFORM_BUFFER, mHandle); }
    void unbind() const { glBindBuffer(GL_UNIFORM_BUFFER, 0); }
    void bindBufferBase() const { glBindBufferBase(GL_UNIFORM_BUFFER, mBindSlot, mHandle); }

    void initBuffer(size_t size, GLuint bindSlot, GLenum usage = GL_DYNAMIC_DRAW);

    template<typename StructType>
    void setData(const StructType& inData);

    template<typename StructType>
    void updateData(const StructType& inData, GLintptr offsetBytes, bool needBind = true);

    void bindToProgram(GLuint programHandle) const;

private:
    GLuint mHandle = 0;          // UBO的OpenGL句柄
    const std::string mName;     // GLSL中的block名字
    GLuint mBindSlot = UINT_MAX; // 绑定槽位，默认为未初始化
    size_t mBufferSize = 0;      // UBO的大小
};

template<typename StructType>
void FUniformBuffer::setData(const StructType& inData) {
    if (mBindSlot == UINT_MAX) {
        std::cerr << "UBO " << mName <<" has not been initialized with a bind slot" << std::endl;
        return;
    }
    if (sizeof(inData) > mBufferSize) {
        std::cerr << "data larger than buffer size" << std::endl;
        return;
    }
    bind();
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(StructType), &inData);
    unbind();
}

template<typename StructType>
void FUniformBuffer::updateData(const StructType& inData, GLintptr offsetBytes, bool needBind) {
    if (mBindSlot == UINT_MAX) {
        std::cerr << "UBO " << mName << " has not been initialized with a bind slot" << std::endl;
        return;
    }
    if (offsetBytes + sizeof(StructType) > mBufferSize) {
        std::cerr << "space overflow" << std::endl;
        return;
    }
    if(needBind) bind();
    glBufferSubData(GL_UNIFORM_BUFFER, offsetBytes, sizeof(StructType), &inData);
    if (needBind) unbind();
}

class FShaderStorageBuffer {
public:
    explicit FShaderStorageBuffer(const std::string& inName)
        : mName{ inName }
    {
        glGenBuffers(1, &mHandle);
    }

    ~FShaderStorageBuffer() {
        glDeleteBuffers(1, &mHandle);
    }

    void bind() const { glBindBuffer(GL_SHADER_STORAGE_BUFFER, mHandle); }
    void unbind() const { glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); }
    void bindBufferBase() const { glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mBindSlot, mHandle); }

    void initBuffer(size_t size, GLuint bindSlot, GLenum usage = GL_DYNAMIC_DRAW);

    template<typename StructType>
    void setData(const StructType& inData);

    template<typename StructType>
    void setData(StructType* inData, uint32_t dataLength);

    template<typename StructType>
    void updateData(const StructType& inData, GLintptr offsetBytes, bool needBind = true);

private:
    GLuint mHandle = 0;          // SSBO的OpenGL句柄
    const std::string mName;     // GLSL中的block名字
    GLuint mBindSlot = UINT_MAX; // 绑定槽位，默认为未初始化
    size_t mBufferSize = 0;      // SSBO的大小
};

template<typename StructType>
void FShaderStorageBuffer::setData(const StructType& inData) {
    if (mBindSlot == UINT_MAX) {
        std::cerr << "SSBO " << mName << " has not been initialized with a bind slot" << std::endl;
        return;
    }
    if (sizeof(inData) > mBufferSize) {
        std::cerr << "data larger than buffer size" << std::endl;
        return;
    }
    bind();
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(StructType), &inData);
    unbind();
}

template<typename StructType>
void FShaderStorageBuffer::setData(StructType* inData, uint32_t dataLength) {
    if (mBindSlot == UINT_MAX) {
        std::cerr << "SSBO " << mName << " has not been initialized with a bind slot" << std::endl;
        return;
    }
    if (sizeof(StructType) * dataLength > mBufferSize) {
        std::cerr << "data larger than buffer size" << std::endl;
        return;
    }
    bind();
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(StructType) * dataLength, inData);
    unbind();
}

template<typename StructType>
void FShaderStorageBuffer::updateData(const StructType& inData, GLintptr offsetBytes, bool needBind) {
    if (mBindSlot == UINT_MAX) {
        std::cerr << "SSBO " << mName << " has not been initialized with a bind slot" << std::endl;
        return;
    }
    if (offsetBytes + sizeof(StructType) > mBufferSize) {
        std::cerr << "space overflow" << std::endl;
        return;
    }
    if (needBind) bind();
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offsetBytes, sizeof(StructType), &inData);
    if (needBind) unbind();
}