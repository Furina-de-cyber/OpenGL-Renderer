#include "renderer.h"

FFrameBuffer::FFrameBuffer()
{
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &mMaxColorAttachments);
	glGenFramebuffers(1, &mFBOHandle);
	mCurrentTarget = GL_FRAMEBUFFER;
}

FFrameBuffer::~FFrameBuffer() {
	glDeleteFramebuffers(1, &mFBOHandle);
}

void FFrameBuffer::bindFBO(GLenum target)
{
	mCurrentTarget = target;
	glBindFramebuffer(target, mFBOHandle);
}

void FFrameBuffer::unbindFBO() {
	if (mCurrentTarget == GL_NONE) return;
	glBindFramebuffer(mCurrentTarget, 0);
}

void FFrameBuffer::setTarget(GLenum inTarget) { mCurrentTarget = inTarget; }

bool FFrameBuffer::attach(ETextureUsage inUsage, std::shared_ptr<FTexture> inTexture, int attachmentBias) {
	if (!inTexture) {
		std::cerr << "attachTextureToFBO: texture null " << std::endl;
		return false;
	}
	bool weBound = false;
	GLenum attachment = GL_NONE;
	if (mCurrentTarget == GL_NONE) {
		bindFBO(GL_FRAMEBUFFER);
		weBound = true;
	}

	switch (inUsage)
	{
	case ETextureUsage::RenderTarget: {
		GLuint idx = static_cast<GLuint>(attachmentBias);
		if (idx >= static_cast<GLuint>(mMaxColorAttachments)) {
			if (weBound) unbindFBO();
			return false;
		}
		attachment = GL_COLOR_ATTACHMENT0 + idx;
		glFramebufferTexture2D(mCurrentTarget, attachment, GL_TEXTURE_2D, inTexture->getHandle(), 0);
		mBindList.push_back(attachment);
	}
									break;
	case ETextureUsage::DepthStencil: {
		attachment = GL_DEPTH_STENCIL_ATTACHMENT;
		glFramebufferTexture2D(mCurrentTarget, attachment, GL_TEXTURE_2D, inTexture->getHandle(), 0);
	}
									break;
	default:
		if (weBound) unbindFBO();
		return false;
	}

	if (glCheckFramebufferStatus(mCurrentTarget) != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "Framebuffer incomplete after attaching " << std::endl;
		if (weBound) unbindFBO();
		return false;
	}
	if (weBound) unbindFBO();
	mTexture.push_back(inTexture);
	return true;
}

void FFrameBuffer::uploadColorAttachment() {
	glDrawBuffers(static_cast<GLsizei>(mBindList.size()), mBindList.data());
}

FRenderer::~FRenderer() {

}

void FRenderer::setRenderState() {
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);
	glStencilMask(0xFF);
	glClearStencil(0);
}

void FRenderer::addShader(const std::string& name, std::shared_ptr<FShader> inShader) {
	if (!mShaders.contains(name)) {
		mShaders[name] = inShader;
	}
}


std::shared_ptr<FTexture> FRenderer::getTexture(const std::string& name, ETextureUsage inUsage) {
	switch (inUsage) {
		case ETextureUsage::ShaderResource:
			if (mShaderResource.contains(name)) {
				return mShaderResource[name];
			}
			break;
		case ETextureUsage::RenderTarget:
			if (mRenderTarget.contains(name)) {
				return mRenderTarget[name];
			}
			break;
		case ETextureUsage::DepthStencil:
			if (mDepthStencil.contains(name)) {
				return mDepthStencil[name];
			}
			break;
		case ETextureUsage::UnorderedAccess:
			if (mUnordererAccess.contains(name)) {
				return mUnordererAccess[name];
			}
			break;
		default:
			return nullptr;
	}
	return nullptr;
}

void FRenderer::addTexture(const std::string& name, ETextureUsage inUsage,
	std::shared_ptr<FTexture> inTexture) {
	switch (inUsage)
	{
		case ETextureUsage::ShaderResource:
			mShaderResource[name] = inTexture;
			break;
		case ETextureUsage::RenderTarget:
			mRenderTarget[name] = inTexture;
			break;
		case ETextureUsage::DepthStencil:
			mDepthStencil[name] = inTexture;
			break;
		case ETextureUsage::UnorderedAccess:
			mUnordererAccess[name] = inTexture;
			break;
		default:
			break;
	}
}

void FRenderer::addSSBO(const std::string& name, std::shared_ptr<FShaderStorageBuffer> inSSBO) {
	mShaderStorageBuffer[name] = inSSBO;
}

std::shared_ptr<FShaderStorageBuffer> FRenderer::getSSBO(const std::string& name) {
	return mShaderStorageBuffer[name];
}

void FRenderer::setViewPort(const int width, const int height) {
	mWindowWidth = width;
	mWindowHeight = height;
}

void FGBufferRenderer::setRenderState() {
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glClearDepth(1.0f);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	//glFrontFace(GL_CCW);
	glDisable(GL_STENCIL_TEST);
	glStencilMask(0xFF);
	glClearStencil(0);
}

#if OPEN_DSA_AND_BINDLESS_TEXTURE
void FGBufferRenderer::initialize() {
	mFrameBuffer = std::make_shared<FFrameBuffer>();

	std::shared_ptr<FTexture> albedo = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> albedoDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::RGBA8), mWindowWidth, mWindowHeight);
	albedo->applyResource(albedoDesc);
	addTexture("GBuffer_Albedo", ETextureUsage::RenderTarget, albedo);
	std::shared_ptr<FTexture> roughnessMetallicLinearDepth = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> rmldDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::RGBA16F), mWindowWidth, mWindowHeight, false);
	roughnessMetallicLinearDepth->applyResource(rmldDesc);
	addTexture("GBuffer_RoughnessMetallicLinearDepth", ETextureUsage::RenderTarget,
		roughnessMetallicLinearDepth);

	std::shared_ptr<FTexture> normal = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> normalDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::RGBA16F), mWindowWidth, mWindowHeight, false
	);
	normal->applyResource(normalDesc);
	addTexture("GBuffer_Normal", ETextureUsage::RenderTarget, normal);

	std::shared_ptr<FTexture> deviceZ = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> deviceZDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::R16F), mWindowWidth, mWindowHeight, false
	);
	deviceZ->applyResource(deviceZDesc);
	addTexture("GBuffer_DeviceZ", ETextureUsage::RenderTarget, deviceZ);

	std::shared_ptr<FTexture> depth = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> depthDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::Depth24S8), mWindowWidth, mWindowHeight);
	depth->applyResource(depthDesc);

	addTexture("GBuffer_Depth", ETextureUsage::DepthStencil, depth);
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, albedo, 0);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, roughnessMetallicLinearDepth, 1);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, normal, 2);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, deviceZ, 3);
	mFrameBuffer->attach(ETextureUsage::DepthStencil, depth);
	mFrameBuffer->unbindFBO();
	mCameraUBO = mScene->mCameraUBO;
	mCameraUBO->bindToProgram(mShaders["GBufferShader"]->getProgramHandle());
}

