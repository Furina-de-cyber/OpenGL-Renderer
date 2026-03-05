#include "renderer.h"
// NOTE:
// position is transformed to world space
// other vertex attributes (normal, uv, etc.) are still in local space
// do NOT use them for world-space shading before proper transform
void FScene::makeRTAsset(){
	mRTAsset = std::make_shared<FRTAsset>();
	uint32_t EBOBias = 0;
	uint32_t trianglesNum = 0;
	bool VAODirty = false;
	for (const auto& assetPtr : mAsset) {
		const glm::mat4& assetModelMatrix = assetPtr->getModelMatrix();
		for (const auto& [meshPtr, materialHash] : assetPtr->mMesh) {
			if (!VAODirty) {
				mRTAsset->mUniformVAO = meshPtr->getVertexBufferDesc();
				VAODirty = true;
			}
			else {
				if (!(*mRTAsset->mUniformVAO == *meshPtr->getVertexBufferDesc())) {
					assert(false && "Inconsistent vertex buffer descriptions among meshes.");
					return;
				}
			}
			std::shared_ptr<FMaterial> material = assetPtr->mMaterial[materialHash];
			mRTAsset->mSceneBindlessPool.emplace_back(material);
			const uint32_t materialIndex = mRTAsset->mSceneBindlessPool.size() - 1;
			const std::vector<float>& vboData = meshPtr->getVertexBufferData();
			const glm::mat4& modelMatrix = assetModelMatrix * meshPtr->getModelMatrix();
			const uint32_t stride = meshPtr->getVertexBufferDesc()->mStride;
			const uint32_t vertexCount = vboData.size() / stride;
			for (uint32_t i = 0; i < vertexCount; ++i) {
				uint32_t baseIndex = i * stride;
				glm::vec4 pos(
					vboData[baseIndex + 0],
					vboData[baseIndex + 1],
					vboData[baseIndex + 2],
					1.0f
				);
				pos = modelMatrix * pos;
				mRTAsset->mSceneVBOAll.push_back(pos.x);
				mRTAsset->mSceneVBOAll.push_back(pos.y);
				mRTAsset->mSceneVBOAll.push_back(pos.z);
				for (uint32_t j = 3; j < stride; ++j) {
					mRTAsset->mSceneVBOAll.push_back(vboData[baseIndex + j]);
				}
			}

			const std::vector<uint32_t>& eboData = meshPtr->getElementBufferData();
			const uint32_t trianglePointNum = 3;
			uint32_t elementBufferCount = eboData.size() / trianglePointNum;
			for (uint32_t i = 0; i < elementBufferCount; i++) {
				uint32_t baseIndex = i * trianglePointNum;
				uint32_t p0 = eboData[baseIndex + 0];
				uint32_t p1 = eboData[baseIndex + 1];
				uint32_t p2 = eboData[baseIndex + 2];
				mRTAsset->mSceneEBOALL.push_back(p0 + EBOBias);
				mRTAsset->mSceneEBOALL.push_back(p1 + EBOBias);
				mRTAsset->mSceneEBOALL.push_back(p2 + EBOBias);
				auto transform = [&](uint32_t p) {
					glm::vec4 v(
						vboData[p * stride + 0],
						vboData[p * stride + 1],
						vboData[p * stride + 2],
						1.0f
					);
					v = modelMatrix * v;
					return glm::vec3(v);
					};
				//glm::vec3 point0 = glm::vec3(vboData[p0 * stride + 0], vboData[p0 * stride + 1], vboData[p0 * stride + 2]);
				//glm::vec3 point1 = glm::vec3(vboData[p1 * stride + 0], vboData[p1 * stride + 1], vboData[p1 * stride + 2]);
				//glm::vec3 point2 = glm::vec3(vboData[p2 * stride + 0], vboData[p2 * stride + 1], vboData[p2 * stride + 2]);
				glm::vec3 point0 = transform(p0);
				glm::vec3 point1 = transform(p1);
				glm::vec3 point2 = transform(p2);
				glm::vec3 center = (point0 + point1 + point2) / 3.0f;
				FAABB AABB = FAABB(point0);
				AABB.expand(point1);
				AABB.expand(point2);
				mRTAsset->mSceneTriangleInfo.emplace_back(AABB, center);
				mRTAsset->mSceneBindlessPoolIndex.push_back(materialIndex);
			}
			EBOBias += vertexCount;
			trianglesNum += elementBufferCount;
		}
	}
	mRTAsset->mTrianglesNum = trianglesNum;
	assert(trianglesNum == mRTAsset->mSceneTriangleInfo.size());
	assert(trianglesNum == mRTAsset->mSceneBindlessPoolIndex.size());

	mRTAsset->mSceneVBOBuffer = std::make_shared<FShaderStorageBuffer>("VertexInfo");
	mRTAsset->mSceneVBOBuffer->initBuffer(mRTAsset->mSceneVBOAll.size() * sizeof(float), mRTAsset->mSceneVBOBindingSlot, GL_STATIC_DRAW);
	mRTAsset->mSceneVBOBuffer->setData(mRTAsset->mSceneVBOAll.data(), mRTAsset->mSceneVBOAll.size());

	mRTAsset->mSceneEBOBuffer = std::make_shared<FShaderStorageBuffer>("ElementInfo");
	mRTAsset->mSceneEBOBuffer->initBuffer(mRTAsset->mSceneEBOALL.size() * sizeof(uint32_t), mRTAsset->mSceneEBOBindingSlot, GL_STATIC_DRAW);
	mRTAsset->mSceneEBOBuffer->setData(mRTAsset->mSceneEBOALL.data(), mRTAsset->mSceneEBOALL.size());

	mRTAsset->mSceneBindlessPoolIndexBuffer = std::make_shared<FShaderStorageBuffer>("TexIndexInfo");
	mRTAsset->mSceneBindlessPoolIndexBuffer->initBuffer(mRTAsset->mSceneBindlessPoolIndex.size() * sizeof(uint32_t), mRTAsset->mSceneBindlessPoolIndexBindingSlot, GL_STATIC_DRAW);
	mRTAsset->mSceneBindlessPoolIndexBuffer->setData(mRTAsset->mSceneBindlessPoolIndex.data(), mRTAsset->mSceneBindlessPoolIndex.size());

	mRTAsset->mSceneBindlessPoolInfoBuffer = std::make_shared<FShaderStorageBuffer>("TexPoolInfo");
	mRTAsset->mSceneBindlessPoolInfoBuffer->initBuffer(mRTAsset->mSceneBindlessPool.size() * sizeof(FBindlessTextureSlot), mRTAsset->mSceneBindlessPoolInfoBindingSlot, GL_STATIC_DRAW);
	mRTAsset->mSceneBindlessPoolInfoBuffer->setData(mRTAsset->mSceneBindlessPool.data(), mRTAsset->mSceneBindlessPool.size());
}

