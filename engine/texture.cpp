#include "texture.h"

uint32_t CalcMaxMipLevels(uint32_t width, uint32_t height)
{
    assert(width > 0 && height > 0);

    uint32_t maxDim = std::max(width, height);

    uint32_t levels = 1;
    while (maxDim > 1)
    {
        maxDim >>= 1;
        ++levels;
    }

    return levels;
}

#if OPEN_DSA_AND_BINDLESS_TEXTURE
FTexture::FTexture(GLenum inTarget)
    : mTarget{ inTarget }, mApplication{ false }
{
    glCreateTextures(inTarget, 1, &mHandle);
}

FTexture::~FTexture() {
    if (mHandle != 0) {
        glDeleteTextures(1, &mHandle);
    }
}

void FTexture::bind(int temporarySlot) const {
    glActiveTexture(GL_TEXTURE0 + temporarySlot);
    glBindTexture(mTarget, mHandle);
}
void FTexture::unbind(int temporarySlot) const {
    glActiveTexture(GL_TEXTURE0 + temporarySlot);
    glBindTexture(mTarget, 0);
}

void FTexture::applyResource(std::shared_ptr<FTextureDesc> inDesc) {
    assert(!mApplication);
    mDesc = inDesc;
	uint32_t mipLevels = mDesc->mMipmap ? CalcMaxMipLevels(mDesc->mWidth, mDesc->mHeight) : 1;
	glTextureStorage2D(mHandle, mipLevels, mDesc->mGLFormatInfo.internal, mDesc->mWidth, mDesc->mHeight);
    mApplication = true;
    //checkTextureStatus(mHandle);
}

void FTexture::setDefaultTextureParameters(bool bMipmap, bool bCubemap) const {
    GLenum minFilter = bMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if(bCubemap) glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void FTexture::allocateResource(unsigned char* data, bool isSetDefaultParam)
{
    assert(mApplication && data != nullptr);
    bool bMipmap = mDesc->mMipmap;
    glTextureSubImage2D(mHandle, 0, 0, 0, mDesc->mWidth, mDesc->mHeight,
        mDesc->mGLFormatInfo.format, mDesc->mGLFormatInfo.type, static_cast<void*>(data));       
    if (bMipmap) glGenerateTextureMipmap(mHandle);
    if (isSetDefaultParam) setDefaultTextureParameters(bMipmap);  
}

void FTexture::allocateResource(const std::string& path, bool isSetDefaultParam) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, ImageChannel);
    mDesc->mHeight = height;
    mDesc->mWidth = width;
    allocateResource(data, isSetDefaultParam);
    stbi_image_free(data);
}

void FTexture::allocateResourceCubemap(std::vector<unsigned char*> data, bool isSetDefaultParam) {
    assert(mApplication);
    size_t length = data.size();
    if (length != NumsCubemapFaces || mDesc->mWidth != mDesc->mHeight) {
        throw std::runtime_error("num of the resource of cubemap must be equal to 6 and width must be equal to height");
        return;
    }
    for (int i = 0; i < NumsCubemapFaces; i++) {
        glTextureSubImage2D(mHandle, i, 0, 0, mDesc->mWidth, mDesc->mHeight,
            mDesc->mGLFormatInfo.format, mDesc->mGLFormatInfo.type, data[i]);
    }
    bool bMipmap = mDesc->mMipmap;
    if (isSetDefaultParam) setDefaultTextureParameters(bMipmap, true);
    if (bMipmap) glGenerateTextureMipmap(mHandle);
}

void FTexture::allocateResourceCubemap(const std::vector<std::string>& path, bool isSetDefaultParam) {
    size_t length = path.size();
    if (length != NumsCubemapFaces) {
        throw std::runtime_error("num of the resource of cubemap must be equal to 6");
        return;
    }
    int width, height, nrChannels;
    std::vector<unsigned char*> data(6);
    data[0] = stbi_load(path[0].c_str(), &width, &height, &nrChannels, ImageChannel);
    mDesc->mHeight = height;
    mDesc->mWidth = width;
    for (int i = 1; i < NumsCubemapFaces; i++) {
        data[i] = stbi_load(path[i].c_str(), &width, &height, &nrChannels, ImageChannel);
    }
    allocateResourceCubemap(data, isSetDefaultParam);
    for (int i = 0; i < NumsCubemapFaces; i++) {
        stbi_image_free(data[i]);
    }
}