void FGBufferRenderer::render() {
	if (!mScene) return;
	auto shader = mShaders["GBufferShader"];
	shader->use();
	setRenderState();
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->uploadColorAttachment();
	glViewport(0, 0, mWindowWidth, mWindowHeight);
	glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	int slot = 0;
	//FCameraInfo cameraUBOdata = getCameraInfo(mScene->mCamera);
	//mCameraUBO->updateData(cameraUBOdata, 0);
	mCameraUBO->bind();
	shader->uniform1f(mScene->mCameraFar, "far");
	shader->uniform1f(mScene->mCameraNear, "near");
	for (const auto& asset : mScene->mAsset) {
		for (const auto& pair : asset->mMesh) {
			auto mesh = pair.first;
			size_t materialIndex = pair.second;
			auto material = asset->mMaterial[materialIndex];

			slot = material->MaterialSlotMap.at("albedo");
			glBindTextureUnit(slot, material->getAlbedo()->getHandle());
			shader->uniform1i(slot, material->MaterialNameMap.at("albedo").c_str());

			slot = material->MaterialSlotMap.at("roughness");
			glBindTextureUnit(slot, material->getRoughness()->getHandle());
			shader->uniform1i(slot, material->MaterialNameMap.at("roughness").c_str());

			slot = material->MaterialSlotMap.at("metallic");
			glBindTextureUnit(slot, material->getMetallic()->getHandle());
			shader->uniform1i(slot, material->MaterialNameMap.at("metallic").c_str());

			glm::mat4 modelMatrix = asset->getModelMatrix() * mesh->getModelMatrix();
			glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
			shader->uniform4x4f(modelMatrix, "modelMatrix");
			shader->uniform3x3f(normalMatrix, "normalMatrix");
			mesh->draw();
		}
	}
	mFrameBuffer->unbindFBO();
	mCameraUBO->unbind();
	shader->end();
}
#else
void FGBufferRenderer::initialize() {
	mFrameBuffer = std::make_shared<FFrameBuffer>();

	std::shared_ptr<FTexture> albedo = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> albedoDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::RGBA8), mWindowWidth, mWindowHeight, false);
	albedo->allocateResource(0, albedoDesc);
	addTexture("GBuffer_Albedo", ETextureUsage::RenderTarget, albedo);

	std::shared_ptr<FTexture> roughnessMetallicLinearDepth = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> rmldDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::RGBA16F), mWindowWidth, mWindowHeight, false);
	roughnessMetallicLinearDepth->allocateResource(0, rmldDesc);
	addTexture("GBuffer_RoughnessMetallicLinearDepth", ETextureUsage::RenderTarget,
		roughnessMetallicLinearDepth);

	std::shared_ptr<FTexture> normal = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> normalDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::RGBA16F), mWindowWidth, mWindowHeight, false
	);
	normal->allocateResource(0, normalDesc);
	addTexture("GBuffer_Normal", ETextureUsage::RenderTarget, normal);

	std::shared_ptr<FTexture> deviceZ = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> deviceZDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::R16F), mWindowWidth, mWindowHeight, true
	);
	deviceZ->allocateResource(0, deviceZDesc);
	addTexture("GBuffer_DeviceZ", ETextureUsage::RenderTarget, deviceZ);

	std::shared_ptr<FTexture> depth = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> depthDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::Depth24S8), mWindowWidth, mWindowHeight, false);
	depth->allocateResource(0, depthDesc);
	addTexture("GBuffer_Depth", ETextureUsage::DepthStencil, depth);
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, albedo, 0);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, roughnessMetallicLinearDepth, 1);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, normal, 2);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, deviceZ, 3);
	mFrameBuffer->attach(ETextureUsage::DepthStencil, depth);
	mFrameBuffer->unbindFBO();
	mCameraUBO = mScene->mCameraUBO;
	mCameraUBO->bindToProgram(mShaders["GBufferShader"]->getProgramHandle());
}

void FGBufferRenderer::render() {
	if (!mScene) return;
	auto shader = mShaders["GBufferShader"];
	shader->use();
	setRenderState();
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->uploadColorAttachment();
	glViewport(0, 0, mWindowWidth, mWindowHeight);
	glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	int slot = 0;
	//FCameraInfo cameraUBOdata = getCameraInfo(mScene->mCamera);
	//mCameraUBO->updateData(cameraUBOdata, 0);
	mCameraUBO->bind();
	shader->uniform1f(mScene->mCameraFar, "far");
	shader->uniform1f(mScene->mCameraNear, "near");
	for (const auto& asset : mScene->mAsset) {
		for (const auto& pair : asset->mMesh) {
			auto mesh = pair.first;
			size_t materialIndex = pair.second;
			auto material = asset->mMaterial[materialIndex];

			slot = material->MaterialSlotMap.at("albedo");
			glActiveTexture(GL_TEXTURE0 + slot);
			glBindTexture(GL_TEXTURE_2D, material->getAlbedo()->getHandle());
			shader->uniform1i(slot, material->MaterialNameMap.at("albedo").c_str());
			slot = material->MaterialSlotMap.at("roughness");
			glActiveTexture(GL_TEXTURE0 + slot);
			glBindTexture(GL_TEXTURE_2D, material->getRoughness()->getHandle());
			shader->uniform1i(slot, material->MaterialNameMap.at("roughness").c_str());
			slot = material->MaterialSlotMap.at("metallic");
			glActiveTexture(GL_TEXTURE0 + slot);
			glBindTexture(GL_TEXTURE_2D, material->getMetallic()->getHandle());
			shader->uniform1i(slot, material->MaterialNameMap.at("metallic").c_str());

			glm::mat4 modelMatrix = asset->getModelMatrix() * mesh->getModelMatrix();
			glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
			shader->uniform4x4f(modelMatrix, "modelMatrix");
			shader->uniform3x3f(normalMatrix, "normalMatrix");
			mesh->draw();
		}
	}
	mFrameBuffer->unbindFBO();
	mCameraUBO->unbind();
	shader->end();
}
#endif

void FScreenSpaceQuadRenderer::initialize() {

}

void FScreenSpaceQuadRenderer::render() {
	if (!mScene) return;
	std::shared_ptr<FShader> shader = mShaders["ScreenSpaceQuadShader"];
	shader->use();
	setRenderState();
	glViewport(0, 0, mWindowWidth, mWindowHeight);
	glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	int slot = 0;
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	glBindTextureUnit(slot, getTexture("BackBufferTexture", ETextureUsage::ShaderResource)->getHandle());
#else
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, getTexture("BackBufferTexture", ETextureUsage::ShaderResource)->getHandle());
#endif
	shader->uniform1i(slot, "samp");
	auto quadMesh = mScene->mScreenQuadMesh;
	quadMesh->draw();
	//queryBoundTextureDetails(GL_TEXTURE0 + slot, GL_TEXTURE_2D);
	shader->end();
}

void FScene::createScreenQuad() {
	std::vector<float> vertexPtr = {};
	std::vector<uint32_t> elePtr = {};
	std::shared_ptr<FVertexBufferDesc> descPtr = std::make_shared<FVertexBufferDesc>();
	GeneratePlaneBufferData(
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		2.0f, 2.0f,
		vertexPtr,
		elePtr,
		descPtr
	);
	mScreenQuadMesh = std::make_shared<FMesh>();
	mScreenQuadMesh->uploadCompleteData(
		std::move(vertexPtr),
		std::move(elePtr),
		descPtr
	);
}

void FDirectLightRenderer::setRenderState() {
	glDisable(GL_DEPTH_TEST);
	glClearDepth(1.0f);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);
	glStencilMask(0xFF);
	glClearStencil(0);
}

void FDirectLightRenderer::initialize() {
	mFrameBuffer = std::make_shared<FFrameBuffer>();

	std::shared_ptr<FTexture> directLight = std::make_shared<FTexture>();
	//std::shared_ptr<FTextureDesc> directLightDesc = std::make_shared<FTextureDesc>(
	//	GetGLFormat(TextureFormat::RGBA8), mWindowWidth, mWindowHeight, false);
	std::shared_ptr<FTextureDesc> directLightDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::RGBA16F), mWindowWidth, mWindowHeight);
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	directLight->applyResource(directLightDesc);
#else
	directLight->allocateResource(0, directLightDesc);
