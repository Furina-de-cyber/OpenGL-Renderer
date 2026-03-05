#include "RTAsset.h"

void FAABB::expand(const FAABB& other) {
	mMin = glm::min(mMin, other.mMin);
	mMax = glm::max(mMax, other.mMax);
}

void FAABB::expand(const glm::vec3 inData) {
	mMin = glm::min(mMin, inData);
	mMax = glm::max(mMax, inData);
}

glm::vec3 FAABB::diagonal() const {
	return mMax - mMin;
}

float FAABB::halfSurfaceArea() const
{
	glm::vec3 lengthSide = mMax - mMin;
	return lengthSide.x * (lengthSide.y + lengthSide.z) + lengthSide.y * lengthSide.z;
}

void Bvh::collect_aabbs_by_depth(
    std::vector<std::vector<FAABB>>& aabbs,
    size_t depth, size_t node_index)
{
    auto& node = nodes[node_index];
    if (depth >= aabbs.size()) aabbs.resize(depth + 1);
    aabbs[depth].push_back(node.aabb());
    if (!node.isLeaf())
    {
        collect_aabbs_by_depth(aabbs, depth + 1, node.first_child_or_primitive + 0);
        collect_aabbs_by_depth(aabbs, depth + 1, node.first_child_or_primitive + 1);
    }
}

std::shared_ptr<Bvh> BinnedBvhBuilder::build_bvh(const std::vector<glm::vec3>& primitive_centers,
    const std::vector<FAABB>& bounding_boxes)
{
    assert(primitive_centers.size() == bounding_boxes.size());
    size_t primitive_count = primitive_centers.size();
    assert(primitive_count != 0);

    std::shared_ptr<Bvh> bvh = std::make_shared<Bvh>();
    bvh->nodes.reserve(2 * primitive_count - 1);
    bvh->primitive_indices.resize(primitive_count);
    std::iota(bvh->primitive_indices.begin(), bvh->primitive_indices.end(), 0);

    bvh->nodes.push_back(BvhNode(0, static_cast<uint32_t>(primitive_count)));

    build_bvh_node(bvh, bvh->nodes.front(), primitive_centers, bounding_boxes);
    bvh->nodes.shrink_to_fit();
    return bvh;
}

size_t BinnedBvhBuilder::compute_bin_index(int axis, const glm::vec3& center,
    const FAABB& aabb) noexcept
{
    //int index = static_cast<int>((center[axis] - aabb.mMin[axis]) * (center[axis] - aabb.mMin[axis]) *
    //    (static_cast<float>(bin_count) / aabb.diagonal()[axis]));
    int index = static_cast<int>((center[axis] - aabb.mMin[axis]) *
        (static_cast<float>(bin_count) / aabb.diagonal()[axis]));
    return std::min(int{ bin_count - 1 }, std::max(0, index));
}

size_t BinnedBvhBuilder::compute_bin_index_precalc(int axis, const glm::vec3& center,
    const FAABB& aabb, const float precalc) noexcept
{
    int index = static_cast<int>((center[axis] - aabb.mMin[axis]) * (precalc));
    return std::min(int{ bin_count - 1 }, std::max(0, index));
}

