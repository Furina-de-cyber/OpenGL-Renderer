#pragma once

#include <unordered_set>
#include "../core.h"
#include "texture.h"
#include "RTAsset.h"
#include "buffer.h"
#include "../tools/hash.h"
//---------------------------------material-------------------------------------------
static const std::string DefaultAlbedoPath = "assets/textures/white_mat_small.png";
static const std::string DefaultMetallicPath = "assets/textures/black_mat_small.png";
static const std::string DefaultRoughnessPath = "assets/textures/white_mat_small.png";
static const std::string DefaultNormalPath = "assets/textures/no.png";

void pprint();

class FMaterial {
public:
	FMaterial();
	FMaterial(const std::string inAlbedoPath);
	FMaterial(const std::string inAlbedoPath,
		const std::string inRoughnessPath,
		const std::string inMetallicPath
	);
	FMaterial(const std::string inAlbedoPath,
		const std::string inRoughnessPath,
		const std::string inMetallicPath,
		const std::string inNormalPath
	);
    FMaterial(
        size_t inHash,
		std::shared_ptr<FTexture> inAlbedo,
		std::shared_ptr<FTexture> inRoughness,
		std::shared_ptr<FTexture> inMetallic,
        std::shared_ptr<FTexture> inNormal = nullptr
    );

	~FMaterial() = default;

    const std::unordered_map<std::string, int> MaterialSlotMap = {
        {"albedo"    , 0 }, 
        {"roughness" , 1 }, 
		{"metallic"  , 2 },
        {"normal"    , 3 }
    };
    const std::unordered_map<std::string, std::string> MaterialNameMap = {
        {"albedo"    , "AlbedoSamp"},
        {"roughness" , "RoughnessSamp"},
        {"metallic"  , "MetallicSamp"},
        {"normal"    , "NormalSamp"}
    };
    
	size_t getMatHash() const { return mMatHash; }
public:
	std::shared_ptr<FTexture> getAlbedo() const { return mAlbedo; }
	std::shared_ptr<FTexture> getNormal() const { return mNormal; }
	std::shared_ptr<FTexture> getRoughness() const { return mRoughness; }
	std::shared_ptr<FTexture> getMetallic() const { return mMetallic; }
private:
	std::shared_ptr<FTexture> mAlbedo;
	std::shared_ptr<FTexture> mNormal;
	std::shared_ptr<FTexture> mRoughness;
	std::shared_ptr<FTexture> mMetallic;

	bool mNeedNormal = false;
	size_t mMatHash = 0;
};

struct FBindlessTextureSlot {
    uint64_t mAlbedoSlot;
    uint64_t mNormalSlot;
    uint64_t mRoughnessSlot;
    uint64_t mMetallicSlot;
    FBindlessTextureSlot(std::shared_ptr<FMaterial> material);
};

//---------------------------------mesh-------------------------------------------
struct FVertexBufferDesc {
    struct VertexDesc {
        using type = float;
        std::string name; // vertex, uv, normal
        GLuint index;
        GLuint size;
        GLuint offset;
        GLenum deviceType = GL_FLOAT;
        GLboolean bNormalized = GL_FALSE;
        VertexDesc(GLuint inIndex, GLuint inSize, GLenum inDeviceType,
            bool inNormalized, const std::string& inName, GLuint inOffset = 0)
            : index{ inIndex }, size{ inSize }, deviceType{ inDeviceType },
			bNormalized{ static_cast<GLboolean>(inNormalized) }, name{ inName }, offset{ inOffset }
        {
        }
        bool operator==(const VertexDesc& other) const {
            return index == other.index && size == other.size &&
                deviceType == other.deviceType && bNormalized == other.bNormalized &&
                name == other.name && offset == other.offset;
		}
    };
    FVertexBufferDesc() { mStride = 0; }
    void addDesc(GLuint inIndex, GLuint inSize, GLenum inDeviceType,bool inNormalized, const std::string& inName);
    void sortSet();
    bool isEmpty() const { return mDescSet.empty(); }
    void rename(VertexDesc& desc, const std::string& inName) { desc.name = inName; }


    int mStride = 0;
    std::vector<VertexDesc> mDescSet = {};

    bool operator==(const FVertexBufferDesc& other) const {
        if (mStride != other.mStride || mDescSet.size() != other.mDescSet.size()) {
            return false;
        }
        for (size_t i = 0; i < mDescSet.size(); ++i) {
            if (!(mDescSet[i] == other.mDescSet[i])) {
                return false;
            }
        }
        return true;
	}
};

