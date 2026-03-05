#pragma once
#include "../core.h"
#include "../wrapper/checkError.h"
#include "textureInfo.h"
#define OPEN_DSA_AND_BINDLESS_TEXTURE 1

static constexpr int ImageChannel = 4;
static constexpr int NumsCubemapFaces = 6;

#if OPEN_DSA_AND_BINDLESS_TEXTURE
class FTextureSampler {
public:
	FTextureSampler();
	~FTextureSampler();

	void bind(GLuint textureUnit) const;
	GLenum& operator[](const GLenum& param);
	void setSamplerState() const;
	GLuint getHandle() const { return mHandle; }
private:
	GLuint mHandle = 0;
	std::unordered_map<GLenum, GLenum> mSamplerState = {
	{ GL_TEXTURE_MIN_FILTER, GL_LINEAR },
	{ GL_TEXTURE_MAG_FILTER, GL_LINEAR },
	{ GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE },
	{ GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE },
	{ GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE }
	};
	GLenum mErrorState = GL_NONE;
};

class FTexture {
public:
	FTexture(GLenum inTarget = GL_TEXTURE_2D);
	~FTexture();

	FTexture(const FTexture& texture) = delete;
	FTexture& operator=(const FTexture& texture) = delete;

	// only for texture 2D
	void applyResource(std::shared_ptr<FTextureDesc> inDesc);
	void allocateResource(unsigned char* data, bool isSetDefaultParam = true);
	void allocateResource(const std::string& path, bool isSetDefaultParam = true);
	template<typename T>
	void allocateResourceTemplate(T* data, bool isSetDefaultParam);

	// only for texture cube map
	void allocateResourceCubemap(std::vector<unsigned char*> data, bool isSetDefaultParam = true);
	void allocateResourceCubemap(const std::vector<std::string>& path, bool isSetDefaultParam = true);
	void allocateResourceCubemapHDR(std::vector<float*> data, bool isSetDefaultParam = true);
	void allocateResourceCubemapHDR(const std::vector<std::string>& path, bool isSetDefaultParam = true);
	
	// conditional bind/unbind
	void bind(int temporarySlot) const;
	void unbind(int temporarySlot) const;

	// bindless
	void applyBindlessHandle();
	void applyBindlessHandleWithSampler(std::shared_ptr<FTextureSampler> textureSampler);
	void setBindlessTextureResident();
	//bool getBindlessHandle(GLuint64& bindlessHandle);
	
public:
	GLuint getHandle() const { return mHandle; }
	GLuint64 getBindlessHandle() const { return mBindlessHandle; }
	GLenum getTarget() const { return mTarget; }
	const std::shared_ptr< FTextureDesc > getDesc() const { return mDesc; }
	bool getBindlessResident() const { return mBindlessResident; }
	bool getBindless() const { return mBindless; }
private:
	GLuint mHandle = 0;
	GLuint64 mBindlessHandle = 0;
	const GLenum mTarget = GL_TEXTURE_2D;
	std::shared_ptr< FTextureDesc > mDesc = nullptr;
	bool mApplication = false;
	bool mBindless = false;
	bool mBindlessResident = false;
	void setDefaultTextureParameters(bool bMipmap, bool bCubemap = false) const;
};

template<typename T>
void FTexture::allocateResourceTemplate(T* data, bool isSetDefaultParam)
{
	assert(mApplication);
	bool bMipmap = mDesc->mMipmap;
	glTextureSubImage2D(mHandle, 0, 0, 0, mDesc->mWidth, mDesc->mHeight,
		mDesc->mGLFormatInfo.format, mDesc->mGLFormatInfo.type, static_cast<void*>(data));
	if(isSetDefaultParam) setDefaultTextureParameters(bMipmap);
	if (bMipmap) glGenerateTextureMipmap(mHandle);
}

#else
class FTexture {
public:
	FTexture(GLenum inTarget = GL_TEXTURE_2D);
	~FTexture();
	
	FTexture(const FTexture& texture) = delete;
	FTexture& operator=(const FTexture& texture) = delete;

	// only for texture 2D
    void allocateResource(int temporarySlot, std::shared_ptr< FTextureDesc > inDesc,
		unsigned char* data = nullptr);
	void allocateResource(int temporarySlot, std::shared_ptr< FTextureDesc > inDesc,
		const std::string& path);
	template<typename T>
	void allocateConstTextureResource(int temporarySlot, std::shared_ptr< FTextureDesc > inDesc, T* data);

	// only for texture cube map
	void allocateResourceCubemap(int temporarySlot, std::shared_ptr< FTextureDesc > inDesc,
		std::vector<unsigned char*> data);
	void allocateResourceCubemap(int temporarySlot, std::shared_ptr< FTextureDesc > inDesc,
		const std::vector<std::string>& path);
	void allocateResourceCubemapHDR(int temporarySlot, std::shared_ptr<FTextureDesc> inDesc, std::vector<float*> data);
	void allocateResourceCubemapHDR(int temporarySlot, std::shared_ptr<FTextureDesc> inDesc, const std::vector<std::string>& path);


    void bind(int temporarySlot) const;
	void unbind(int temporarySlot) const;
public:
	GLuint getHandle() const { return mHandle; }
	GLenum getTarget() const { return mTarget; }
	const std::shared_ptr< FTextureDesc > getDesc() const { return mDesc; }
	bool getAllocated() const { return mAllocated; }
private:
	GLuint mHandle = 0;
	const GLenum mTarget = GL_TEXTURE_2D;
	std::shared_ptr< FTextureDesc > mDesc = nullptr;
	bool mAllocated = false;
};

template<typename T>
void FTexture::allocateConstTextureResource(int temporarySlot, std::shared_ptr<FTextureDesc> inDesc, T* data)
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
#endif

uint32_t CalcMaxMipLevels(uint32_t width, uint32_t height);