BinnedBvhBuilder::BestSplit BinnedBvhBuilder::find_best_split(
    size_t begin, size_t end, const FAABB& node_aabb, const std::vector<uint32_t>& primitive_indices,
    const std::vector<glm::vec3>& primitive_centers,
    const std::vector<FAABB>& bounding_boxes) const noexcept
{
    float min_cost = std::numeric_limits<float>::max();
    size_t min_bin = 0;
    int min_axis = -1;

    float precalc;

    for (int axis = 0; axis < 3; ++axis)
    {
        if (node_aabb.diagonal()[axis] < 1e-6f)
            continue;
        Bin bins[bin_count];

        precalc = (static_cast<float>(bin_count) / node_aabb.diagonal()[axis]);

        for (size_t i = begin; i < end; ++i)
        {
            const glm::vec3& primitive_center = primitive_centers[primitive_indices[i]];
            Bin& bin = bins[compute_bin_index_precalc(axis, primitive_center, node_aabb, precalc)];
            bin.primitive_count++;
            bin.aabb.expand(bounding_boxes[primitive_indices[i]]);
        }
        FAABB left_aabb;
        size_t left_count = 0;
        for (size_t i = 0; i < bin_count; ++i)
        {
            left_aabb.expand(bins[i].aabb);
            left_count += bins[i].primitive_count;
            bins[i].cost = left_aabb.halfSurfaceArea() * left_count;
        }
        FAABB right_aabb;
        size_t right_count = 0;
        for (size_t i = bin_count - 1; i > 0; --i)
        {
            right_aabb.expand(bins[i].aabb);
            right_count += bins[i].primitive_count;
            float cost = right_aabb.halfSurfaceArea() * right_count + bins[i - 1].cost;
            if (cost < min_cost)
            {
                min_cost = cost;
                min_axis = axis;
                min_bin = i;
            }
        }
    }

    assert(min_axis != -1);
    auto best = BestSplit();
    best.min_cost = min_cost;
    best.min_axis = min_axis;
    best.min_bin = min_bin;
    return best;
}

void BinnedBvhBuilder::build_bvh_node(std::shared_ptr<Bvh> bvh, BvhNode& node_to_build,
    const std::vector<glm::vec3>& primitive_centers,
    const std::vector<FAABB>& bounding_boxes)
{
    assert(node_to_build.isLeaf());
    const size_t primitives_begin = node_to_build.first_child_or_primitive;
    const size_t primitives_end = primitives_begin + node_to_build.primitive_count;

    node_to_build.aabb() = FAABB();
    for (size_t i = primitives_begin; i < primitives_end; ++i)
        node_to_build.aabb().expand(bounding_boxes[bvh->primitive_indices[i]]);

    glm::vec3 diag = node_to_build.aabb().diagonal();
    if (diag.x < 1e-6f && diag.y < 1e-6f && diag.z < 1e-6f)
    {
        return;
    }
    if (node_to_build.primitive_count < min_primitives_per_leaf) return;

    auto [min_cost, min_axis, min_bin] =
        find_best_split(primitives_begin, primitives_end, node_to_build.aabb(),
            bvh->primitive_indices, primitive_centers, bounding_boxes);

    float no_split_cost = node_to_build.aabb().halfSurfaceArea() * node_to_build.primitive_count;
    size_t right_partition_begin = 0;
    if (min_cost >= no_split_cost)
    {
        if (node_to_build.primitive_count <= max_primitives_per_leaf) return;
        std::sort(bvh->primitive_indices.begin() + primitives_begin,
            bvh->primitive_indices.begin() + primitives_end,
            [&primitive_centers, min_axis = min_axis](size_t i, size_t j) {
                return primitive_centers[i][min_axis] < primitive_centers[j][min_axis];
            });
        right_partition_begin = primitives_begin + (node_to_build.primitive_count >> 1);
    }
    else
    {
        right_partition_begin =
            std::partition(bvh->primitive_indices.begin() + primitives_begin,
                bvh->primitive_indices.begin() + primitives_end,
                [&primitive_centers, &node_to_build, min_axis = min_axis,
                min_bin = min_bin](size_t i) {
                    size_t bin_index = compute_bin_index(min_axis, primitive_centers[i],
                        node_to_build.aabb());
                    return bin_index < min_bin;
                }) -
            bvh->primitive_indices.begin();
    }
    //assert(right_partition_begin > primitives_begin && right_partition_begin < primitives_end);
    if (right_partition_begin <= primitives_begin || right_partition_begin >= primitives_end)
    {
        return;
    }

    uint32_t first_child_index = static_cast<uint32_t>(bvh->nodes.size());
    auto& left_child = bvh->nodes.emplace_back();
    auto& right_child = bvh->nodes.emplace_back();
    node_to_build.primitive_count = 0;
    node_to_build.first_child_or_primitive = first_child_index;

    left_child.primitive_count = static_cast<uint32_t>(right_partition_begin - primitives_begin);
    left_child.first_child_or_primitive = static_cast<uint32_t>(primitives_begin);
    right_child.primitive_count = static_cast<uint32_t>(primitives_end - right_partition_begin);
    right_child.first_child_or_primitive = static_cast<uint32_t>(right_partition_begin);

    build_bvh_node(bvh, left_child, primitive_centers, bounding_boxes);
    build_bvh_node(bvh, right_child, primitive_centers, bounding_boxes);
}

