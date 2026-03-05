#include "buffer.h"

void FUniformBuffer::initBuffer(size_t size, GLuint bindSlot, GLenum usage) {
    mBindSlot = bindSlot;
    mBufferSize = size;
    bind();
    glBufferData(GL_UNIFORM_BUFFER, mBufferSize, nullptr, usage);
    glBindBufferBase(GL_UNIFORM_BUFFER, mBindSlot, mHandle);
    unbind();
}

void FUniformBuffer::bindToProgram(GLuint programHandle) const {
    GLuint blockIndex = glGetUniformBlockIndex(programHandle, mName.c_str());
    if (blockIndex == GL_INVALID_INDEX) {
        std::cerr << "no block name match: " << mName << std::endl;
        return;
    }
    glUniformBlockBinding(programHandle, blockIndex, mBindSlot);
}

void FShaderStorageBuffer::initBuffer(size_t size, GLuint bindSlot, GLenum usage) {
    mBindSlot = bindSlot;
    mBufferSize = size;
    bind();
    glBufferData(GL_SHADER_STORAGE_BUFFER, mBufferSize, nullptr, usage);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mBindSlot, mHandle);
    unbind();
}