void FScene::makeSceneBVH() {
	assert(mSceneBVH);

	mSceneBVH->mNodesBuffer = std::make_shared<FShaderStorageBuffer>("nodes");
	mSceneBVH->mNodesBuffer->initBuffer(mSceneBVH->nodes.size() * sizeof(BvhNode), mSceneBVH->mNodesSlot, GL_STATIC_DRAW);
	mSceneBVH->mNodesBuffer->setData(mSceneBVH->nodes.data(), mSceneBVH->nodes.size());

	mSceneBVH->mPrimitiveIndicesBuffer = std::make_shared<FShaderStorageBuffer>("indices");
	mSceneBVH->mPrimitiveIndicesBuffer->initBuffer(mSceneBVH->primitive_indices.size() * sizeof(uint32_t), mSceneBVH->mPrimitiveIndicesSlot, GL_STATIC_DRAW);
	mSceneBVH->mPrimitiveIndicesBuffer->setData(mSceneBVH->primitive_indices.data(), mSceneBVH->primitive_indices.size());
}

void FRTTestRenderer::initialize() {
	std::shared_ptr<FTexture> output = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> outputDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::RGBA16F), mWindowWidth, mWindowHeight, true);
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	output->applyResource(outputDesc);
#else
	output->allocateResource(0, outputDesc);
