#include "asset.h"

void pprint() {
	std::cout << "Asset module is working!" << std::endl;
}
#if OPEN_DSA_AND_BINDLESS_TEXTURE
FMaterial::FMaterial()
{
	mAlbedo = std::make_shared<FTexture>();
	mNormal = nullptr;
	mRoughness = std::make_shared<FTexture>();
	mMetallic = std::make_shared<FTexture>();
	mNeedNormal = false;
	mAlbedo->applyResource(std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false));
	mRoughness->applyResource(std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false));
	mMetallic->applyResource(std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false));
	mAlbedo->allocateResource(DefaultAlbedoPath, true);
	mRoughness->allocateResource(DefaultRoughnessPath, true);
	mMetallic->allocateResource(DefaultMetallicPath, true);
	mMatHash = hash_utils::hash_strings(
		DefaultAlbedoPath,
		DefaultRoughnessPath,
		DefaultMetallicPath,
		DefaultNormalPath
	);
}

FMaterial::FMaterial(const std::string inAlbedoPath)
{
	mAlbedo = std::make_shared<FTexture>();
	mNormal = nullptr;
	mRoughness = std::make_shared<FTexture>();
	mMetallic = std::make_shared<FTexture>();
	mNeedNormal = false;
	mAlbedo->applyResource(std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false));
	mRoughness->applyResource(std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false));
	mMetallic->applyResource(std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false));
	mAlbedo->allocateResource(inAlbedoPath, true);
	mRoughness->allocateResource(DefaultRoughnessPath, true);
	mMetallic->allocateResource(DefaultMetallicPath, true);
	mMatHash = hash_utils::hash_strings(
		inAlbedoPath,
		DefaultRoughnessPath,
		DefaultMetallicPath,
		DefaultNormalPath
	);
}

FMaterial::FMaterial(const std::string inAlbedoPath,
	const std::string inRoughnessPath,
	const std::string inMetallicPath)
{
	mAlbedo = std::make_shared<FTexture>();
	mNormal = nullptr;
	mRoughness = std::make_shared<FTexture>();
	mMetallic = std::make_shared<FTexture>();
	mNeedNormal = false;
	mAlbedo->applyResource(std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false));
	mRoughness->applyResource(std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false));
	mMetallic->applyResource(std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false));
	mAlbedo->allocateResource(inAlbedoPath, true);
	mRoughness->allocateResource(inRoughnessPath, true);
	mMetallic->allocateResource(inMetallicPath, true);
	mMatHash = hash_utils::hash_strings(
		inAlbedoPath,
		inRoughnessPath,
		inMetallicPath,
		DefaultNormalPath
	);
}

FMaterial::FMaterial(const std::string inAlbedoPath,
	const std::string inRoughnessPath,
	const std::string inMetallicPath,
	const std::string inNormalPath)
{
	mAlbedo = std::make_shared<FTexture>();
	mNormal = std::make_shared<FTexture>();
	mRoughness = std::make_shared<FTexture>();
	mMetallic = std::make_shared<FTexture>();
	mNeedNormal = true;
	mAlbedo->applyResource(std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false));
	mRoughness->applyResource(std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false));
	mMetallic->applyResource(std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false));
	mNormal->applyResource(std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false));
	mAlbedo->allocateResource(inAlbedoPath, true);
	mRoughness->allocateResource(inRoughnessPath, true);
	mMetallic->allocateResource(inMetallicPath, true);
	mNormal->allocateResource(inNormalPath, true);
	mMatHash = hash_utils::hash_strings(
		inAlbedoPath,
		inRoughnessPath,
		inMetallicPath,
		inNormalPath
	);
}
#else
FMaterial::FMaterial()
{
	mAlbedo = std::make_shared<FTexture>();
	mNormal = nullptr;
	mRoughness = std::make_shared<FTexture>();
	mMetallic = std::make_shared<FTexture>();
	mNeedNormal = false;
	mAlbedo->allocateResource(0,
		std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false),
		DefaultAlbedoPath);
	mRoughness->allocateResource(0,
		std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false),
		DefaultRoughnessPath);
	mMetallic->allocateResource(0,
		std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false),
		DefaultMetallicPath);
	mMatHash = hash_utils::hash_strings(
		DefaultAlbedoPath,
		DefaultRoughnessPath,
		DefaultMetallicPath,
		DefaultNormalPath
	);
}

