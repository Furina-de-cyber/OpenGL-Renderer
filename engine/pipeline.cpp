#include "pipeline.h"

FPipeline::~FPipeline() {

}

void buildPipeline(std::shared_ptr<FPipeline>& outPipeline, 
	std::shared_ptr<FScene> inScene, EPipelineType type) {
	switch (type)
	{
	case EPipelineType::Debug:
		buildDebugPipeline(outPipeline, inScene);
		break;
	case EPipelineType::Deferred:
		break;
	case EPipelineType::Forward:
		break;
	default:
		break;
	}
}

void buildDebugPipeline(std::shared_ptr<FPipeline>& outPipeline, std::shared_ptr<FScene> inScene) {
	inScene->createScreenQuad();

	std::shared_ptr<FTexture>historyTAAResult = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> historyTAAResultDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::RGBA16F), inScene->mWindowWidth, inScene->mWindowHeight);
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	historyTAAResult->applyResource(historyTAAResultDesc);
#else
	historyTAAResult->allocateResource(0, historyTAAResultDesc);
#endif

	std::shared_ptr<FTexture>shadowFilterHistoryTAAResult = std::make_shared<FTexture>();
	std::shared_ptr<FTextureDesc> shadowFilterHistoryTAAResultDesc = std::make_shared<FTextureDesc>(
		GetGLFormat(TextureFormat::R16F), inScene->mWindowWidth, inScene->mWindowHeight);
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	shadowFilterHistoryTAAResult->applyResource(shadowFilterHistoryTAAResultDesc);
#else
	shadowFilterHistoryTAAResult->allocateResource(0, shadowFilterHistoryTAAResultDesc);