#endif
	addTexture("DirectLight", ETextureUsage::RenderTarget, directLight);
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, directLight, 0);
	mFrameBuffer->unbindFBO();
	mCameraUBO = mScene->mCameraUBO;
	mLightUBO = mScene->mLightUBO;
	mCameraUBO->bindToProgram(mShaders["DirectLightShader"]->getProgramHandle());
	mLightUBO->bindToProgram(mShaders["DirectLightShader"]->getProgramHandle());
}

void FDirectLightRenderer::render() {
	if (!mScene) return;
	auto shader = mShaders["DirectLightShader"];
	shader->use();
	setRenderState();
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->uploadColorAttachment();
	glViewport(0, 0, mWindowWidth, mWindowHeight);
	glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	int slot = 0;
	//FCameraInfo cameraUBOdata = getCameraInfo(mScene->mCamera);
	//mCameraUBO->updateData(cameraUBOdata, 0);
	//mLightUBO->updateData(*(mScene->mMainLight), 0);
	mCameraUBO->bind();
	mLightUBO->bind();
	auto mesh = mScene->mMainLightVolume;
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	slot = mScene->mGBufferSlotMap.at("GBuffer_Albedo");
	glBindTextureUnit(slot, getTexture("GBuffer_Albedo", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "AlbedoSamp");
	slot = mScene->mGBufferSlotMap.at("GBuffer_RoughnessMetallicLinearDepth");
	glBindTextureUnit(slot, getTexture("GBuffer_RoughnessMetallicLinearDepth", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "RoughnessMetallicLinearDepthSamp");
	slot = mScene->mGBufferSlotMap.at("GBuffer_Normal");
	glBindTextureUnit(slot, getTexture("GBuffer_Normal", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "NormalSamp");

	slot += 1;
	glBindTextureUnit(slot, mScene->mLUT["LTCMat"]->getHandle());
	shader->uniform1i(slot, "LTCMatSamp");
	slot += 1;
	glBindTextureUnit(slot, mScene->mLUT["LTCAmp"]->getHandle());
	shader->uniform1i(slot, "LTCAmpSamp");
	slot += 1;
	glBindTextureUnit(slot, getTexture("ShadowFactor", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "shadowFactorSamp");
#else
	slot = mScene->mGBufferSlotMap.at("GBuffer_Albedo");
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("GBuffer_Albedo", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "AlbedoSamp");
	slot = mScene->mGBufferSlotMap.at("GBuffer_RoughnessMetallicLinearDepth");
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("GBuffer_RoughnessMetallicLinearDepth", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "RoughnessMetallicLinearDepthSamp");
	slot = mScene->mGBufferSlotMap.at("GBuffer_Normal");
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("GBuffer_Normal", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "NormalSamp");

	slot += 1;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, mScene->mLUT["LTCMat"]->getHandle());
	shader->uniform1i(slot, "LTCMatSamp");
	slot += 1;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, mScene->mLUT["LTCAmp"]->getHandle());
	shader->uniform1i(slot, "LTCAmpSamp");
	slot += 1;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("ShadowFactor", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "shadowFactorSamp");
#endif
	glm::mat4 worldToClip = mScene->mCamera->getProjectionMatrix() *
		mScene->mCamera->getViewMatrix() * 
		mesh->getModelMatrix();
	shader->uniform4x4f(worldToClip, "worldToClip");
	shader->uniform4x4f(glm::inverse(mScene->mCamera->getViewMatrix()), "cameraInvView");
	mesh->draw();
	mFrameBuffer->unbindFBO();
	mLightUBO->unbind();
	mCameraUBO->unbind();
	shader->end();	
}

FShadowProjectionRenderer::FShadowProjectionRenderer(std::shared_ptr<FShadowTerm> inShadowTerm)
	: mShadowTerm{ inShadowTerm } {
}

void FShadowProjectionRenderer::setRenderState() {
	glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_GREATER);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glClearDepth(1.0f);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);
	glStencilMask(0xFF);
	glClearStencil(0);
}

void FShadowProjectionRenderer::initialize() {
	mFrameBuffer = std::make_shared<FFrameBuffer>();
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	std::shared_ptr<FTexture> shadowMap = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> shadowMapDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::R16F), mWindowWidth, mWindowHeight);
	shadowMap->applyResource(shadowMapDesc);
	addTexture("Shadow_Map", ETextureUsage::RenderTarget, shadowMap);
	std::shared_ptr<FTexture> depth = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> depthDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::Depth24S8), mWindowWidth, mWindowHeight);
	depth->applyResource(depthDesc);
	addTexture("Depth", ETextureUsage::DepthStencil, depth);
#else
	std::shared_ptr<FTexture> shadowMap = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> shadowMapDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::R16F), mWindowWidth, mWindowHeight, false);
	shadowMap->allocateResource(0, shadowMapDesc);
	addTexture("Shadow_Map", ETextureUsage::RenderTarget, shadowMap);
	std::shared_ptr<FTexture> depth = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> depthDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::Depth24S8), mWindowWidth, mWindowHeight, false);
	depth->allocateResource(0, depthDesc);
	addTexture("Depth", ETextureUsage::DepthStencil, depth);
#endif
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, shadowMap, 0);
	mFrameBuffer->attach(ETextureUsage::DepthStencil, depth);
	mFrameBuffer->unbindFBO();
}

void FShadowProjectionRenderer::render() {
	if (!mScene) return;
	auto shader = mShaders["ShadowMapProjShader"];
	shader->use();
	setRenderState();
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->uploadColorAttachment();
	glViewport(0, 0, mWindowWidth, mWindowHeight);
	//glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	shader->uniform4f(mShadowTerm->refineLightPosition, "lightPosition");
	shader->uniform4x4f(mShadowTerm->lightViewMatrix, "lightViewMatrix");
	shader->uniform4x4f(mShadowTerm ->lightProjMatrix, "lightProjMatrix");
	shader->uniform4f(
		glm::vec4(mShadowTerm->maxTan, mShadowTerm->slopeTan, mShadowTerm->constTan, mShadowTerm->mNear),
		"combineDepthParam"
	);
	shader->uniform2f(glm::vec2(mShadowTerm->mFar, mShadowTerm->mNear), "lightFN");

	for (const auto& asset : mScene->mAsset) {
		if (asset->mShadow == false) {
			continue;
		}
		for (const auto& pair : asset->mMesh) {
			auto mesh = pair.first;
			size_t materialIndex = pair.second;
			auto material = asset->mMaterial[materialIndex];
			
			glm::mat4 modelMatrix = asset->getModelMatrix() * mesh->getModelMatrix();
			glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
			shader->uniform4x4f(modelMatrix, "modelMatrix");
			shader->uniform3x3f(normalMatrix, "normalMatrix");
			mesh->draw();
		}
	}
	mFrameBuffer->unbindFBO();
	shader->end();
}