namespace VertexArrayObject {
    void genVAO(GLuint* inHandle);
    void deleteVAO(GLuint* inHandle);
    void uploadVAO(GLenum inTarget, GLuint inBufferHandle, GLuint inVAOHandle,
        std::shared_ptr<FVertexBufferDesc> inDesc, GLuint inEBOHandle);
};

class FMeshBuffer {
public:
    FMeshBuffer(GLenum inTarget);
    ~FMeshBuffer();

    template<typename T>
    void uploadData(const std::vector<T>& inData); 

public:
    GLuint getHandle() const { return mBufferHandle; }
	GLenum getTarget() const { return mTarget; }
    void bind() const { glBindBuffer(mTarget, mBufferHandle); }
    void unbind() const { glBindBuffer(mTarget, 0); }
	
private:
    GLuint mBufferHandle = 0;
    size_t mDataSize = 0;
    GLenum mTarget = GL_NONE;
};

class FMesh {
public:
    FMesh() = default;
    ~FMesh();

    void uploadCompleteData(
        std::vector<float>&& inVBOData,
        std::vector<uint32_t>&& inEBOData,
        std::shared_ptr<FVertexBufferDesc> inDesc
    );
	void draw() const;
    GLuint getVAOHandle() const { return mVAOHandle; }
	glm::vec3 getGeometryCenter() const { return mGeometryCenter; }
	glm::mat4 getModelMatrix() const { return mModelMatrix; }
	void setModelMatrix(const glm::mat4& inModelMatrix) { mModelMatrix = inModelMatrix; }
	const std::vector<float>& getVertexBufferData() const { return mVertexBufferData; }
	const std::vector<uint32_t>& getElementBufferData() const { return mElementBufferData; }
	const std::shared_ptr<FVertexBufferDesc> getVertexBufferDesc() const { return mVertexArrayDesc; }
private:
    std::shared_ptr<FVertexBufferDesc> mVertexArrayDesc = nullptr;
    std::shared_ptr<FMeshBuffer> mVertexBufferObject = nullptr;
    std::shared_ptr<FMeshBuffer> mElementBufferObject = nullptr;
    std::vector<float> mVertexBufferData;
    std::vector<uint32_t> mElementBufferData;
    GLuint mVAOHandle = 0;
    void getGeometryCenter(glm::vec3& center);
	glm::vec3 mGeometryCenter = glm::vec3(0.0f);
	glm::mat4 mModelMatrix = glm::mat4(1.0f);
};
//------------------------------------light------------------------------------------------

//struct FLight
//{
//    FLight() = default;
//	virtual ~FLight() = 0;
//	glm::vec4 mPosition = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
//	glm::vec4 mDirection = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
//	glm::vec4 mColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
//	float mIntensity = 1.0f;
//	int mType = -1; // 0: directional, 1: rect, 2: spot, 3: point, -1: illegal
//    float mFallOffExp = 2.0f;
//    float mRadius = 1.0f;
//};
//
//struct FRectLight : public FLight
//{
//    FRectLight() = default;
//    FRectLight(float inHalfWidth, float inHalfHeight, glm::vec4 inRight);
//    FRectLight(float inHalfWidth, float inHalfHeight);
//    ~FRectLight() = default;
//	glm::vec4 mRight = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
//    float mHalfWidth = 1.0f;
//    float mHalfHeight = 1.0f;
//    float pad1 = 0.0f;
//    float pad2 = 0.0f;
//};

struct FLight {
	FLight() = default;
    FLight(float inHalfWidth, float inHalfHeight, glm::vec4 inRight);
    FLight(float inHalfWidth, float inHalfHeight);
    ~FLight() = default;
    glm::vec4 mPosition = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 mDirection = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    glm::vec4 mColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    float mIntensity = 1.0f;
    int mType = -1; // 0: directional, 1: rect, 2: spot, 3: point, -1: illegal
    float mFallOffExp = 2.0f;
    float mRadius = 1.0f;
    glm::vec4 mRight = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    float mHalfWidth = 1.0f;
    float mHalfHeight = 1.0f;
    float pad1 = 0.0f;
    float pad2 = 0.0f;
    
};