#endif

	outPipeline = std::make_shared<FDebugPipeline>(inScene, historyTAAResult, shadowFilterHistoryTAAResult);

	std::shared_ptr<FDynamicDiffuseProbe> ddgiProbe = std::make_shared<FDynamicDiffuseProbe>(
		inScene->mProbeCenter, inScene->mProbeSize
	);
	ddgiProbe->makeDynamicDiffuseProbe();
	ddgiProbe->makeSSBO();
	std::shared_ptr<FRenderer> gDDGIProbePass = std::make_shared<FDynamicDiffuseProbeRenderer>(ddgiProbe);
	gDDGIProbePass->setViewPort(ddgiProbe->getTextureSize(), ddgiProbe->getTextureSize());
	gDDGIProbePass->setScene(inScene);
	gDDGIProbePass->addShader("DDProbeShader",
		std::make_shared<FShader>(
			"./assets/shaders/screenTest/RT/DDProbe.comp"
		)
	);
	gDDGIProbePass->addSSBO("Probe", ddgiProbe->getProbeSSBO());
	gDDGIProbePass->initialize();
	outPipeline->addRenderPass(gDDGIProbePass);

	std::shared_ptr<FRenderer> gGBufferPass = std::make_shared<FGBufferRenderer>();
	gGBufferPass->setViewPort(inScene->mWindowWidth, inScene->mWindowHeight);
	gGBufferPass->setScene(inScene);
	gGBufferPass->addShader("GBufferShader",
		std::make_shared<FShader>(
			"./assets/shaders/screenTest/GBuffer.vert",
			"./assets/shaders/screenTest/GBuffer.frag"
		)
	);
	gGBufferPass->initialize();
	outPipeline->addRenderPass(gGBufferPass);

	std::shared_ptr<FRenderer> gDDGIPass = std::make_shared<FDynamicDiffuseGlobalIlluminationRenderer>(ddgiProbe);
	gDDGIPass->setViewPort(inScene->mWindowWidth, inScene->mWindowHeight);
	gDDGIPass->setScene(inScene);
	gDDGIPass->addShader("DDGIShader",
		std::make_shared<FShader>(
			"./assets/shaders/screenTest/RT/DDGI.vert",
			"./assets/shaders/screenTest/RT/DDGI.frag"
		)
	);
	gDDGIPass->addTexture("GBuffer_Albedo", ETextureUsage::ShaderResource,
		gGBufferPass->getTexture("GBuffer_Albedo", ETextureUsage::RenderTarget));
	gDDGIPass->addTexture("GBuffer_RoughnessMetallicLinearDepth", ETextureUsage::ShaderResource,
		gGBufferPass->getTexture("GBuffer_RoughnessMetallicLinearDepth", ETextureUsage::RenderTarget));
	gDDGIPass->addTexture("GBuffer_Normal", ETextureUsage::ShaderResource,
		gGBufferPass->getTexture("GBuffer_Normal", ETextureUsage::RenderTarget));
	gDDGIPass->initialize();
	outPipeline->addRenderPass(gDDGIPass);

	std::shared_ptr<FRenderer> gHiZPass = std::make_shared<FRefineHiZRenderer>();
	gHiZPass->setViewPort(inScene->mWindowWidth, inScene->mWindowHeight);
	gHiZPass->setScene(inScene);
	gHiZPass->addShader("HiZShader",
		std::make_shared<FShader>(
			"./assets/shaders/screenTest/HiZ.comp"
		)
	);
	gHiZPass->initialize();
	gHiZPass->addTexture("HiZ", ETextureUsage::UnorderedAccess,
		gGBufferPass->getTexture("GBuffer_DeviceZ", ETextureUsage::RenderTarget));
	outPipeline->addRenderPass(gHiZPass);

	std::shared_ptr<FRenderer> gHorizonalPass = std::make_shared<FHorizonalDataRenderer>();
	gHorizonalPass->setViewPort(inScene->mWindowWidth, inScene->mWindowHeight);
	gHorizonalPass->setScene(inScene);
	gHorizonalPass->addShader("HorizonalDataShader",
		std::make_shared<FShader>(
			"./assets/shaders/screenTest/horizonalData.comp"
		)
	);
	gHorizonalPass->initialize();
	gHorizonalPass->addTexture("HiZ", ETextureUsage::UnorderedAccess,
		gHiZPass->getTexture("HiZ", ETextureUsage::UnorderedAccess));
	outPipeline->addRenderPass(gHorizonalPass);

	const float nearBias = 0.01f;
	const float far = 7.5f;
	std::shared_ptr<FShadowTerm> shadowTerm = std::make_shared<FShadowTerm>(inScene->mMainLight, nearBias, far);
	std::shared_ptr<FRenderer> gShadowProjPass = std::make_shared<FShadowProjectionRenderer>(shadowTerm);
	gShadowProjPass->setViewPort(shadowTerm->mShadowMapWidth, shadowTerm->mShadowMapHeight);
	gShadowProjPass->setScene(inScene);
	gShadowProjPass->addShader("ShadowMapProjShader",
		std::make_shared<FShader>(
			"./assets/shaders/screenTest/shadowProj.vert",
			"./assets/shaders/screenTest/shadowProj.frag"
		)
	);
	gShadowProjPass->initialize();
	outPipeline->addRenderPass(gShadowProjPass);

	std::shared_ptr<FRenderer> gShadowPass = std::make_shared<FShadowRenderer>(shadowTerm);
	gShadowPass->setViewPort(inScene->mWindowWidth, inScene->mWindowHeight);
	gShadowPass->setScene(inScene);
	gShadowPass->addShader("ShadowShader",
		std::make_shared<FShader>(
			"./assets/shaders/screenTest/shadow.vert",
			"./assets/shaders/screenTest/shadow.frag"
		)
	);
	gShadowPass->initialize();
	gShadowPass->addTexture("GBuffer_RoughnessMetallicLinearDepth", ETextureUsage::ShaderResource,
		gGBufferPass->getTexture("GBuffer_RoughnessMetallicLinearDepth", ETextureUsage::RenderTarget));
	gShadowPass->addTexture("shadowProjTexture", ETextureUsage::ShaderResource,
		gShadowProjPass->getTexture("Shadow_Map", ETextureUsage::RenderTarget));
	outPipeline->addRenderPass(gShadowPass);

	const int filterRound = 2;
	std::shared_ptr<FRenderer> gShadowFilterPass = std::make_shared<FPingPongFilterRenderer>(filterRound);
	gShadowFilterPass->setViewPort(inScene->mWindowWidth, inScene->mWindowHeight);
	gShadowFilterPass->setScene(inScene);
	gShadowFilterPass->addShader("PingPongFilterShader",
		std::make_shared<FShader>(
			"./assets/shaders/screenTest/pingPongFilter.comp"
		)
	);
	gShadowFilterPass->initialize();
	gShadowFilterPass->addTexture("PingPong", ETextureUsage::UnorderedAccess,
		gShadowPass->getTexture("ShadowFactor", ETextureUsage::RenderTarget));
	outPipeline->addRenderPass(gShadowFilterPass);

	std::shared_ptr<FRenderer> gShadowTAAPass = std::make_shared<FShadowTemporalAntiAliasingCSRenderer>();
	gShadowTAAPass->setViewPort(inScene->mWindowWidth, inScene->mWindowHeight);
	gShadowTAAPass->setScene(inScene);
	gShadowTAAPass->addShader("ShadowTAAShader",
		std::make_shared<FShader>(
			"./assets/shaders/screenTest/ShadowTemporalFilter.comp"
		)
	);
	gShadowTAAPass->initialize();
	gShadowTAAPass->addTexture("TAAInput", ETextureUsage::UnorderedAccess,
		gShadowFilterPass->getTexture("PingPong", ETextureUsage::UnorderedAccess));
	gShadowTAAPass->addTexture("Depth", ETextureUsage::UnorderedAccess,
		gGBufferPass->getTexture("GBuffer_RoughnessMetallicLinearDepth", ETextureUsage::RenderTarget));
	gShadowTAAPass->addTexture("TAAHistoryInput", ETextureUsage::UnorderedAccess, shadowFilterHistoryTAAResult);
	outPipeline->addRenderPass(gShadowTAAPass);

	std::shared_ptr<FRenderer> gDirectLightPass = std::make_shared<FDirectLightRenderer>();
	gDirectLightPass->setViewPort(inScene->mWindowWidth, inScene->mWindowHeight);
	gDirectLightPass->setScene(inScene);
	gDirectLightPass->addShader("DirectLightShader",
		std::make_shared<FShader>(
			"./assets/shaders/screenTest/directLight.vert",
			"./assets/shaders/screenTest/directLight.frag"
		)
	);
	gDirectLightPass->initialize();
	gDirectLightPass->addTexture("GBuffer_Albedo", ETextureUsage::ShaderResource,
		gGBufferPass->getTexture("GBuffer_Albedo", ETextureUsage::RenderTarget));
	gDirectLightPass->addTexture("GBuffer_RoughnessMetallicLinearDepth", ETextureUsage::ShaderResource,
		gGBufferPass->getTexture("GBuffer_RoughnessMetallicLinearDepth", ETextureUsage::RenderTarget));
	gDirectLightPass->addTexture("GBuffer_Normal", ETextureUsage::ShaderResource,
		gGBufferPass->getTexture("GBuffer_Normal", ETextureUsage::RenderTarget));
	gDirectLightPass->addTexture("ShadowFactor", ETextureUsage::ShaderResource,
		gShadowTAAPass->getTexture("TAAResult", ETextureUsage::UnorderedAccess));
	outPipeline->addRenderPass(gDirectLightPass);

	std::shared_ptr<FRenderer> gSSRPass = std::make_shared<FScreenSpaceReflectionRenderer>();
	gSSRPass->setViewPort(inScene->mWindowWidth, inScene->mWindowHeight);
	gSSRPass->setScene(inScene);
	gSSRPass->addShader("SSRShader",
		std::make_shared<FShader>(
			"./assets/shaders/screenTest/SSRT/SSR.vert",
			"./assets/shaders/screenTest/SSRT/SSR.frag"
		)
	);
	gSSRPass->initialize();
	gSSRPass->addTexture("HiZ", ETextureUsage::ShaderResource,
		gHiZPass->getTexture("HiZ", ETextureUsage::UnorderedAccess));
	gSSRPass->addTexture("RMLD", ETextureUsage::ShaderResource,
		gGBufferPass->getTexture("GBuffer_RoughnessMetallicLinearDepth", ETextureUsage::RenderTarget));
	gSSRPass->addTexture("normal", ETextureUsage::ShaderResource,
		gGBufferPass->getTexture("GBuffer_Normal", ETextureUsage::RenderTarget));
	gSSRPass->addTexture("SSRInput", ETextureUsage::ShaderResource,
		gDirectLightPass->getTexture("DirectLight", ETextureUsage::RenderTarget));
	gSSRPass->addSSBO("horizonalData", gHorizonalPass->getSSBO("horizonalData"));
	outPipeline->addRenderPass(gSSRPass);

	std::shared_ptr<FRenderer> gMergePass = std::make_shared<FMergeResultRenderer>();
	gMergePass->setViewPort(inScene->mWindowWidth, inScene->mWindowHeight);
	gMergePass->setScene(inScene);
	gMergePass->addShader("MergeResultRenderer",
		std::make_shared<FShader>(
			"./assets/shaders/screenTest/SSRT/merge.vert",
			"./assets/shaders/screenTest/SSRT/merge.frag"
		)
	);
	gMergePass->initialize();
	gMergePass->addTexture("DirectLight", ETextureUsage::ShaderResource,
		gDirectLightPass->getTexture("DirectLight", ETextureUsage::RenderTarget));
	gMergePass->addTexture("SSR", ETextureUsage::ShaderResource,
		gSSRPass->getTexture("SSRResult", ETextureUsage::RenderTarget));
	gMergePass->addTexture("GI", ETextureUsage::ShaderResource,
	//	gSSGIPass->getTexture("SSGIResult", ETextureUsage::RenderTarget));
		gDDGIPass->getTexture("DDGIResult", ETextureUsage::RenderTarget));
	outPipeline->addRenderPass(gMergePass);

	std::shared_ptr<FRenderer> gTAAPass = std::make_shared<FTemporalAntiAliasingCSRenderer>();
	gTAAPass->setViewPort(inScene->mWindowWidth, inScene->mWindowHeight);
	gTAAPass->setScene(inScene);
	gTAAPass->addShader("TAAShader",
		std::make_shared<FShader>(
			"./assets/shaders/screenTest/TAA.comp"
		)
	);
	gTAAPass->initialize();
	gTAAPass->addTexture("TAAInput", ETextureUsage::UnorderedAccess,
		//gDirectLightPass->getTexture("DirectLight", ETextureUsage::RenderTarget));
		gMergePass->getTexture("output", ETextureUsage::RenderTarget));
	gTAAPass->addTexture("Depth", ETextureUsage::UnorderedAccess,
		gGBufferPass->getTexture("GBuffer_RoughnessMetallicLinearDepth", ETextureUsage::RenderTarget));
	gTAAPass->addTexture("TAAHistoryInput", ETextureUsage::UnorderedAccess, historyTAAResult);
	outPipeline->addRenderPass(gTAAPass);

	std::shared_ptr<FRenderer> gScreenPass = std::make_shared<FScreenSpaceQuadRenderer>();
	gScreenPass->setViewPort(inScene->mWindowWidth, inScene->mWindowHeight);
	gScreenPass->setScene(inScene);
	gScreenPass->addShader("ScreenSpaceQuadShader",
		std::make_shared<FShader>(
			"./assets/shaders/screenTest/screen.vert",
			"./assets/shaders/screenTest/screen.frag"
		)
	);
	gScreenPass->initialize();
	gScreenPass->addTexture("BackBufferTexture", ETextureUsage::ShaderResource,
		gTAAPass->getTexture("TAAResult", ETextureUsage::UnorderedAccess));
	outPipeline->addRenderPass(gScreenPass);
}