#if OPEN_DSA_AND_BINDLESS_TEXTURE
void FIBLGenerateRenderer::initialize() {
	mFrameBuffer = std::make_shared<FFrameBuffer>();
	//auto type = TextureFormat::RGBA8;
	auto type = TextureFormat::RGBA32F;

	std::shared_ptr<FTexture> diffuse = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> diffuseDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(type), mWindowWidth, mWindowHeight);
	diffuse->applyResource(diffuseDesc);
	addTexture("diffuse", ETextureUsage::RenderTarget, diffuse);

	std::shared_ptr<FTexture> specularR1 = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> specularR1Desc = std::make_shared<FTextureDesc>(
		GetGLFormat(type), mWindowWidth, mWindowHeight);
	specularR1->applyResource(specularR1Desc);
	addTexture("specularR1", ETextureUsage::RenderTarget, specularR1);

	std::shared_ptr<FTexture> specularR2 = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> specularR2Desc = std::make_shared<FTextureDesc>(
		GetGLFormat(type), mWindowWidth, mWindowHeight);
	specularR2->applyResource(specularR1Desc);
	addTexture("specularR2", ETextureUsage::RenderTarget, specularR2);

	std::shared_ptr<FTexture> specularR3 = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> specularR3Desc = std::make_shared<FTextureDesc>(
		GetGLFormat(type), mWindowWidth, mWindowHeight);
	specularR3->applyResource(specularR1Desc);
	addTexture("specularR3", ETextureUsage::RenderTarget, specularR3);

	std::shared_ptr<FTexture> specularR4 = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> specularR4Desc = std::make_shared<FTextureDesc>(
		GetGLFormat(type), mWindowWidth, mWindowHeight);
	specularR4->applyResource(specularR1Desc);
	addTexture("specularR4", ETextureUsage::RenderTarget, specularR4);

	std::shared_ptr<FTexture> specularR5 = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> specularR5Desc = std::make_shared<FTextureDesc>(
		GetGLFormat(type), mWindowWidth, mWindowHeight);
	specularR5->applyResource(specularR1Desc);
	addTexture("specularR5", ETextureUsage::RenderTarget, specularR5);

	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, diffuse, 0);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, specularR1, 1);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, specularR2, 2);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, specularR3, 3);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, specularR4, 4);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, specularR5, 5);
	mFrameBuffer->unbindFBO();
}

void FIBLGenerateRenderer::render() {
	if (!mScene) return;
	std::shared_ptr<FShader> shader = mShaders["IBLGenerateShader"];
	shader->use();
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->uploadColorAttachment();
	setRenderState();
	glViewport(0, 0, mWindowWidth, mWindowHeight);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	int slot = 0;
	glBindTextureUnit(slot, getTexture("resource", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "cubemapSampler");
	shader->uniform1f(1024.0f, "size");
	//shader->uniform1i(128, "sampleNumsDiffuse");
	//shader->uniform1i(128, "sampleNumsSpecular");
	shader->uniform1i(1024, "sampleNumsDiffuse");
	shader->uniform1i(1024, "sampleNumsSpecular");
	for (int i = 0; i < 6; i++) {
		shader->uniform1i(i, "faceIndex");
		auto quadMesh = mScene->mScreenQuadMesh;
		quadMesh->draw();
		SaveRenderTarget(mWindowWidth, mWindowHeight, GL_COLOR_ATTACHMENT0,
			mOutPath + "Vis/" + std::to_string(i) + "_diffuse.png", EOutputFormat::LDR_8BIT, EYFlip::FlipY);
		SaveRenderTarget(mWindowWidth, mWindowHeight, GL_COLOR_ATTACHMENT0,
			mOutPath + "RT/" + std::to_string(i) + "_diffuse.hdr", EOutputFormat::HDR_FLOAT, EYFlip::FlipY);
		for (int j = 0; j < 5; j++) {
			SaveRenderTarget(mWindowWidth, mWindowHeight, GL_COLOR_ATTACHMENT1 + j,
				mOutPath + "Vis/" + std::to_string(i) + "_specular" + std::to_string(j) + ".png",
				EOutputFormat::LDR_8BIT, EYFlip::FlipY);
			SaveRenderTarget(mWindowWidth, mWindowHeight, GL_COLOR_ATTACHMENT1 + j,
				mOutPath + "RT/" + std::to_string(i) + "_specular" + std::to_string(j) + ".hdr",
				EOutputFormat::HDR_FLOAT, EYFlip::FlipY);
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}
	//queryBoundTextureDetails(GL_TEXTURE0 + slot, GL_TEXTURE_2D);
	mFrameBuffer->unbindFBO();
	shader->end();
}
#else
void FIBLGenerateRenderer::initialize() {
	mFrameBuffer = std::make_shared<FFrameBuffer>();
	//auto type = TextureFormat::RGBA8;
	auto type = TextureFormat::RGBA32F;

	std::shared_ptr<FTexture> diffuse = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> diffuseDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(type), mWindowWidth, mWindowHeight, false);
	diffuse->allocateResource(0, diffuseDesc);
	addTexture("diffuse", ETextureUsage::RenderTarget, diffuse);

	std::shared_ptr<FTexture> specularR1 = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> specularR1Desc = std::make_shared<FTextureDesc>(
		GetGLFormat(type), mWindowWidth, mWindowHeight, false);
	specularR1->allocateResource(0, specularR1Desc);
	addTexture("specularR1", ETextureUsage::RenderTarget, specularR1);

	std::shared_ptr<FTexture> specularR2 = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> specularR2Desc = std::make_shared<FTextureDesc>(
		GetGLFormat(type), mWindowWidth, mWindowHeight, false);
	specularR2->allocateResource(0, specularR2Desc);
	addTexture("specularR2", ETextureUsage::RenderTarget, specularR2);

	std::shared_ptr<FTexture> specularR3 = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> specularR3Desc = std::make_shared<FTextureDesc>(
		GetGLFormat(type), mWindowWidth, mWindowHeight, false);
	specularR3->allocateResource(0, specularR3Desc);
	addTexture("specularR3", ETextureUsage::RenderTarget, specularR3);

	std::shared_ptr<FTexture> specularR4 = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> specularR4Desc = std::make_shared<FTextureDesc>(
		GetGLFormat(type), mWindowWidth, mWindowHeight, false);
	specularR4->allocateResource(0, specularR4Desc);
	addTexture("specularR4", ETextureUsage::RenderTarget, specularR4);

	std::shared_ptr<FTexture> specularR5 = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> specularR5Desc = std::make_shared<FTextureDesc>(
		GetGLFormat(type), mWindowWidth, mWindowHeight, false);
	specularR5->allocateResource(0, specularR5Desc);
	addTexture("specularR5", ETextureUsage::RenderTarget, specularR5);

	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, diffuse, 0);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, specularR1, 1);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, specularR2, 2);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, specularR3, 3);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, specularR4, 4);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, specularR5, 5);
	mFrameBuffer->unbindFBO();
}

void FIBLGenerateRenderer::render() {
	if (!mScene) return;
	std::shared_ptr<FShader> shader = mShaders["IBLGenerateShader"];
	shader->use();
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->uploadColorAttachment();
	setRenderState();
	glViewport(0, 0, mWindowWidth, mWindowHeight);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	int slot = 0;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_CUBE_MAP, getTexture("resource", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "cubemapSampler");
	shader->uniform1f(1024.0f, "size");
	//shader->uniform1i(128, "sampleNumsDiffuse");
	//shader->uniform1i(128, "sampleNumsSpecular");
	shader->uniform1i(1024, "sampleNumsDiffuse");
	shader->uniform1i(1024, "sampleNumsSpecular");
	for (int i = 0; i < 6; i++) {
		shader->uniform1i(i, "faceIndex");
		auto quadMesh = mScene->mScreenQuadMesh;
		quadMesh->draw();
		SaveRenderTarget(mWindowWidth, mWindowHeight, GL_COLOR_ATTACHMENT0,
			mOutPath +"Vis/" + std::to_string(i) + "_diffuse.png", EOutputFormat::LDR_8BIT, EYFlip::FlipY);
		SaveRenderTarget(mWindowWidth, mWindowHeight, GL_COLOR_ATTACHMENT0,
			mOutPath + "RT/" + std::to_string(i) + "_diffuse.hdr", EOutputFormat::HDR_FLOAT, EYFlip::FlipY);
		for (int j = 0; j < 5; j++) {
			SaveRenderTarget(mWindowWidth, mWindowHeight, GL_COLOR_ATTACHMENT1 + j,
				mOutPath + "Vis/" + std::to_string(i) + "_specular"+ std::to_string(j) +".png", 
				EOutputFormat::LDR_8BIT, EYFlip::FlipY);
			SaveRenderTarget(mWindowWidth, mWindowHeight, GL_COLOR_ATTACHMENT1 + j,
				mOutPath + "RT/" + std::to_string(i) + "_specular" + std::to_string(j) + ".hdr",
				EOutputFormat::HDR_FLOAT, EYFlip::FlipY);
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}
	//queryBoundTextureDetails(GL_TEXTURE0 + slot, GL_TEXTURE_2D);
	mFrameBuffer->unbindFBO();
	shader->end();
}
#endif
void FEnvBRDFRenderer::initialize() {
	mFrameBuffer = std::make_shared<FFrameBuffer>();
	std::shared_ptr<FTexture> envBRDF = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> envBRDFDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::RGBA32F), mWindowWidth, mWindowHeight);
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	envBRDF->applyResource(envBRDFDesc);
#else
	envBRDF->allocateResource(0, envBRDFDesc);