#endif
	addTexture("output", ETextureUsage::UnorderedAccess, output);
	//checkTextureStatus(output->getHandle());
	mCameraUBO = mScene->mCameraUBO;
	mLightUBO = mScene->mLightUBO;
	mCameraUBO->bindToProgram(mShaders["RTTestShader"]->getProgramHandle());
	mLightUBO->bindToProgram(mShaders["RTTestShader"]->getProgramHandle());
}

void FRTTestRenderer::render() {
	if (!mScene) return;
	std::shared_ptr<FShader> shader = mShaders["RTTestShader"];
	shader->use();
	shader->uniform2f(glm::vec2(static_cast<float>(mWindowWidth), static_cast<float>(mWindowHeight)), "viewPortSize");
	const int localSizeX = 16, localSizeY = 16;
	const int threadGroupNumX = (mWindowWidth + localSizeX - 1) / localSizeX;
	const int threadGroupNumY = (mWindowHeight + localSizeY - 1) / localSizeY;
	const int threadGroupNumZ = 1;

	shader->uniform1f(mScene->mCamera->getFovy() / 2.0f, "hfov");
	shader->uniform1f(mScene->mCamera->getAspect(), "aspect");
	mScene->mRTAsset->mSceneVBOBuffer->bindBufferBase();
	mScene->mRTAsset->mSceneEBOBuffer->bindBufferBase();
	mScene->mRTAsset->mSceneBindlessPoolIndexBuffer->bindBufferBase();
	mScene->mRTAsset->mSceneBindlessPoolInfoBuffer->bindBufferBase();
	mScene->mSceneBVH->mNodesBuffer->bindBufferBase();
	mScene->mSceneBVH->mPrimitiveIndicesBuffer->bindBufferBase();
	glBindImageTexture(6, getTexture("output", ETextureUsage::UnorderedAccess)->getHandle(),
		0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	mCameraUBO->bind();
	mLightUBO->bind();
	glDispatchCompute(threadGroupNumX, threadGroupNumY, threadGroupNumZ);
	glMemoryBarrier(
		GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
		GL_SHADER_STORAGE_BARRIER_BIT |
		GL_TEXTURE_FETCH_BARRIER_BIT
	);
	shader->end();
	mCameraUBO->unbind();
	mLightUBO->unbind();
}

void FDynamicDiffuseProbeRenderer::initialize() {
	mLightUBO = mScene->mLightUBO;
	mLightUBO->bindToProgram(mShaders["DDProbeShader"]->getProgramHandle());
}

void FDynamicDiffuseProbeRenderer::render() {
	if (!mNeedCalled) return;
	else {
		mNeedCalled = false;
	}
	if (!mScene) return;
	std::shared_ptr<FShader> shader = mShaders["DDProbeShader"];
	shader->use();
	mLightUBO->bind();
	
	const int localSizeX = 16, localSizeY = 16;
	const int threadGroupNumX = (mProbe->mTextureSize + localSizeX - 1) / localSizeX;
	const int threadGroupNumY = (mProbe->mTextureSize + localSizeY - 1) / localSizeY;
	const int threadGroupNumZ = 1;
	const glm::ivec2 tileBias[4] = {
		glm::ivec2(0, 0),
		glm::ivec2(0, 16),
		glm::ivec2(16, 0),
		glm::ivec2(16, 16)
	}; // mProbe->mTextureSize = 32, local size = 16, so each probe occupies a 2x2 tile in the texture

	mScene->mRTAsset->mSceneVBOBuffer->bindBufferBase();
	mScene->mRTAsset->mSceneEBOBuffer->bindBufferBase();
	mScene->mRTAsset->mSceneBindlessPoolIndexBuffer->bindBufferBase();
	mScene->mRTAsset->mSceneBindlessPoolInfoBuffer->bindBufferBase();
	mScene->mSceneBVH->mNodesBuffer->bindBufferBase();
	mScene->mSceneBVH->mPrimitiveIndicesBuffer->bindBufferBase();
	getSSBO("Probe")->bindBufferBase();
	shader->uniform2f(glm::vec2(static_cast<float>(mProbe->mTextureSize), static_cast<float>(mProbe->mTextureSize)), "viewPortSize");
	
	for (const auto& tile : tileBias) {
		shader->uniform1i(tile.x, "xBias");
		shader->uniform1f(tile.y, "yBias");
		for (int x = 0; x < mProbe->mProbeNumsX; x++) {
			for (int y = 0; y < mProbe->mProbeNumsY; y++) {
				for (int z = 0; z < mProbe->mProbeNumsZ; z++) {
					int probeIndex = x * mProbe->mProbeNumsY * mProbe->mProbeNumsZ + y * mProbe->mProbeNumsZ + z;
					shader->uniform1i(probeIndex, "probeIndex");
					glBindImageTexture(0, mProbe->mIrradianceBindlessTextures[probeIndex]->getHandle(),
						0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
					glBindImageTexture(1, mProbe->mDepthBindlessTextures[probeIndex]->getHandle(),
						0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG16F);
					glDispatchCompute(threadGroupNumX, threadGroupNumY, threadGroupNumZ);
					glMemoryBarrier(
						GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
						GL_SHADER_STORAGE_BARRIER_BIT |
						GL_TEXTURE_FETCH_BARRIER_BIT
					);
				}
			}
		}
	}

	shader->end();
	mLightUBO->unbind();
}

void FDynamicDiffuseGlobalIlluminationRenderer::initialize() {
	mFrameBuffer = std::make_shared<FFrameBuffer>();

	std::shared_ptr<FTexture> DDGIResult = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> DDGIResultDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::RGBA16F), mWindowWidth, mWindowHeight);
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	DDGIResult->applyResource(DDGIResultDesc);
#else
	DDGIResult->allocateResource(0, DDGIResultDesc);
#endif
	addTexture("DDGIResult", ETextureUsage::RenderTarget, DDGIResult);
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->attach(ETextureUsage::RenderTarget, DDGIResult, 0);
	mFrameBuffer->unbindFBO();
	mCameraUBO = mScene->mCameraUBO;
	mCameraUBO->bindToProgram(mShaders["DDGIShader"]->getProgramHandle());
}