struct FShadowTerm {
    float mFar = 100.0f;
    float mNearBias = 0.01f;
    float mFarBias = 100.0f;
    float mNear = 0.01f;
    float maxTan = 0.02f;
    float slopeTan = 0.01f;
    float constTan = 0.0001f;
    const int mShadowMapWidth = 1024;
    const int mShadowMapHeight = 1024;
    glm::mat4 lightViewMatrix;
    glm::mat4 lightProjMatrix;
    glm::vec4 refineLightPosition;
    glm::vec4 lightAngleInfo;
    FShadowTerm() = default;
    FShadowTerm(std::shared_ptr<FLight> inLight, float nearBias, float far);
};

void getLightMatrix(std::shared_ptr<FLight> inLight, float nearBias, float far, float& near,
    glm::mat4& lightViewMatrix, glm::mat4& lightProjMatrix, glm::vec4& refineLightPosition,
    glm::vec4& lightAngleInfo);

//---------------------------------asset manager-------------------------------------------
struct FRTAsset {
	FRTAsset() = default;
	std::vector<float> mSceneVBOAll = {};                 // with world space position, uv, normal
    std::vector<uint32_t> mSceneEBOALL = {};              // 3 elements -> 1 triangle
	// length of mSceneBindlessPoolIndex is the same as the nums of triangle 
	std::vector<uint32_t> mSceneBindlessPoolIndex = {};   // index to mSceneBindlessPool, -1 means no material
	uint32_t mTrianglesNum = 0;

	std::vector<FRTTriangleInfo> mSceneTriangleInfo = {}; // total nums of triangle = mSceneEBOALL.size() / 3

    std::shared_ptr<FVertexBufferDesc> mUniformVAO = nullptr;
    std::vector<FBindlessTextureSlot> mSceneBindlessPool = {};

	std::shared_ptr<FShaderStorageBuffer> mSceneVBOBuffer = nullptr;
	std::shared_ptr<FShaderStorageBuffer> mSceneEBOBuffer = nullptr;
	std::shared_ptr<FShaderStorageBuffer> mSceneBindlessPoolIndexBuffer = nullptr;
	std::shared_ptr<FShaderStorageBuffer> mSceneBindlessPoolInfoBuffer = nullptr;

	const uint32_t mSceneVBOBindingSlot = 0;
	const uint32_t mSceneEBOBindingSlot = 1;
	const uint32_t mSceneBindlessPoolIndexBindingSlot = 2;
	const uint32_t mSceneBindlessPoolInfoBindingSlot = 3;
};

struct FAsset {
public:
    std::unordered_map<size_t, std::shared_ptr<FMaterial>> mMaterial;
	std::unordered_map<std::shared_ptr<FMesh>, size_t> mMesh;
    FAsset() = default;
    glm::mat4 getModelMatrix() const { return mModelMatrix; }
    void setModelMatrix(const glm::mat4& inModelMatrix) { mModelMatrix = inModelMatrix; }
    bool mShadow = false;

private:
    glm::mat4 mModelMatrix = glm::mat4(1.0f);
};

//--------------------------------- asset utils -------------------------------------------
void GeneratePlaneBufferData(
    const glm::vec3& center,
    const glm::vec3& normal,
    float width,
    float height,

    std::vector<float>& vertexPtr,
    std::vector<uint32_t>& elePtr,
    std::shared_ptr<FVertexBufferDesc> descPtr
);

void GenerateBoxBufferData(
    const glm::vec3& center,
    const glm::vec3& size,
    std::vector<float>& vertexPtr,
    std::vector<uint32_t>& elePtr,
    std::shared_ptr<FVertexBufferDesc> descPtr
);

void GenerateSkyboxCube(
    const glm::vec3& center,
    const glm::vec3& size,
    std::vector<float>& vertexPtr,
    std::vector<uint32_t>& elePtr,
    std::shared_ptr<FVertexBufferDesc> descPtr
);

glm::mat4 createSimpleModelMatrix(const glm::vec3& translation,
    const glm::vec3& rotation,
    const glm::vec3& scale);

void GenerateUVSphereBufferData(
    const glm::vec3& center,
    float radius,
    uint32_t longitudeSegments,
    uint32_t latitudeSegments,
    bool positionOnly,

    std::vector<float>& vertexPtr,
    std::vector<uint32_t>& elePtr,
    std::shared_ptr<FVertexBufferDesc> descPtr,
    bool reverseNormal = false
);

void GenerateLightBoxBufferData(
    const glm::vec3& center,
    float halfWidth,   // x
    float halfHeight,  // z
    float halfBias,    // y
    bool positionOnly,

    std::vector<float>& vertexPtr,
    std::vector<uint32_t>& elePtr,
    std::shared_ptr<FVertexBufferDesc> descPtr
);