#endif
	addTexture("envBRDF", ETextureUsage::RenderTarget, envBRDF);

	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, envBRDF, 0);
	mFrameBuffer->unbindFBO();
}

void FEnvBRDFRenderer::render() {
	if (!mScene) return;
	std::shared_ptr<FShader> shader = mShaders["EnvBRDFShader"];
	shader->use();
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->uploadColorAttachment();
	setRenderState();
	glViewport(0, 0, mWindowWidth, mWindowHeight);
	glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	shader->uniform1f(1024.0f, "size");
	shader->uniform1i(1024, "sampleNums");
	auto quadMesh = mScene->mScreenQuadMesh;
	quadMesh->draw();
	SaveRenderTarget(mWindowWidth, mWindowHeight, GL_COLOR_ATTACHMENT0, 
		mOutPath + "RT/envBRDF.png", EOutputFormat::LDR_8BIT, EYFlip::FlipY);
	mFrameBuffer->unbindFBO();
	shader->end();
}

FShadowRenderer::FShadowRenderer(std::shared_ptr<FShadowTerm> inShadowTerm)
	: mShadowTerm{ inShadowTerm } {
}

void FShadowRenderer::initialize() {
	mFrameBuffer = std::make_shared<FFrameBuffer>();
	std::shared_ptr<FTexture> shadow = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> shadowDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::R16F), mWindowWidth, mWindowHeight);
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	shadow->applyResource(shadowDesc);
#else
	shadow->allocateResource(0, shadowDesc);
#endif
	addTexture("ShadowFactor", ETextureUsage::RenderTarget, shadow);

	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, shadow, 0);
	mFrameBuffer->unbindFBO();

	mCameraUBO = mScene->mCameraUBO;
	mCameraUBO->bindToProgram(mShaders["ShadowShader"]->getProgramHandle());
}

void FShadowRenderer::render() {
	if (!mScene) return;
	std::shared_ptr<FShader> shader = mShaders["ShadowShader"];
	shader->use();
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->uploadColorAttachment();
	setRenderState();
	glViewport(0, 0, mWindowWidth, mWindowHeight);
	glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	auto quadMesh = mScene->mScreenQuadMesh;
	mCameraUBO->bind();
	shader->uniform4f(mShadowTerm->refineLightPosition, "lightPosition");
	shader->uniform4f(mShadowTerm->lightAngleInfo, "lightAngleInfo");
	shader->uniform4x4f(mShadowTerm->lightViewMatrix, "lightViewMatrix");
	shader->uniform4x4f(mShadowTerm->lightProjMatrix, "lightProjMatrix");
	shader->uniform4x4f(glm::inverse(mScene->mCamera->getViewMatrix()), "cameraInvView");
	shader->uniform2f(glm::vec2(mShadowTerm->mFar, mShadowTerm->mNear), "lightFN");
	shader->uniform2f(glm::vec2(1.0f/static_cast<float>(mShadowTerm->mShadowMapWidth), 1.0f/static_cast<float>(mShadowTerm->mShadowMapHeight)), "invShadowMapSize");
	shader->uniform1i(16, "searchSampleNum");
	shader->uniform1i(32, "occSampleNum");
	shader->uniform1i(mScene->mFrameIndex % 8, "frameIndexMod8");
	//shader->uniform1i(8, "searchSampleNum");
	//shader->uniform1i(8, "occSampleNum");
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	int slot = 0;
	glBindTextureUnit(slot, getTexture("GBuffer_RoughnessMetallicLinearDepth", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "RoughnessMetallicLinearDepthSamp");
	slot += 1;
	glBindTextureUnit(slot, getTexture("shadowProjTexture", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "shadowProjSamp");
#else
	int slot = 0;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("GBuffer_RoughnessMetallicLinearDepth", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "RoughnessMetallicLinearDepthSamp");
	slot += 1;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("shadowProjTexture", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "shadowProjSamp");
#endif
	quadMesh->draw();

	mFrameBuffer->unbindFBO();
	mCameraUBO->unbind();
	shader->end();
}

void FPingPongFilterRenderer::initialize() {
	std::shared_ptr<FTexture> PingPong = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> PingPongDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::R16F), mWindowWidth, mWindowHeight);
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	PingPong->applyResource(PingPongDesc);
#else
	PingPong->allocateResource(0, PingPongDesc);
#endif
	addTexture("PingPong2", ETextureUsage::UnorderedAccess, PingPong);
}

void FPingPongFilterRenderer::render() {
	if (!mScene) return;
	std::shared_ptr<FShader> shader = mShaders["PingPongFilterShader"];
	shader->use();
	shader->uniform1i(mWindowWidth, "outWidth");
	shader->uniform1i(mWindowHeight, "outHeight");
	const int localSizeX = 16, localSizeY = 16;
	const int threadGroupNumX = (mWindowWidth + localSizeX - 1) / localSizeX;
	const int threadGroupNumY = (mWindowHeight + localSizeY - 1) / localSizeY;
	const int threadGroupNumZ = 1;
	bool horizonal = true;
	for (int i = 0; i < mFilterRound; i++) {
		shader->uniform1i(horizonal, "horizonal");
		glBindImageTexture(mReadonlyBind, getTexture("PingPong", ETextureUsage::UnorderedAccess)->getHandle(), 
			0, GL_FALSE, 0, GL_READ_ONLY, GL_R16F);
		glBindImageTexture(mWriteonlyBind, getTexture("PingPong2", ETextureUsage::UnorderedAccess)->getHandle(),
			0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);
		glDispatchCompute(threadGroupNumX, threadGroupNumY, threadGroupNumZ);
		glMemoryBarrier(
			GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
			GL_SHADER_STORAGE_BARRIER_BIT |
			GL_TEXTURE_FETCH_BARRIER_BIT
		);

		shader->uniform1i(!horizonal, "horizonal");
		glBindImageTexture(mReadonlyBind, getTexture("PingPong2", ETextureUsage::UnorderedAccess)->getHandle(),
			0, GL_FALSE, 0, GL_READ_ONLY, GL_R16F);
		glBindImageTexture(mWriteonlyBind, getTexture("PingPong", ETextureUsage::UnorderedAccess)->getHandle(),
			0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);
		glDispatchCompute(threadGroupNumX, threadGroupNumY, threadGroupNumZ);
		glMemoryBarrier(
			GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
			GL_SHADER_STORAGE_BARRIER_BIT |
			GL_TEXTURE_FETCH_BARRIER_BIT
		);
	}
	shader->end();
}

