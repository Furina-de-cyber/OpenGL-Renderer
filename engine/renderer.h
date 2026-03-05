#pragma once

#include "../core.h"
#include "texture.h"
#include "asset.h"
#include "shader.h"
#include "camera/camera.h"
#include "buffer.h"
#include "RTAsset.h"
#include "../tools/image.h"
#include "../wrapper/checkError.h"

static const int ShadowMapSize = 1024;

enum class ETextureUsage {
    ShaderResource,
    RenderTarget,
    DepthStencil,
    UnorderedAccess
};

class FFrameBuffer {
public:
    FFrameBuffer();
    ~FFrameBuffer();

    void bindFBO(GLenum target);
    void unbindFBO();
    void setTarget(GLenum inTarget);

	bool attach(ETextureUsage inUsage, std::shared_ptr<FTexture> inTexture, int attachmentBias = 0);
	void uploadColorAttachment();
    void clearBindList() { mBindList.clear(); }
private:

    GLuint mFBOHandle = 0;
    GLint mMaxColorAttachments = 0;
    GLenum mCurrentTarget = GL_NONE; // GL_FRAMEBUFFER / GL_DRAW_FRAMEBUFFER / GL_READ_FRAMEBUFFER
    std::vector<GLenum> mBindList = {};
	std::vector<std::shared_ptr<FTexture>> mTexture = {};
};

struct FScene {
	std::vector<std::shared_ptr<FAsset>> mAsset = {};
	std::shared_ptr<FMesh> mScreenQuadMesh = nullptr;
	std::shared_ptr<Camera> mCamera;
	glm::mat4 historyCameraView = glm::mat4(1.0f);
	std::shared_ptr<FLight> mMainLight = nullptr;
	std::shared_ptr<FMesh> mMainLightVolume = nullptr;
	const std::unordered_map<std::string, int> mGBufferSlotMap = {
		{"GBuffer_Albedo", 0},
		{"GBuffer_RoughnessMetallicLinearDepth", 1},
		{"GBuffer_Normal", 2}
	};

	int mWindowWidth = 1920;
	int mWindowHeight = 1080;
	float mCameraFar = 100.0f;
	float mCameraNear = 0.1f;
	std::unordered_map<std::string, std::shared_ptr<FTexture>> mLUT = {};

	FScene(int inWidth, int inHeight) : mWindowWidth{ inWidth }, mWindowHeight{ inHeight } {}
	FScene() = default;
	void createScreenQuad();

	std::shared_ptr<FUniformBuffer> mCameraUBO = nullptr;
	std::shared_ptr<FUniformBuffer> mLightUBO = nullptr;

	int mFrameIndex = 0;

	std::shared_ptr<FRTAsset> mRTAsset = nullptr;
	std::shared_ptr<Bvh> mSceneBVH = nullptr;
	std::unique_ptr<BvhBuilder> mBVHBuilder = nullptr;
	void makeRTAsset();
	void makeSceneBVH();

	glm::vec3 mProbeCenter = glm::vec3(0.0f);
	glm::vec3 mProbeSize = glm::vec3(1.0f);
};

class FRenderer {
public:
	FRenderer() = default;
	virtual ~FRenderer() = 0;
	virtual void initialize() = 0;
	virtual void render() = 0;
	virtual void setRenderState();
public:
	void addShader(const std::string& name, std::shared_ptr<FShader> inShader);
	std::shared_ptr<FTexture> getTexture(const std::string& name, ETextureUsage inUsage);
	void addTexture(const std::string& name, ETextureUsage inUsage, std::shared_ptr<FTexture> inTexture);
	void addSSBO(const std::string& name, std::shared_ptr<FShaderStorageBuffer> inSSBO);
	std::shared_ptr<FShaderStorageBuffer> getSSBO(const std::string& name);
	void setScene(std::shared_ptr<FScene> inScene) { mScene = inScene; }
	void setViewPort(const int width, const int height);
protected:
	std::unordered_map<std::string, std::shared_ptr<FShader>> mShaders;
	std::unordered_map<std::string, std::shared_ptr<FTexture>> mShaderResource;
	std::unordered_map<std::string, std::shared_ptr<FTexture>> mRenderTarget;
	std::unordered_map<std::string, std::shared_ptr<FTexture>> mDepthStencil;
	std::unordered_map<std::string, std::shared_ptr<FTexture>> mUnordererAccess;
	std::unordered_map<std::string, std::shared_ptr<FShaderStorageBuffer>> mShaderStorageBuffer;
	std::shared_ptr<FScene> mScene = nullptr;
	std::shared_ptr<FFrameBuffer> mFrameBuffer = nullptr;
	int mWindowWidth = 1920;
	int mWindowHeight = 1080;
	const glm::vec4 mClearColor = glm::vec4(0.5f, 0.0f, 0.5f, 1.0f);
};