void FDynamicDiffuseProbe::makeDynamicDiffuseProbe() {
	glm::vec3 probeSpaceMin = mProbeSpaceCenter - mProbeSpaceSize * 0.5f;
	glm::vec3 probeSpaceMax = mProbeSpaceCenter + mProbeSpaceSize * 0.5f;

	glm::vec3 probeSpaceStep = mProbeSpaceSize / (glm::vec3(mProbeNumsX, mProbeNumsY, mProbeNumsZ) - glm::vec3(1.0f));
	
    mIrradianceBindlessTextures.reserve(mTotalProbeNums);
	mIrradianceBindlessTextureHandles.reserve(mTotalProbeNums);
	mDepthBindlessTextures.reserve(mTotalProbeNums);
	mDepthBindlessTextureHandles.reserve(mTotalProbeNums);

    for (int x = 0; x < mProbeNumsX; ++x) {
		for (int y = 0; y < mProbeNumsY; ++y) {
            for(int z = 0; z < mProbeNumsZ; ++z) {
				glm::vec3 probePos = probeSpaceMin + glm::vec3(x, y, z) * probeSpaceStep;
				mProbes.emplace_back(probePos, mSampleNums);

				std::shared_ptr<FTexture> irradianceTexture = std::make_shared<FTexture>();
                std::shared_ptr<FTextureDesc> irradianceDesc = std::make_shared<FTextureDesc>(
                    GetGLFormat(TextureFormat::RGBA16F), mTextureSize, mTextureSize, true);
                irradianceTexture->applyResource(irradianceDesc);
                irradianceTexture->applyBindlessHandle();
                irradianceTexture->setBindlessTextureResident();
				mIrradianceBindlessTextures.push_back(irradianceTexture);
                mIrradianceBindlessTextureHandles.push_back(irradianceTexture->getBindlessHandle());

                std::shared_ptr<FTexture> depthTexture = std::make_shared<FTexture>();
                std::shared_ptr<FTextureDesc> depthDesc = std::make_shared<FTextureDesc>(
                    GetGLFormat(TextureFormat::RG16F), mTextureSize, mTextureSize, true);
                depthTexture->applyResource(depthDesc);
                depthTexture->applyBindlessHandle();
                depthTexture->setBindlessTextureResident();
                mDepthBindlessTextures.push_back(depthTexture);
				mDepthBindlessTextureHandles.push_back(depthTexture->getBindlessHandle());
            }
        }
    }
}

void FDynamicDiffuseProbe::makeSSBO() {
	mIrradianceBindlessHandleSSBO = std::make_shared<FShaderStorageBuffer>("Irradiance");
	mIrradianceBindlessHandleSSBO->initBuffer(sizeof(uint64_t) * mIrradianceBindlessTextureHandles.size(), 0, GL_STATIC_DRAW);
	mIrradianceBindlessHandleSSBO->setData(mIrradianceBindlessTextureHandles.data(), static_cast<uint32_t>(mIrradianceBindlessTextureHandles.size()));

	mDepthBindlessHandleSSBO = std::make_shared<FShaderStorageBuffer>("Depth");
	mDepthBindlessHandleSSBO->initBuffer(sizeof(uint64_t) * mDepthBindlessTextureHandles.size(), 1, GL_STATIC_DRAW);
	mDepthBindlessHandleSSBO->setData(mDepthBindlessTextureHandles.data(), static_cast<uint32_t>(mDepthBindlessTextureHandles.size()));

	mProbeSSBO = std::make_shared<FShaderStorageBuffer>("Probe");
    mProbeSSBO->initBuffer(sizeof(FProbe) * mProbes.size(), 6, GL_STATIC_DRAW);
	mProbeSSBO->setData(mProbes.data(), static_cast<uint32_t>(mProbes.size()));
}