#pragma once

#include "../core.h"
#include "renderer.h"
#include "asset.h"
#include "texture.h"

enum class EPipelineType {
	Debug,
	Deferred,
	Forward
};

class FPipeline {
public:
	FPipeline(std::shared_ptr<FScene> inScene) : mScene{ inScene } {}
	virtual ~FPipeline() = 0;

	void addRenderPass(std::shared_ptr<FRenderer> inRenderPass) {
		mRenderPasses.push_back(inRenderPass);
	}
	void initialize() {
		for (const auto& pass : mRenderPasses) {
			pass->initialize();
		}
	}

	void render() {
		for (const auto& pass : mRenderPasses) {
			pass->render();
		}
	}

	virtual void update(std::shared_ptr<FScene> inScene) {

	}

protected:
	std::vector<std::shared_ptr<FRenderer>> mRenderPasses;
	std::shared_ptr<FScene> mScene = nullptr;
};

class FDebugPipeline : public FPipeline {
public:
	FDebugPipeline(
		std::shared_ptr<FScene> inScene, 
		std::shared_ptr<FTexture> inHistoryTAAResult,
		std::shared_ptr<FTexture> inShadowFilterHistoryTAAResult
	)
		: FPipeline{ inScene }, 
		mHistoryTAAResult{ inHistoryTAAResult }, 
		mShadowFilterHistoryTAAResult{ inShadowFilterHistoryTAAResult } {
	}
	~FDebugPipeline() override = default;
	void update(std::shared_ptr<FScene> inScene) override;
private:
	std::shared_ptr<FTexture> mHistoryTAAResult = nullptr;
	std::shared_ptr<FTexture> mShadowFilterHistoryTAAResult = nullptr;
};

class FIBLGeneratePipeline : public FPipeline {
public:
	FIBLGeneratePipeline(std::shared_ptr<FScene> inScene) : FPipeline{ inScene } {}
	~FIBLGeneratePipeline() override = default;
};

void buildPipeline(std::shared_ptr<FPipeline>& outPipeline, std::shared_ptr<FScene> inScene, EPipelineType type);

void buildDebugPipeline(std::shared_ptr<FPipeline>& outPipeline, std::shared_ptr<FScene> inScene);

void buildIBLGeneratePipeline(
	std::shared_ptr<FPipeline>& outPipeline, 
	std::shared_ptr<FScene> inScene, 
	const std::vector<std::string>& path, 
	const std::string& outPath,
	bool bHDR = false);