FMaterial::FMaterial(const std::string inAlbedoPath)
{
	mAlbedo = std::make_shared<FTexture>();
	mNormal = nullptr;
	mRoughness = std::make_shared<FTexture>();
	mMetallic = std::make_shared<FTexture>();
	mNeedNormal = false;
	mAlbedo->allocateResource(0,
		std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false),
		inAlbedoPath);
	mRoughness->allocateResource(0,
		std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false),
		DefaultRoughnessPath);
	mMetallic->allocateResource(0,
		std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false),
		DefaultMetallicPath);
	mMatHash = hash_utils::hash_strings(
		inAlbedoPath,
		DefaultRoughnessPath,
		DefaultMetallicPath,
		DefaultNormalPath
	);
}

FMaterial::FMaterial(const std::string inAlbedoPath,
	const std::string inRoughnessPath,
	const std::string inMetallicPath)
{
	mAlbedo = std::make_shared<FTexture>();
	mNormal = nullptr;
	mRoughness = std::make_shared<FTexture>();
	mMetallic = std::make_shared<FTexture>();
	mNeedNormal = false;
	mAlbedo->allocateResource(0,
		std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false),
		inAlbedoPath);
	mRoughness->allocateResource(0,
		std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false),
		inRoughnessPath);
	mMetallic->allocateResource(0,
		std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false),
		inMetallicPath);
	mMatHash = hash_utils::hash_strings(
		inAlbedoPath,
		inRoughnessPath,
		inMetallicPath,
		DefaultNormalPath
	);
}

FMaterial::FMaterial(const std::string inAlbedoPath,
	const std::string inRoughnessPath,
	const std::string inMetallicPath,
	const std::string inNormalPath)
{
	mAlbedo = std::make_shared<FTexture>();
	mNormal = std::make_shared<FTexture>();
	mRoughness = std::make_shared<FTexture>();
	mMetallic = std::make_shared<FTexture>();
	mNeedNormal = true;
	mAlbedo->allocateResource(0,
		std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false),
		inAlbedoPath);
	mRoughness->allocateResource(0,
		std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false),
		inRoughnessPath);
	mMetallic->allocateResource(0,
		std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false),
		inMetallicPath);
	mNormal->allocateResource(0,
		std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false),
		inNormalPath);
	mMatHash = hash_utils::hash_strings(
		inAlbedoPath,
		inRoughnessPath,
		inMetallicPath,
		inNormalPath
	);
}
#endif

FMaterial::FMaterial(
	size_t inHash,
	std::shared_ptr<FTexture> inAlbedo, 
	std::shared_ptr<FTexture> inRoughness, 
	std::shared_ptr<FTexture> inMetallic, 
	std::shared_ptr<FTexture> inNormal
) : mAlbedo{ inAlbedo },
	mRoughness{ inRoughness },
	mMetallic{ inMetallic },
	mMatHash{ inHash }
{
	if(inNormal) {
		mNeedNormal = true;
		mNormal = inNormal;
	} else {
		mNeedNormal = false;
		mNormal = nullptr;
	}
}

FBindlessTextureSlot::FBindlessTextureSlot(std::shared_ptr<FMaterial> material) {
	auto getBindlessTexture = [](std::shared_ptr<FTexture> texture) -> GLuint64 {
		if (!texture) return 0;
		if (!texture->getBindless()) {
			texture->applyBindlessHandle();
			texture->setBindlessTextureResident();
		}
		if (!texture->getBindlessResident()) {
			texture->setBindlessTextureResident();
		}
		return texture->getBindlessHandle();
		};
	mAlbedoSlot = getBindlessTexture(material->getAlbedo());
	mNormalSlot = getBindlessTexture(material->getNormal());
	mRoughnessSlot = getBindlessTexture(material->getRoughness());
	mMetallicSlot = getBindlessTexture(material->getMetallic());
}