void FTexture::allocateResourceCubemapHDR(std::vector<float*> data, bool isSetDefaultParam) {
    assert(mApplication);
    size_t length = data.size();
    if (length != NumsCubemapFaces || mDesc->mWidth != mDesc->mHeight) {
        throw std::runtime_error("num of the resource of cubemap must be equal to 6 and width must be equal to height");
        return;
    }

    for (int i = 0; i < NumsCubemapFaces; i++) {
        glTextureSubImage2D(mHandle, i, 0, 0, mDesc->mWidth, mDesc->mHeight,
            mDesc->mGLFormatInfo.format, mDesc->mGLFormatInfo.type, data[i]);
    }

    bool bMipmap = mDesc->mMipmap;
    if (isSetDefaultParam)setDefaultTextureParameters(bMipmap, true);
    if (bMipmap) glGenerateTextureMipmap(mHandle);
}

void FTexture::allocateResourceCubemapHDR(const std::vector<std::string>& path, bool isSetDefaultParam) {
    size_t length = path.size();
    if (length != NumsCubemapFaces) {
        throw std::runtime_error("num of the resource of cubemap must be equal to 6");
        return;
    }

    int width = 0, height = 0, nrChannels = 0;
    std::vector<float*> data(NumsCubemapFaces);

    data[0] = stbi_loadf(path[0].c_str(), &width, &height, &nrChannels, ImageChannel);
    if (!data[0]) {
        throw std::runtime_error("Failed to load HDR image: " + path[0]);
    }
    mDesc->mHeight = height;
    mDesc->mWidth = width;

    for (int i = 1; i < NumsCubemapFaces; i++) {
        data[i] = stbi_loadf(path[i].c_str(), &width, &height, &nrChannels, ImageChannel);
        if (!data[i]) {
            for (int j = 0; j < i; j++) stbi_image_free(data[j]);
            throw std::runtime_error("Failed to load HDR image: " + path[i]);
        }
    }
    allocateResourceCubemapHDR(data, isSetDefaultParam);
    for (int i = 0; i < NumsCubemapFaces; i++) {
        stbi_image_free(data[i]);
    }
}

void FTexture::applyBindlessHandle() {
    mBindlessHandle = glGetTextureHandleARB(mHandle);
	mBindless = true;
}

void FTexture::applyBindlessHandleWithSampler(std::shared_ptr<FTextureSampler> textureSampler) {
    mBindlessHandle = glGetTextureSamplerHandleARB(mHandle, textureSampler->getHandle());
    mBindless = true;
}

//bool FTexture::getBindlessHandle(GLuint64& bindlessHandle)
//{
//    if (mBindlessResident) {
//		bindlessHandle = mBindlessHandle;
//		return true;
//    }
//    return false;
//}

void FTexture::setBindlessTextureResident(){
    if (!mBindless) {
		std::cerr << "Warning: Attempting to set a non-bindless texture as resident. This operation will be ignored." << std::endl;
    }
	glMakeTextureHandleResidentARB(mBindlessHandle);
	mBindlessResident = true;
}

FTextureSampler::FTextureSampler() {
    glCreateSamplers(1, &mHandle);
}

FTextureSampler::~FTextureSampler() {
    if (mHandle != 0) {
        glDeleteSamplers(1, &mHandle);
    }
}

GLenum& FTextureSampler::operator[](const GLenum& param) {
    if (!mSamplerState.contains(param)) {
		std::cerr << "Warning: Sampler parameter " << param << " is not recognized. Returning reference to a default value." << std::endl;
        return mErrorState;
    }
    return mSamplerState[param];
}

void FTextureSampler::setSamplerState() const {
	for (const auto& [param, value] : mSamplerState) {
        glSamplerParameteri(mHandle, param, value);
    }
}

void FTextureSampler::bind(GLuint unit) const {
    glBindSampler(unit, mHandle);
}

#else
FTexture::FTexture(GLenum inTarget) 
    : mTarget{ inTarget }
{
	glGenTextures(1, &mHandle);
}

FTexture::~FTexture() {
	if (mHandle != 0) {
		glDeleteTextures(1, &mHandle);
	}
}

void FTexture::allocateResourceCubemapHDR(
    int temporarySlot,
    std::shared_ptr<FTextureDesc> inDesc,
    std::vector<float*> data
) {
    mDesc = inDesc;
    bind(temporarySlot);

    size_t length = data.size();
    if (length != NumsCubemapFaces || mDesc->mWidth != mDesc->mHeight) {
        throw std::runtime_error("num of the resource of cubemap must be equal to 6 and width must be equal to height");
        return;
    }

    for (int i = 0; i < NumsCubemapFaces; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
            mDesc->mGLFormatInfo.internal, mDesc->mWidth, mDesc->mHeight,
            0, mDesc->mGLFormatInfo.format, mDesc->mGLFormatInfo.type, static_cast<void*>(data[i]));
    }
    
    bool bMipmap = mDesc->mMipmap;
    GLenum minFilter = bMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    if (bMipmap) glGenerateMipmap(mTarget);

    unbind(temporarySlot);
    mAllocated = true;
}