void FTemporalAntiAliasingRenderer::initialize() {
	mFrameBuffer = std::make_shared<FFrameBuffer>();

	std::shared_ptr<FTexture> TAAResult = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> TAAResultDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::RGBA16F), mWindowWidth, mWindowHeight);
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	TAAResult->applyResource(TAAResultDesc);
#else
	TAAResult->allocateResource(0, TAAResultDesc);
#endif
	addTexture("TAAResult", ETextureUsage::RenderTarget, TAAResult);
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, TAAResult, 0);
	mFrameBuffer->unbindFBO();
}

void FTemporalAntiAliasingRenderer::render() {
	if (!mScene) return;
	std::shared_ptr<FShader> shader = mShaders["TAAShader"];
	shader->use();
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->uploadColorAttachment();
	setRenderState();
	glViewport(0, 0, mWindowWidth, mWindowHeight);
	glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	shader->uniform4x4f(mScene->historyCameraView, "historyCameraView");
	shader->uniform4x4f(glm::inverse(mScene->historyCameraView), "historyCameraInvView");
	shader->uniform4x4f(mScene->mCamera->getViewMatrix(), "currentCameraView");
	shader->uniform4x4f(mScene->mCamera->getInvViewMatrix(), "currentCameraInvView");
	shader->uniform4x4f(mScene->mCamera->getProjectionMatrix(), "cameraProjection");
	shader->uniform2f(glm::vec2(1.0f / static_cast<float>(mWindowWidth), 1.0f / static_cast<float>(mWindowHeight)), "invSize");
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	int slot = 0;
	glBindTextureUnit(slot, getTexture("TAAInput", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "currentTAAInput");
	slot += 1;
	glBindTextureUnit(slot, getTexture("Depth", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "RoughnessMetallicLinearDepthSamp");
	slot += 1;
	if (!mInitialized) {
		glBindTextureUnit(slot, getTexture("TAAInput", ETextureUsage::ShaderResource)->getHandle());
		mInitialized = true;
	}
	else {
		glBindTextureUnit(slot, getTexture("TAAHistoryInput", ETextureUsage::ShaderResource)->getHandle());
	}
	shader->uniform1i(slot, "historyTAAInput");
#else
	int slot = 0;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("TAAInput", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "currentTAAInput");
	slot += 1;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("Depth", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "RoughnessMetallicLinearDepthSamp");
	slot += 1;
	glActiveTexture(GL_TEXTURE0 + slot);
	if (!mInitialized) {
		glBindTexture(GL_TEXTURE_2D, getTexture("TAAInput", ETextureUsage::ShaderResource)->getHandle());
		mInitialized = true;
	}
	else {
		glBindTexture(GL_TEXTURE_2D, getTexture("TAAHistoryInput", ETextureUsage::ShaderResource)->getHandle());
	}
	shader->uniform1i(slot, "historyTAAInput");
#endif
	auto quadMesh = mScene->mScreenQuadMesh;
	quadMesh->draw();
	
	// 后续需要优化，变成ping pong FBO
	glCopyImageSubData(
		getTexture("TAAResult", ETextureUsage::RenderTarget)->getHandle(), GL_TEXTURE_2D, 0, 0, 0, 0,
		getTexture("TAAHistoryInput", ETextureUsage::ShaderResource)->getHandle(), GL_TEXTURE_2D, 0, 0, 0, 0,
		mWindowWidth, mWindowHeight, 1
	);
	mFrameBuffer->unbindFBO();
	shader->end();
}

void FTemporalAntiAliasingCSRenderer::initialize() {
	std::shared_ptr<FTexture> TAAResult = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> TAAResultDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::RGBA16F), mWindowWidth, mWindowHeight);
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	TAAResult->applyResource(TAAResultDesc);
#else
	TAAResult->allocateResource(0, TAAResultDesc);
#endif	
	addTexture("TAAResult", ETextureUsage::UnorderedAccess, TAAResult);
}