void FDynamicDiffuseGlobalIlluminationRenderer::render() {
	if (!mScene) return;
	auto shader = mShaders["DDGIShader"];
	shader->use();
	setRenderState();
	mFrameBuffer->bindFBO(GL_FRAMEBUFFER);
	mFrameBuffer->uploadColorAttachment();
	glViewport(0, 0, mWindowWidth, mWindowHeight);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	int slot = 0;
	mCameraUBO->bind();
	auto mesh = mScene->mScreenQuadMesh;
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
#endif

	shader->uniform4x4f(glm::inverse(mScene->mCamera->getViewMatrix()), "cameraInvView");
	shader->uniform3f(mScene->mProbeSize, "probeMatrixSize");
	shader->uniform3f(mScene->mProbeCenter, "probeMatrixCenter");
	shader->uniform3f(glm::vec3(4.0f, 4.0f, 4.0f), "probeGridSize");
	shader->uniform1i(mScene->mFrameIndex % 8, "frameIndexMod8");

	mProbe->getDepthSSBO()->bindBufferBase();
	mProbe->getIrradianceSSBO()->bindBufferBase();
	mProbe->getProbeSSBO()->bindBufferBase();

	mesh->draw();

	shader->end();
	mFrameBuffer->unbindFBO();
	mCameraUBO->unbind();
}