void FTexture::allocateResourceCubemapHDR(
    int temporarySlot,
    std::shared_ptr<FTextureDesc> inDesc,
    const std::vector<std::string>& path
) {
    size_t length = path.size();
    if (length != NumsCubemapFaces) {
        throw std::runtime_error("num of the resource of cubemap must be equal to 6");
        return;
    }

    int width = 0, height = 0, nrChannels = 0;
    std::vector<float*> data(NumsCubemapFaces);

    data[0] = stbi_loadf(path[0].c_str(), &width, &height, &nrChannels, ImageChannel);
    if (!data[0]) {
        throw std::runtime_error("Failed to load HDR image: " + path[0]);
    }
    inDesc->mHeight = height;
    inDesc->mWidth = width;

    for (int i = 1; i < NumsCubemapFaces; i++) {
        data[i] = stbi_loadf(path[i].c_str(), &width, &height, &nrChannels, ImageChannel);
        if (!data[i]) {
            for (int j = 0; j < i; j++) stbi_image_free(data[j]);
            throw std::runtime_error("Failed to load HDR image: " + path[i]);
        }
    }
    allocateResourceCubemapHDR(temporarySlot, inDesc, data);
    for (int i = 0; i < NumsCubemapFaces; i++) {
        stbi_image_free(data[i]);
    }
}


void FTexture::allocateResourceCubemap(
    int temporarySlot, 
    std::shared_ptr<FTextureDesc> inDesc, 
    std::vector<unsigned char*> data
) {
    mDesc = inDesc;
    bind(temporarySlot);
    size_t length = data.size();
    if (length != NumsCubemapFaces || mDesc->mWidth != mDesc->mHeight) {
        throw std::runtime_error("num of the resource of cubemap must be equal to 6 and width must be equal to height");
        return;
    }
    for (int i = 0; i < NumsCubemapFaces; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
            mDesc->mGLFormatInfo.internal, mDesc->mWidth, mDesc->mHeight,
            0, mDesc->mGLFormatInfo.format, mDesc->mGLFormatInfo.type, static_cast<void*>(data[i]));
    }
    bool bMipmap = mDesc->mMipmap;
    GLenum minFilter = bMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    if (bMipmap) glGenerateMipmap(mTarget);
    unbind(temporarySlot);
    mAllocated = true;
}

void FTexture::allocateResourceCubemap(
    int temporarySlot, 
    std::shared_ptr<FTextureDesc> inDesc, 
    const std::vector<std::string>& path
) {
    size_t length = path.size();
    if (length != NumsCubemapFaces) {
        throw std::runtime_error("num of the resource of cubemap must be equal to 6");
        return;
    }
    int width, height, nrChannels;
    std::vector<unsigned char*> data(6);
    data[0] = stbi_load(path[0].c_str(), &width, &height, &nrChannels, ImageChannel);
    inDesc->mHeight = height;
    inDesc->mWidth = width;
    for (int i = 1; i < NumsCubemapFaces; i++) {
        data[i] = stbi_load(path[i].c_str(), &width, &height, &nrChannels, ImageChannel);
    }
    allocateResourceCubemap(temporarySlot, inDesc, data);
    for (int i = 0; i < NumsCubemapFaces; i++) {
        stbi_image_free(data[i]);
    }
}

void FTexture::bind(int temporarySlot) const {
    glActiveTexture(GL_TEXTURE0 + temporarySlot);
    glBindTexture(mTarget, mHandle);
}
void FTexture::unbind(int temporarySlot) const {
    glActiveTexture(GL_TEXTURE0 + temporarySlot);
    glBindTexture(mTarget, 0);
}

void FTexture::allocateResource(int temporarySlot, 
    std::shared_ptr< FTextureDesc > inDesc, 
    unsigned char* data)
{
    mDesc = inDesc;
    bool bMipmap = mDesc->mMipmap;
    bind(temporarySlot);
    glTexImage2D(GL_TEXTURE_2D, 0, mDesc->mGLFormatInfo.internal,
        mDesc->mWidth, mDesc->mHeight, 0, mDesc->mGLFormatInfo.format,
        mDesc->mGLFormatInfo.type, static_cast<void*>(data));
    GLenum minFilter = bMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (bMipmap) glGenerateMipmap(mTarget);
    unbind(temporarySlot);
    mAllocated = true;
}

void FTexture::allocateResource(int temporarySlot, std::shared_ptr< FTextureDesc > inDesc,
    const std::string& path) {
	int width, height, nrChannels;

    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, ImageChannel);
    inDesc->mHeight = height;
	inDesc->mWidth = width;
    allocateResource(temporarySlot, inDesc, data);
	stbi_image_free(data);
}
#endif