void FVertexBufferDesc::addDesc(GLuint inIndex, GLuint inSize, GLenum inDeviceType,
	bool inNormalized, const std::string& inName) {
	mDescSet.emplace_back(inIndex, inSize, inDeviceType, inNormalized, inName, mStride);
	mStride += inSize;
}
void FVertexBufferDesc::sortSet() {
	std::sort(mDescSet.begin(), mDescSet.end(),
		[](const VertexDesc& a, const VertexDesc& b) {
			return a.index < b.index;
		});
}

void VertexArrayObject::genVAO(GLuint* inHandle) { glGenVertexArrays(1, inHandle); }
void VertexArrayObject::deleteVAO(GLuint* inHandle) { glDeleteVertexArrays(1, inHandle); }
void VertexArrayObject::uploadVAO(GLenum inTarget, GLuint inBufferHandle, GLuint inVAOHandle,
	std::shared_ptr<FVertexBufferDesc> inDesc, GLuint inEBOHandle) {
	if (!inDesc) return;
	glBindVertexArray(inVAOHandle);

	glBindBuffer(inTarget, inBufferHandle);

	const std::vector<FVertexBufferDesc::VertexDesc>& descs = inDesc->mDescSet;
	int offsetElements = 0;
	for (const auto& desc : descs) {
		glEnableVertexAttribArray(desc.index);
		glVertexAttribPointer(desc.index, desc.size, desc.deviceType, desc.bNormalized,
			inDesc->mStride * static_cast<GLsizei>(sizeof(typename FVertexBufferDesc::VertexDesc::type)),
			reinterpret_cast<void*>(offsetElements * sizeof(typename FVertexBufferDesc::VertexDesc::type)));
		offsetElements += desc.size;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, inEBOHandle);
	glBindBuffer(inTarget, 0);
	glBindVertexArray(0);
}

FMeshBuffer::FMeshBuffer(GLenum inTarget)
	: mTarget{ inTarget }
{
	glGenBuffers(1, &mBufferHandle);
}

FMeshBuffer::~FMeshBuffer() { 
	glDeleteBuffers(1, &mBufferHandle); 
}

template<typename T>
void FMeshBuffer::uploadData(const std::vector<T>& inData) {
	mDataSize = inData.size() * sizeof(T);
	bind();
	glBufferData(mTarget, mDataSize, reinterpret_cast<const void*>(inData.data()), GL_STATIC_DRAW);
	unbind();
}

FMesh::~FMesh() {
	if (mVAOHandle != 0) {
		glDeleteVertexArrays(1, &mVAOHandle);
	}
}

void FMesh::uploadCompleteData(
	std::vector<float>&& inVBOData,
	std::vector<uint32_t>&& inEBOData,
	std::shared_ptr<FVertexBufferDesc> inDesc
) {
	mVertexBufferData = std::move(inVBOData);
	mElementBufferData = std::move(inEBOData);
	mVertexArrayDesc = inDesc;
	mVertexBufferObject = std::make_shared<FMeshBuffer>(GL_ARRAY_BUFFER);
	mElementBufferObject = std::make_shared<FMeshBuffer>(GL_ELEMENT_ARRAY_BUFFER);
	VertexArrayObject::genVAO(&mVAOHandle);
	mVertexBufferObject->uploadData<float>(mVertexBufferData);
	mElementBufferObject->uploadData<uint32_t>(mElementBufferData);
	VertexArrayObject::uploadVAO(mVertexBufferObject->getTarget(),
		mVertexBufferObject->getHandle(), mVAOHandle,
		mVertexArrayDesc, mElementBufferObject->getHandle());
	getGeometryCenter(mGeometryCenter);
}

