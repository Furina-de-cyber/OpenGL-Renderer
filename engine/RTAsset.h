#pragma once

#include "../core.h"
#include "buffer.h"
#include "texture.h"

struct FAABB {
	explicit FAABB(glm::vec3 inMin, glm::vec3 inMax) : mMin{ inMin }, mMax{ inMax } {};
	explicit FAABB(glm::vec3 inData) : mMin{ inData }, mMax{ inData } {};
	explicit FAABB() = default;
	glm::vec3 mMin = glm::vec3(FLOATINF);
    float pad0 = 0.0f;
	glm::vec3 mMax = glm::vec3(- FLOATINF);
    float pad1 = 0.0f;
	void expand(const FAABB& other);
	void expand(const glm::vec3 inData);
	float halfSurfaceArea() const;
    glm::vec3 diagonal() const;
};

struct FRTTriangleInfo {
	FAABB mTriangleAABB = FAABB();
	glm::vec3 mWorldTriangleCenter = glm::vec3(0.0f);
    float pad = 0.0f;
	FRTTriangleInfo(FAABB inAABB) : mTriangleAABB{ inAABB }, mWorldTriangleCenter{ glm::vec3(0.0f)} {};
	FRTTriangleInfo(FAABB inAABB, glm::vec3 inCenter) : mTriangleAABB{ inAABB }, mWorldTriangleCenter{ inCenter } {};
	const FAABB& aabb() const { return mTriangleAABB; }
	const glm::vec3& center() const { return mWorldTriangleCenter; }
};

struct BvhNode
{
	BvhNode(uint32_t first_child_or_primitive_, uint32_t primitive_count_)
		: first_child_or_primitive{ first_child_or_primitive_ },
          primitive_count{ primitive_count_ }
    {}
	BvhNode() : BvhNode(0, 0) {}
    FAABB mAABB;
    uint32_t first_child_or_primitive;
    uint32_t primitive_count;
    float pad0 = 0.0f, pad1 = 0.0f;
    FAABB& aabb() { return mAABB; }

    bool isLeaf() const { return primitive_count != 0; }
};

struct Bvh {
public:
    std::vector<BvhNode> nodes;
    std::vector<uint32_t> primitive_indices;
    std::vector<std::vector<FAABB>> collect_aabbs_by_depth()
    {
        std::vector<std::vector<FAABB>> aabbs;
        collect_aabbs_by_depth(aabbs, 0, 0);
        return aabbs;
    }
    const uint32_t mNodesSlot = 4;
    const uint32_t mPrimitiveIndicesSlot = 5;
    std::shared_ptr<FShaderStorageBuffer> mNodesBuffer = nullptr;
    std::shared_ptr<FShaderStorageBuffer> mPrimitiveIndicesBuffer = nullptr;
private:
    void collect_aabbs_by_depth(
        std::vector<std::vector<FAABB>>& aabbs,
        size_t depth, size_t node_index);
};

class BvhBuilder
{
public:
    std::shared_ptr<Bvh> build_bvh(const std::vector<FRTTriangleInfo>& primitives)
    {
        std::vector<FAABB> bounding_boxes(primitives.size());
        std::vector<glm::vec3> primitive_centers(primitives.size());
        for (size_t i = 0, n = primitives.size(); i < n; ++i)
        {
            bounding_boxes[i] = primitives[i].aabb();
            primitive_centers[i] = primitives[i].center();
        }
        return build_bvh(primitive_centers, bounding_boxes);
    }

    virtual std::shared_ptr<Bvh> build_bvh(
        const std::vector<glm::vec3>& primitive_centers,
        const std::vector<FAABB>& bounding_boxes) = 0;
};

class BinnedBvhBuilder : public BvhBuilder
{
public:
    using BvhBuilder::build_bvh;

    std::shared_ptr<Bvh> build_bvh(
        const std::vector<glm::vec3>& primitive_centers,
        const std::vector<FAABB>& bounding_boxes) override;

private:
    static constexpr size_t min_primitives_per_leaf = 4;
    static constexpr size_t max_primitives_per_leaf = 8;
    static constexpr size_t bin_count = 16;

    static_assert(min_primitives_per_leaf < max_primitives_per_leaf);

    struct Bin
    {
        FAABB aabb;
        size_t primitive_count = 0;
        float cost = 0.0f;
    };

    void build_bvh_node(
        std::shared_ptr<Bvh> bvh, BvhNode& node_to_build,
        const std::vector<glm::vec3>& primitive_centers,
        const std::vector<FAABB>& bounding_boxes);

    static size_t compute_bin_index(
        int axis, const glm::vec3& center, const FAABB& centers_aabb) noexcept;

    static size_t compute_bin_index_precalc(
        int axis, const glm::vec3& center, const FAABB& centers_aabb, const float precalc) noexcept;

    struct BestSplit
    {
        float min_cost;
        int min_axis;
        size_t min_bin;
    };
    BestSplit find_best_split(
        size_t begin, size_t end,
        const FAABB& node_aabb,
        const std::vector<uint32_t>& primitive_indices,
        const std::vector<glm::vec3>& primitive_centers,
        const std::vector<FAABB>& bounding_boxes) const noexcept;
};

// the local space of the probe is the world space
struct FProbe {
    glm::vec3 mPosition = glm::vec3(0.0f);
    uint32_t mSampleNums = 4;
	FProbe(glm::vec3 inPos, uint32_t inSampleNums) : mPosition{ inPos }, mSampleNums{ inSampleNums } {};
};

class FDynamicDiffuseProbe {
public:
    FDynamicDiffuseProbe() = default;
    FDynamicDiffuseProbe(glm::vec3 inCenter, glm::vec3 inSize):
		mProbeSpaceCenter{ inCenter }, mProbeSpaceSize{ inSize } {
	};
    ~FDynamicDiffuseProbe() = default;

    void makeDynamicDiffuseProbe();
    void makeSSBO();
	uint32_t getTextureSize() const { return mTextureSize; } 
	std::shared_ptr<FShaderStorageBuffer> getProbeSSBO() const { return mProbeSSBO; }
    std::shared_ptr<FShaderStorageBuffer> getIrradianceSSBO() const { return mIrradianceBindlessHandleSSBO; }
    std::shared_ptr<FShaderStorageBuffer> getDepthSSBO() const { return mDepthBindlessHandleSSBO; }
	friend class FDynamicDiffuseProbeRenderer;
private:
	glm::vec3 mProbeSpaceCenter = glm::vec3(0.0f);
	glm::vec3 mProbeSpaceSize = glm::vec3(1.0f, 1.0f, 1.0f);

    const uint32_t mSampleNums = 4;

	const uint32_t mProbeNumsX = 4;
    const uint32_t mProbeNumsY = 4;
    const uint32_t mProbeNumsZ = 4;
	const uint32_t mTotalProbeNums = mProbeNumsX * mProbeNumsY * mProbeNumsZ;

	const uint32_t mTextureSize = 32;

	std::vector<std::shared_ptr<FTexture>> mIrradianceBindlessTextures;
	std::vector<uint64_t> mIrradianceBindlessTextureHandles;
    std::vector<std::shared_ptr<FTexture>> mDepthBindlessTextures;
    std::vector<uint64_t> mDepthBindlessTextureHandles;
	std::vector<FProbe> mProbes;
	std::shared_ptr<FShaderStorageBuffer> mIrradianceBindlessHandleSSBO = nullptr;
	std::shared_ptr<FShaderStorageBuffer> mDepthBindlessHandleSSBO = nullptr;
	std::shared_ptr<FShaderStorageBuffer> mProbeSSBO = nullptr;
};