class FGBufferRenderer : public FRenderer {
public:
	FGBufferRenderer() = default;
	~FGBufferRenderer() override = default;
	void render() override;
	void initialize() override;
	void setRenderState() override;
private:
	std::shared_ptr<FUniformBuffer> mCameraUBO = nullptr;
	friend class FDirectLightRenderer;
};

class FScreenSpaceQuadRenderer : public FRenderer {
public:
	FScreenSpaceQuadRenderer() = default;
	~FScreenSpaceQuadRenderer() override = default;
	void render() override;
	void initialize() override;
};

class FDirectLightRenderer : public FRenderer {
public:
	FDirectLightRenderer() = default;
	FDirectLightRenderer(std::shared_ptr<FUniformBuffer> mCameraUBO) : mCameraUBO(mCameraUBO) {}
	~FDirectLightRenderer() override = default;
	void render() override;
	void initialize() override;
	void setRenderState() override;
private:
	std::shared_ptr<FUniformBuffer> mCameraUBO = nullptr;
	std::shared_ptr<FUniformBuffer> mLightUBO = nullptr;
};

class FShadowProjectionRenderer : public FRenderer {
public:
	FShadowProjectionRenderer(std::shared_ptr<FShadowTerm> inShadowTerm);
	~FShadowProjectionRenderer() override = default;
	void render() override;
	void initialize() override;
	void setRenderState() override;
private:
	std::shared_ptr<FUniformBuffer> mLightUBO = nullptr;
	std::shared_ptr<FShadowTerm> mShadowTerm = nullptr;
};


class FShadowRenderer : public FRenderer {
public:
	FShadowRenderer(std::shared_ptr<FShadowTerm> inShadowTerm);
	~FShadowRenderer() override = default;
	void render() override;
	void initialize() override;
private:
	std::shared_ptr<FUniformBuffer> mLightUBO = nullptr;
	std::shared_ptr<FShadowTerm> mShadowTerm = nullptr;
	std::shared_ptr<FUniformBuffer> mCameraUBO = nullptr;
};

class FPingPongFilterRenderer : public FRenderer {
public:
	FPingPongFilterRenderer(int inFilterRound) : mFilterRound{ inFilterRound } {}
	~FPingPongFilterRenderer() override = default;
	void render() override;
	void initialize() override;
private:
	int mFilterRound;
	const int mReadonlyBind = 0;
	const int mWriteonlyBind = 1;
};

class FTemporalAntiAliasingRenderer : public FRenderer {
public:
	FTemporalAntiAliasingRenderer() = default;
	~FTemporalAntiAliasingRenderer() override = default;
	void render() override;
	void initialize() override;
private:
	bool mInitialized = false;
};

class FTemporalAntiAliasingCSRenderer : public FRenderer {
public:
	FTemporalAntiAliasingCSRenderer() = default;
	~FTemporalAntiAliasingCSRenderer() override = default;
	void render() override;
	void initialize() override;
private:
	bool mInitialized = false;
};

class FShadowTemporalAntiAliasingCSRenderer : public FRenderer {
public:
	FShadowTemporalAntiAliasingCSRenderer() = default;
	~FShadowTemporalAntiAliasingCSRenderer() override = default;
	void render() override;
	void initialize() override;
private:
	bool mInitialized = false;
};

class FIBLGenerateRenderer : public FRenderer {
public:
	FIBLGenerateRenderer(const std::string& outPath) : mOutPath{ outPath } {};
	~FIBLGenerateRenderer() override = default;
	void render() override;
	void initialize() override;
private:
	std::string mOutPath;
};

class FEnvBRDFRenderer : public FRenderer {
public:
	FEnvBRDFRenderer(const std::string& outPath) : mOutPath{ outPath } {};
	~FEnvBRDFRenderer() override = default;
	void render() override;
	void initialize() override;
private:
	std::string mOutPath;
};