void FMesh::draw() const {
	glBindVertexArray(mVAOHandle);
	glDrawElements(GL_TRIANGLES,
		static_cast<GLsizei>(mElementBufferData.size()),
		GL_UNSIGNED_INT,
		reinterpret_cast<const void*>(0));
	glBindVertexArray(0);
}

void FMesh::getGeometryCenter(glm::vec3& center) {
	for (const auto& desc : mVertexArrayDesc->mDescSet) {
		if (desc.name == "vertex") {
			size_t vertexCount = mVertexBufferData.size() / (mVertexArrayDesc->mStride);
			glm::vec3 minPos(FLT_MAX);
			glm::vec3 maxPos(-FLT_MAX);
			for (size_t i = 0; i < vertexCount; ++i) {
				size_t baseIndex = i * mVertexArrayDesc->mStride + desc.offset;
				glm::vec3 pos(
					mVertexBufferData[baseIndex + 0],
					mVertexBufferData[baseIndex + 1],
					mVertexBufferData[baseIndex + 2]
				);
				minPos = glm::min(minPos, pos);
				maxPos = glm::max(maxPos, pos);
			}
			center = (minPos + maxPos) * 0.5f;
			return;
		}
	}
}

FLight::FLight(float inHalfWidth, float inHalfHeight, glm::vec4 inRight)
	: mHalfWidth{ inHalfWidth }, mHalfHeight{ inHalfHeight }, mRight{ inRight }
{

}

FLight::FLight(float inHalfWidth, float inHalfHeight)
	: mHalfWidth{ inHalfWidth }, mHalfHeight{ inHalfHeight }
{

}

glm::mat4 createSimpleModelMatrix(const glm::vec3& translation,
	const glm::vec3& rotation,
	const glm::vec3& scale)
{
	glm::mat4 model = glm::mat4(1.0f);

	// 平移
	model = glm::translate(model, translation);

	// 旋转 - 使用常见的ZYX顺序
	model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));

	// 缩放
	model = glm::scale(model, scale);

	return model;
}

void getLightMatrix(std::shared_ptr<FLight> inLight, float nearBias, float far, float& near,
	glm::mat4& lightViewMatrix, glm::mat4& lightProjMatrix, glm::vec4& refineLightPosition,
	glm::vec4& lightAngleInfo) {
	//const float halfTheta = PI / 3.0f;
	const float halfTheta = PI / 4.0f;
	float invTanHalfTheta = 1 / tan(halfTheta);
	float length = std::max(inLight->mHalfHeight, inLight->mHalfWidth);
	lightAngleInfo = glm::vec4(tan(halfTheta), invTanHalfTheta, length / 10.0f, 0.0f);
	
	float backDist = length * invTanHalfTheta;
	near = backDist + nearBias;
	refineLightPosition = inLight->mPosition - inLight->mDirection * backDist;

	//refineLightPosition = inLight->mPosition;
	//near = 0.01f;
	glm::vec3 f = glm::normalize(glm::vec3(inLight->mDirection));
	glm::vec3 r = glm::normalize(glm::vec3(inLight->mRight));
	glm::vec3 u = glm::normalize(glm::cross(f, r));

	glm::vec3 p = glm::vec3(refineLightPosition);

	glm::mat3 R = glm::mat3(r, u, f);
	glm::mat3 R_T = glm::transpose(R);
	glm::vec4 minusR_TMulTrans = glm::vec4(-R_T * p, 1.0f);

	lightViewMatrix = glm::mat4(R_T);
	lightViewMatrix[3] = minusR_TMulTrans;
	
	lightProjMatrix = glm::mat4(
		glm::vec4(invTanHalfTheta, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, invTanHalfTheta, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, - near / (far - near), 1.0f),
		glm::vec4(0.0f, 0.0f, far * near / (far - near), 0.0f)
	);
}

FShadowTerm::FShadowTerm(std::shared_ptr<FLight> inLight, float nearBias, float far) 
	: mNearBias{nearBias}, mFar{far} {
	getLightMatrix(inLight, nearBias, far, mNear, lightViewMatrix, lightProjMatrix, refineLightPosition, lightAngleInfo);
}