void FTemporalAntiAliasingCSRenderer::render() {
	if (!mScene) return;
	std::shared_ptr<FShader> shader = mShaders["TAAShader"];
	shader->use();

	shader->uniform4x4f(mScene->historyCameraView, "historyCameraView");
	shader->uniform4x4f(glm::inverse(mScene->historyCameraView), "historyCameraInvView");
	shader->uniform4x4f(mScene->mCamera->getViewMatrix(), "currentCameraView");
	shader->uniform4x4f(mScene->mCamera->getInvViewMatrix(), "currentCameraInvView");
	shader->uniform4x4f(mScene->mCamera->getProjectionMatrix(), "cameraProjection");
	shader->uniform2f(glm::vec2(1.0f / static_cast<float>(mWindowWidth), 1.0f / static_cast<float>(mWindowHeight)), "invSize");
	shader->uniform1i(mWindowWidth, "outWidth");
	shader->uniform1i(mWindowHeight, "outHeight");

	glBindImageTexture(0, getTexture("TAAInput", ETextureUsage::UnorderedAccess)->getHandle(),
		0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
	if (!mInitialized) {
		glBindImageTexture(1, getTexture("TAAInput", ETextureUsage::UnorderedAccess)->getHandle(),
			0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
		mInitialized = true;
	}
	else {
		glBindImageTexture(1, getTexture("TAAHistoryInput", ETextureUsage::UnorderedAccess)->getHandle(),
			0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
	}
	glBindImageTexture(2, getTexture("TAAResult", ETextureUsage::UnorderedAccess)->getHandle(),
		0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glBindImageTexture(3, getTexture("Depth", ETextureUsage::UnorderedAccess)->getHandle(),
		0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
	const int localSizeX = 16, localSizeY = 16;
	const int threadGroupNumX = (mWindowWidth + localSizeX - 1) / localSizeX;
	const int threadGroupNumY = (mWindowHeight + localSizeY - 1) / localSizeY;
	const int threadGroupNumZ = 1;
	glDispatchCompute(threadGroupNumX, threadGroupNumY, threadGroupNumZ);
	glMemoryBarrier(
		GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT
	);
	glCopyImageSubData(
		getTexture("TAAResult", ETextureUsage::UnorderedAccess)->getHandle(), GL_TEXTURE_2D, 0, 0, 0, 0,
		getTexture("TAAHistoryInput", ETextureUsage::UnorderedAccess)->getHandle(), GL_TEXTURE_2D, 0, 0, 0, 0,
		mWindowWidth, mWindowHeight, 1
	);
	shader->end();
}

void FShadowTemporalAntiAliasingCSRenderer::initialize() {
	std::shared_ptr<FTexture> TAAResult = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> TAAResultDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::R16F), mWindowWidth, mWindowHeight);
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	TAAResult->applyResource(TAAResultDesc);
#else
	TAAResult->allocateResource(0, TAAResultDesc);
#endif
	addTexture("TAAResult", ETextureUsage::UnorderedAccess, TAAResult);
}

void FShadowTemporalAntiAliasingCSRenderer::render() {
	if (!mScene) return;
	std::shared_ptr<FShader> shader = mShaders["ShadowTAAShader"];
	shader->use();

	shader->uniform4x4f(mScene->historyCameraView, "historyCameraView");
	shader->uniform4x4f(glm::inverse(mScene->historyCameraView), "historyCameraInvView");
	shader->uniform4x4f(mScene->mCamera->getViewMatrix(), "currentCameraView");
	shader->uniform4x4f(mScene->mCamera->getInvViewMatrix(), "currentCameraInvView");
	shader->uniform4x4f(mScene->mCamera->getProjectionMatrix(), "cameraProjection");
	shader->uniform2f(glm::vec2(1.0f / static_cast<float>(mWindowWidth), 1.0f / static_cast<float>(mWindowHeight)), "invSize");
	shader->uniform1i(mWindowWidth, "outWidth");
	shader->uniform1i(mWindowHeight, "outHeight");

	glBindImageTexture(0, getTexture("TAAInput", ETextureUsage::UnorderedAccess)->getHandle(),
		0, GL_FALSE, 0, GL_READ_ONLY, GL_R16F);
	if (!mInitialized) {
		glBindImageTexture(1, getTexture("TAAInput", ETextureUsage::UnorderedAccess)->getHandle(),
			0, GL_FALSE, 0, GL_READ_ONLY, GL_R16F);
		mInitialized = true;
	}
	else {
		glBindImageTexture(1, getTexture("TAAHistoryInput", ETextureUsage::UnorderedAccess)->getHandle(),
			0, GL_FALSE, 0, GL_READ_ONLY, GL_R16F);
	}
	glBindImageTexture(2, getTexture("TAAResult", ETextureUsage::UnorderedAccess)->getHandle(),
		0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);
	glBindImageTexture(3, getTexture("Depth", ETextureUsage::UnorderedAccess)->getHandle(),
		0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
	const int localSizeX = 16, localSizeY = 16;
	const int threadGroupNumX = (mWindowWidth + localSizeX - 1) / localSizeX;
	const int threadGroupNumY = (mWindowHeight + localSizeY - 1) / localSizeY;
	const int threadGroupNumZ = 1;
	glDispatchCompute(threadGroupNumX, threadGroupNumY, threadGroupNumZ);
	glMemoryBarrier(
		GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT
	);
	glCopyImageSubData(
		getTexture("TAAResult", ETextureUsage::UnorderedAccess)->getHandle(), GL_TEXTURE_2D, 0, 0, 0, 0,
		getTexture("TAAHistoryInput", ETextureUsage::UnorderedAccess)->getHandle(), GL_TEXTURE_2D, 0, 0, 0, 0,
		mWindowWidth, mWindowHeight, 1
	);
	shader->end();
}

void FHiZRenderer::initialize() {
	std::shared_ptr<FTexture> HiZ = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> HiZDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::R16F), mWindowWidth, mWindowHeight, true);
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	HiZ->applyResource(HiZDesc);
#else
	HiZ->allocateResource(0, HiZDesc);
#endif
	addTexture("HiZ", ETextureUsage::UnorderedAccess, HiZ);
}

void FHiZRenderer::render() {
	if (!mScene) return;
	glCopyImageSubData(
		getTexture("inDepth", ETextureUsage::UnorderedAccess)->getHandle(), GL_TEXTURE_2D, 0, 0, 0, 0,
		getTexture("HiZ", ETextureUsage::UnorderedAccess)->getHandle(), GL_TEXTURE_2D, 0, 0, 0, 0,
		mWindowWidth, mWindowHeight, 1
	);
	std::shared_ptr<FShader> shader = mShaders["HiZShader"];
	const int localSizeX = 16, localSizeY = 16;
	const int threadGroupNumZ = 1;
	shader->use();

	int width  = mWindowWidth;
	int height = mWindowHeight;
	shader->uniform1i(true, "calcMax");
	for (int i = 0; i < mHiZLevel; i++) {
		shader->uniform1i(width, "inWidth");
		shader->uniform1i(height, "inHeight");
		width = std::max(1, mWindowWidth >> (i + 1));
		height = std::max(1, mWindowHeight >> (i + 1));
		shader->uniform1i(width, "outWidth");
		shader->uniform1i(height, "outHeight");
		glBindImageTexture(0, getTexture("HiZ", ETextureUsage::UnorderedAccess)->getHandle(),
			i, GL_FALSE, 0, GL_READ_ONLY, GL_R16F);
		glBindImageTexture(1, getTexture("HiZ", ETextureUsage::UnorderedAccess)->getHandle(),
			i+1, GL_FALSE, 0, GL_READ_ONLY, GL_R16F);

		int threadGroupNumX = (width + localSizeX - 1) / localSizeX;
		int threadGroupNumY = (height + localSizeY - 1) / localSizeY;
		glDispatchCompute(threadGroupNumX, threadGroupNumY, threadGroupNumZ);
		glMemoryBarrier(
			GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
			GL_SHADER_STORAGE_BARRIER_BIT |
			GL_TEXTURE_FETCH_BARRIER_BIT
		);
	}
	
	shader->end();
}

void FRefineHiZRenderer::initialize() {

}

void FRefineHiZRenderer::render() {
	if (!mScene) return;
	std::shared_ptr<FShader> shader = mShaders["HiZShader"];
	const int localSizeX = 16, localSizeY = 16;
	const int threadGroupNumZ = 1;
	shader->use();

	int width = mWindowWidth;
	int height = mWindowHeight;
	shader->uniform1i(true, "calcMax");
	for (int i = 0; i < mHiZLevel; i++) {
		shader->uniform1i(width, "inWidth");
		shader->uniform1i(height, "inHeight");
		width = std::max(1, mWindowWidth >> (i + 1));
		height = std::max(1, mWindowHeight >> (i + 1));
		shader->uniform1i(width, "outWidth");
		shader->uniform1i(height, "outHeight");
		glBindImageTexture(0, getTexture("HiZ", ETextureUsage::UnorderedAccess)->getHandle(),
			i, GL_FALSE, 0, GL_READ_ONLY, GL_R16F);
		glBindImageTexture(1, getTexture("HiZ", ETextureUsage::UnorderedAccess)->getHandle(),
			i + 1, GL_FALSE, 0, GL_READ_ONLY, GL_R16F);

		int threadGroupNumX = (width + localSizeX - 1) / localSizeX;
		int threadGroupNumY = (height + localSizeY - 1) / localSizeY;
		glDispatchCompute(threadGroupNumX, threadGroupNumY, threadGroupNumZ);
		glMemoryBarrier(
			GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
			GL_SHADER_STORAGE_BARRIER_BIT |
			GL_TEXTURE_FETCH_BARRIER_BIT
		);
	}

	shader->end();
}

void FExtractChannelRenderer::initialize() {
	std::shared_ptr<FTexture> output = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> outputDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::R16F), mWindowWidth, mWindowHeight);
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	output->applyResource(outputDesc);
#else
	output->allocateResource(0, outputDesc);
#endif
	addTexture("output", ETextureUsage::UnorderedAccess, output);
}