class FHiZRenderer : public FRenderer {
public:
	FHiZRenderer() = default;
	~FHiZRenderer() override = default;
	void render() override;
	void initialize() override;
private:
	const int mHiZLevel = 10;
};

class FRefineHiZRenderer : public FRenderer {
public:
	FRefineHiZRenderer() = default;
	~FRefineHiZRenderer() override = default;
	void render() override;
	void initialize() override;
private:
	const int mHiZLevel = 10;
};

class FExtractChannelRenderer : public FRenderer {
public:
	FExtractChannelRenderer(int inChannelIndex) :mChannelIndex{ inChannelIndex } {}
	~FExtractChannelRenderer() override = default;
	void render() override;
	void initialize() override;
private:
	int mChannelIndex = 0;
};

struct FHorizonalData {
	float maxtan[8];
	float depthMip2;
};

struct FHorizonalDataGPU {
	FHorizonalData data[32400];
};

class FHorizonalDataRenderer : public FRenderer {
public:
	FHorizonalDataRenderer() = default;
	~FHorizonalDataRenderer() override = default;
	void render() override;
	void initialize() override;
};

class FScreenSpaceReflectionRenderer : public FRenderer {
public:
	FScreenSpaceReflectionRenderer() = default;
	FScreenSpaceReflectionRenderer(std::shared_ptr<FUniformBuffer> mCameraUBO) : mCameraUBO(mCameraUBO) {}
	~FScreenSpaceReflectionRenderer() override = default;
	void render() override;
	void initialize() override;
private:
	std::shared_ptr<FUniformBuffer> mCameraUBO = nullptr;
	std::shared_ptr<FUniformBuffer> mLightUBO = nullptr;
	const int mNumSteps = 12;
	const int mNumRays = 12;
};

class FScreenSpaceGlobalIlluminationRenderer : public FRenderer {
public:
	FScreenSpaceGlobalIlluminationRenderer() = default;
	FScreenSpaceGlobalIlluminationRenderer(std::shared_ptr<FUniformBuffer> mCameraUBO) : mCameraUBO(mCameraUBO) {}
	~FScreenSpaceGlobalIlluminationRenderer() override = default;
	void render() override;
	void initialize() override;
private:
	std::shared_ptr<FUniformBuffer> mCameraUBO = nullptr;
	std::shared_ptr<FUniformBuffer> mLightUBO = nullptr;
	const int mNumSteps = 8;
	const int mNumRays = 8;
};

class FMergeResultRenderer : public FRenderer {
public:
	FMergeResultRenderer() = default;
	~FMergeResultRenderer() override = default;
	void render() override;
	void initialize() override;
};

class FRTTestRenderer : public FRenderer {
public:
	FRTTestRenderer() = default;
	~FRTTestRenderer() override = default;
	void render() override;
	void initialize() override;
private:
	std::shared_ptr<FUniformBuffer> mLightUBO = nullptr;
	std::shared_ptr<FUniformBuffer> mCameraUBO = nullptr;
};

class FDynamicDiffuseProbeRenderer : public FRenderer {
public:
	explicit FDynamicDiffuseProbeRenderer(std::shared_ptr<FDynamicDiffuseProbe> inProbe) : mProbe(inProbe) {}
	FDynamicDiffuseProbeRenderer() = delete;
	~FDynamicDiffuseProbeRenderer() override = default;
	void render() override;
	void initialize() override;
private:
	std::shared_ptr<FUniformBuffer> mLightUBO = nullptr;
	std::shared_ptr<FDynamicDiffuseProbe> mProbe = nullptr;
	bool mNeedCalled = true;
};

class FDynamicDiffuseGlobalIlluminationRenderer : public FRenderer {
public:
	explicit FDynamicDiffuseGlobalIlluminationRenderer(std::shared_ptr<FDynamicDiffuseProbe> inProbe) : mProbe(inProbe) {}
	FDynamicDiffuseGlobalIlluminationRenderer() = delete;
	~FDynamicDiffuseGlobalIlluminationRenderer() override = default;
	void render() override;
	void initialize() override;
private:
	std::shared_ptr<FUniformBuffer> mCameraUBO = nullptr;
	std::shared_ptr<FDynamicDiffuseProbe> mProbe = nullptr;
};