void buildIBLGeneratePipeline(
	std::shared_ptr<FPipeline>& outPipeline, 
	std::shared_ptr<FScene> inScene,
	const std::vector<std::string>& path,
	const std::string& outPath,
	bool bHDR
) {
	inScene->createScreenQuad();
	outPipeline = std::make_shared<FIBLGeneratePipeline>(inScene);
	std::shared_ptr<FRenderer> gIBLPass = std::make_shared<FIBLGenerateRenderer>(outPath);
	gIBLPass->setViewPort(inScene->mWindowWidth, inScene->mWindowHeight);
	gIBLPass->setScene(inScene);
	gIBLPass->addShader("IBLGenerateShader",
		std::make_shared<FShader>(
			"./assets/shaders/preCompute/ambient.vert",
			"./assets/shaders/preCompute/ambient.frag"
		)
	);

	std::shared_ptr<FTexture> inTexture = std::make_shared<FTexture>(GL_TEXTURE_CUBE_MAP);
#if OPEN_DSA_AND_BINDLESS_TEXTURE
	if (!bHDR) {
		std::shared_ptr<FTextureDesc> inTextureDesc = std::make_shared<FTextureDesc>(
			GetGLFormat(TextureFormat::RGBA8), inScene->mWindowWidth, inScene->mWindowHeight, true);
		inTexture->applyResource(inTextureDesc);
		inTexture->allocateResourceCubemap(path, true);
	}
	else {
		std::shared_ptr<FTextureDesc> inTextureDesc = std::make_shared<FTextureDesc>(
			GetGLFormat(TextureFormat::RGBA32F), inScene->mWindowWidth, inScene->mWindowHeight, true);
		inTexture->applyResource(inTextureDesc);
		inTexture->allocateResourceCubemapHDR(path, true);
	}
#else
	if (!bHDR) {
		std::shared_ptr<FTextureDesc> inTextureDesc = std::make_shared<FTextureDesc>(
			GetGLFormat(TextureFormat::RGBA8), inScene->mWindowWidth, inScene->mWindowHeight, true);
		inTexture->allocateResourceCubemap(0, inTextureDesc, path);
	}
	else {
		std::shared_ptr<FTextureDesc> inTextureDesc = std::make_shared<FTextureDesc>(
			GetGLFormat(TextureFormat::RGBA32F), inScene->mWindowWidth, inScene->mWindowHeight, true);
		inTexture->allocateResourceCubemapHDR(0, inTextureDesc, path);
	}
#endif
	gIBLPass->addTexture("resource", ETextureUsage::ShaderResource, inTexture);

	gIBLPass->initialize();
	outPipeline->addRenderPass(gIBLPass);

	std::shared_ptr<FRenderer> gEnvBRDFPass = std::make_shared<FEnvBRDFRenderer>(outPath);
	gEnvBRDFPass->setViewPort(inScene->mWindowWidth, inScene->mWindowHeight);
	gEnvBRDFPass->setScene(inScene);
	gEnvBRDFPass->addShader("EnvBRDFShader",
		std::make_shared<FShader>(
			"./assets/shaders/preCompute/envBRDF.vert",
			"./assets/shaders/preCompute/envBRDF.frag"
		)
	);
	gEnvBRDFPass->initialize();
	outPipeline->addRenderPass(gEnvBRDFPass);
}

void FDebugPipeline::update(std::shared_ptr<FScene> inScene) {
	FCameraInfo cameraUBOdata = getCameraInfo(mScene->mCamera);
	inScene->mCameraUBO->updateData(cameraUBOdata, 0, true);
	inScene->mLightUBO->updateData(*(mScene->mMainLight), 0, true);
	inScene->mFrameIndex = (inScene->mFrameIndex + 1) % 1024;
}