void FExtractChannelRenderer::render() {
	if (!mScene) return;
	std::shared_ptr<FShader> shader = mShaders["ExtractChannelShader"];
	const int localSizeX = 16, localSizeY = 16;
	const int threadGroupNumX = (mWindowWidth + localSizeX - 1) / localSizeX;
	const int threadGroupNumY = (mWindowHeight + localSizeY - 1) / localSizeY;
	const int threadGroupNumZ = 1;
	shader->use();

	shader->uniform1i(mWindowWidth, "inWidth");
	shader->uniform1i(mWindowHeight, "inHeight");
	shader->uniform1i(mChannelIndex, "channelIndex");
	shader->uniform1f(mScene->mCameraFar, "far");
	shader->uniform1f(mScene->mCameraNear, "near");
	glBindImageTexture(0, getTexture("input", ETextureUsage::UnorderedAccess)->getHandle(),
		0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
	glBindImageTexture(1, getTexture("output", ETextureUsage::UnorderedAccess)->getHandle(),
		0, GL_FALSE, 0, GL_READ_ONLY, GL_R16F);
	glDispatchCompute(threadGroupNumX, threadGroupNumY, threadGroupNumZ);
	glMemoryBarrier(
		GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT
	);
	shader->end();
}

void FHorizonalDataRenderer::initialize() {
	std::shared_ptr<FShaderStorageBuffer> dataSSBO = std::make_shared<FShaderStorageBuffer>("horizonalData");
	dataSSBO->initBuffer(sizeof(FHorizonalDataGPU), 0);
	addSSBO("horizonalData", dataSSBO);
}

void FHorizonalDataRenderer::render() {
	if (!mScene) return;
	std::shared_ptr<FShader> shader = mShaders["HorizonalDataShader"];
	const int localSizeX = 8, localSizeY = 8, localSizeZ = 8;
	const int threadGroupNumX = (mWindowWidth + localSizeX - 1) / localSizeX;
	const int threadGroupNumY = (mWindowHeight + localSizeY - 1) / localSizeY;
	const int threadGroupNumZ = 1;
	shader->use();

	shader->uniform1i(mWindowWidth, "HiZM0Width");
	shader->uniform1i(mWindowHeight, "HiZM0Height");
	shader->uniform1i(mWindowWidth / localSizeX, "outWidth");
	shader->uniform1i(mWindowHeight / localSizeY, "outHeight");
	shader->uniform1f(mScene->mCameraFar, "far");

	glBindTextureUnit(0, getTexture("HiZ", ETextureUsage::UnorderedAccess)->getHandle());
	getSSBO("horizonalData")->bindBufferBase();

	glDispatchCompute(threadGroupNumX, threadGroupNumY, threadGroupNumZ);
	glMemoryBarrier(
		GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT
	);
	shader->end();
}

void FScreenSpaceReflectionRenderer::initialize() {
	mFrameBuffer = std::make_shared<FFrameBuffer>();

	std::shared_ptr<FTexture> SSRResult = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> SSRResultDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::RGBA16F), mWindowWidth, mWindowHeight);
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	SSRResult->applyResource(SSRResultDesc);
#else
	SSRResult->allocateResource(0, SSRResultDesc);
#endif
	addTexture("SSRResult", ETextureUsage::RenderTarget, SSRResult);
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, SSRResult, 0);
	mFrameBuffer->unbindFBO();
	mCameraUBO = mScene->mCameraUBO;
	mLightUBO = mScene->mLightUBO;
	mCameraUBO->bindToProgram(mShaders["SSRShader"]->getProgramHandle());
	mLightUBO->bindToProgram(mShaders["SSRShader"]->getProgramHandle());
}

void FScreenSpaceReflectionRenderer::render() {
	if (!mScene) return;
	auto shader = mShaders["SSRShader"];
	shader->use();
	setRenderState();
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->uploadColorAttachment();
	glViewport(0, 0, mWindowWidth, mWindowHeight);
	glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	mCameraUBO->bind();
	mLightUBO->bind();
	auto mesh = mScene->mScreenQuadMesh;

	shader->uniform1f(mScene->mCameraFar, "far");
	shader->uniform1f(mScene->mCameraNear, "near");
	shader->uniform1i(mWindowWidth, "Width");
	shader->uniform1i(mWindowHeight, "Height");
	shader->uniform1i(mScene->mFrameIndex % 8, "frameIndexMod8");
	shader->uniform4x4f(mScene->mCamera->getInvViewMatrix(), "cameraInvView");
	shader->uniform1i(mNumRays, "NumRays");
	shader->uniform1i(mNumSteps, "NumSteps");

	int slot = 0;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("HiZ", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "HiZ");

	slot += 1;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("RMLD", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "RMLD");

	slot += 1;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("normal", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "normal");

	slot += 1;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("SSRInput", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "SSRInput");

	mesh->draw();
	mFrameBuffer->unbindFBO();
	mLightUBO->unbind();
	mCameraUBO->unbind();
	shader->end();
}

void FScreenSpaceGlobalIlluminationRenderer::initialize() {
	mFrameBuffer = std::make_shared<FFrameBuffer>();

	std::shared_ptr<FTexture> SSRResult = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> SSRResultDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::RGBA16F), mWindowWidth, mWindowHeight);
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	SSRResult->applyResource(SSRResultDesc);
#else
	SSRResult->allocateResource(0, SSRResultDesc);
#endif
	addTexture("SSGIResult", ETextureUsage::RenderTarget, SSRResult);
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, SSRResult, 0);
	mFrameBuffer->unbindFBO();
	mCameraUBO = mScene->mCameraUBO;
	mLightUBO = mScene->mLightUBO;
	mCameraUBO->bindToProgram(mShaders["SSGIShader"]->getProgramHandle());
	mLightUBO->bindToProgram(mShaders["SSGIShader"]->getProgramHandle());
}

void FScreenSpaceGlobalIlluminationRenderer::render() {
	if (!mScene) return;
	auto shader = mShaders["SSGIShader"];
	shader->use();
	setRenderState();
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->uploadColorAttachment();
	glViewport(0, 0, mWindowWidth, mWindowHeight);
	glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	mCameraUBO->bind();
	mLightUBO->bind();
	auto mesh = mScene->mScreenQuadMesh;

	shader->uniform1f(mScene->mCameraFar, "far");
	shader->uniform1f(mScene->mCameraNear, "near");
	shader->uniform1i(mWindowWidth, "Width");
	shader->uniform1i(mWindowHeight, "Height");
	shader->uniform1i(mScene->mFrameIndex % 8, "frameIndexMod8");
	shader->uniform4x4f(mScene->mCamera->getInvViewMatrix(), "cameraInvView");
	shader->uniform1i(mNumRays, "NumRays");
	shader->uniform1i(mNumSteps, "NumSteps");
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	int slot = 0;
	glBindTextureUnit(slot, getTexture("HiZ", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "HiZ");

	slot += 1;
	glBindTextureUnit(slot, getTexture("RMLD", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "RMLD");

	slot += 1;
	glBindTextureUnit(slot, getTexture("normal", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "normal");

	slot += 1;
	glBindTextureUnit(slot, getTexture("SSGIInput", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "SSGIInput");

	slot += 1;
	glBindTextureUnit(slot, getTexture("albedo", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "albedo");
#else
	int slot = 0;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("HiZ", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "HiZ");

	slot += 1;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("RMLD", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "RMLD");

	slot += 1;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("normal", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "normal");

	slot += 1;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("SSGIInput", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "SSGIInput");

	slot += 1;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("albedo", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "albedo");
#endif
	mesh->draw();
	mFrameBuffer->unbindFBO();
	mLightUBO->unbind();
	mCameraUBO->unbind();
	shader->end();
}

void FMergeResultRenderer::initialize() {
	mFrameBuffer = std::make_shared<FFrameBuffer>();

	std::shared_ptr<FTexture> output = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> outputDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::RGBA16F), mWindowWidth, mWindowHeight);
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	output->applyResource(outputDesc);
#else
	output->allocateResource(0, outputDesc);
#endif
	addTexture("output", ETextureUsage::RenderTarget, output);

	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, output, 0);
}

void FMergeResultRenderer::render() {
	if (!mScene) return;
	std::shared_ptr<FShader> shader = mShaders["MergeResultRenderer"];
	shader->use();
	setRenderState();
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->uploadColorAttachment();
	glViewport(0, 0, mWindowWidth, mWindowHeight);
	glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	shader->uniform1i(true ,"highlightedSSR");
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	int slot = 0;
	glBindTextureUnit(slot, getTexture("DirectLight", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "DirectLight");

	slot += 1;
	glBindTextureUnit(slot, getTexture("SSR", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "SSR");

	slot += 1;
	glBindTextureUnit(slot, getTexture("GI", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "GI");
#else
	int slot = 0;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("DirectLight", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "DirectLight");

	slot += 1;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("SSR", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "SSR");

	slot += 1;
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, getTexture("GI", ETextureUsage::ShaderResource)->getHandle());
	shader->uniform1i(slot, "GI");
#endif
	auto quadMesh = mScene->mScreenQuadMesh;
	quadMesh->draw();
	mFrameBuffer->unbindFBO();
	shader->end();
}