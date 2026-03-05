#pragma once

#include "../core.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../engine/asset.h"
#include "hash.h"

enum class EMaterialTexUsage {
	Albedo,
	Roughness,
	Metallic,
	Normal,
	Specular,
	AmbientOcclusion,
	Emissive
};



class FModelLoader {
public:
	FModelLoader() = default;
	~FModelLoader() = default;
	void importModel(std::shared_ptr<FAsset>& outAsset,
		const std::string& modelPath,
		bool loadNormal = false
	) {
		mNormalTexture = loadNormal;
		std::size_t lastIndex = modelPath.find_last_of('//');
		mRootPath = modelPath.substr(0, lastIndex + 1);
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(
			modelPath.c_str(),
			aiProcess_Triangulate |           // 三角化
			aiProcess_CalcTangentSpace |      // 计算切线空间
			aiProcess_GenSmoothNormals      // 生成平滑法线
		);
		//const aiScene* scene = importer.ReadFile(
		//	modelPath.c_str(),
		//	aiProcess_Triangulate
		//);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::cout << "Error: " << importer.GetErrorString() << std::endl;
			return;
		}
		outAsset = std::make_shared<FAsset>();
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		ProcessNode(outAsset, scene->mRootNode, scene, modelMatrix);
	}
private:
	bool mNormalTexture = false;
	bool mAllowBaseColorTexture = true;
	const std::string mDefaultTexPath = "assets/textures/white_mat_small.png";
	const std::string mDefaultTexMetallicPath = "assets/textures/black_mat_small.png";
	std::string mRootPath;

	void ProcessNode(
		std::shared_ptr<FAsset>& outAsset, 
		aiNode* node, 
		const aiScene* scene,
		glm::mat4 parentTransform
	) {
		glm::mat4 nodeTransform = glm::transpose(glm::make_mat4(&node->mTransformation.a1));
		glm::mat4 globalTransform = parentTransform * nodeTransform;
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			ProcessMesh(outAsset, mesh, scene, globalTransform);
		}
		for (unsigned int i = 0; i < node->mNumChildren; i++) {
			ProcessNode(outAsset, node->mChildren[i], scene, globalTransform);
		}
	}
	void ProcessMesh(
		std::shared_ptr<FAsset>& outAsset, 
		aiMesh* mesh, 
		const aiScene* scene,
		glm::mat4 parentTransform
	) {
		//std::cout << "Mesh name: " << mesh->mName.C_Str() << std::endl;
		//std::cout << "Vertex count: " << mesh->mNumVertices << std::endl;
		//std::cout << "Face count: " << mesh->mNumFaces << std::endl;
		std::shared_ptr<FVertexBufferDesc> desc = std::make_shared<FVertexBufferDesc>();
		desc->addDesc(0, 3, GL_FLOAT, GL_FALSE, "vertex");
		desc->addDesc(1, 2, GL_FLOAT, GL_FALSE, "uv");
		desc->addDesc(2, 3, GL_FLOAT, GL_FALSE, "normal");
		std::vector<float> tempVertexData = {};
		std::vector<uint32_t> tempElementData = {};
		auto allocVertex = [mesh](int index, std::vector<float>& inVec, int& stride) {
			aiVector3D vertex = mesh->mVertices[index];
			inVec.insert(inVec.end(), { vertex.x, vertex.y, vertex.z });
			stride += 3;
			};
		auto allocUV = [mesh](int index, std::vector<float>& inVec, int& stride) {
			for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; i++) {
				if (mesh->HasTextureCoords(i)) {
					aiVector3D texCoord = mesh->mTextureCoords[i][index];
					//inVec.insert(inVec.end(), { texCoord.x, texCoord.y });
					inVec.push_back(texCoord.x);
					inVec.push_back(texCoord.y);
					break; // 通常使用第一组UV
				}
				else {
					//inVec.insert(inVec.end(), { 0.0f, 0.0f});
					inVec.push_back(0.0f);
					inVec.push_back(0.0f);
					break;
				}
			}
			stride += 2;
			};
		auto allocNormal = [mesh](int index, std::vector<float>& inVec, int& stride) {
			if (!(mesh->HasNormals())) {
				//inVec.insert(inVec.end(), { 0.0f, 0.0f, 1.0f });
				inVec.push_back(0.0f);
				inVec.push_back(0.0f);
				inVec.push_back(1.0f);
			}
			else {
				aiVector3D normal = mesh->mNormals[index];
				//inVec.insert(inVec.end(), { normal.x, normal.y, normal.z });
				inVec.push_back(normal.x);
				inVec.push_back(normal.y);
				inVec.push_back(normal.z);
			}
			stride += 3;
			};

		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			int stride = desc->mStride;
			//std::vector<float> tempVertex(stride);
			std::vector<float> tempVertex;
			tempVertex.reserve(stride);
			int testStride = 0;
			for (const auto& info : desc->mDescSet) {
				if (info.name == "vertex") allocVertex(i, tempVertex, testStride);
				else if (info.name == "uv") allocUV(i, tempVertex, testStride);
				else if (info.name == "normal") allocNormal(i, tempVertex, testStride);
			}
			if (testStride != stride) {
				throw std::runtime_error("stride error in VAO");
				return;
			}
			tempVertexData.insert(tempVertexData.end(),
				std::make_move_iterator(tempVertex.begin()),
				std::make_move_iterator(tempVertex.end())
			);
			tempVertex.clear();
		}
		//mTempElementData.reserve(mesh->mNumFaces * 3);
		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				//mTempElementData.push_back(mNumElement + face.mIndices[j]);
				tempElementData.push_back(face.mIndices[j]);
			}
		}
		std::shared_ptr<FMesh> subMesh = std::make_shared<FMesh>();
		subMesh->setModelMatrix(parentTransform);
		subMesh->uploadCompleteData(
			std::move(tempVertexData),
			std::move(tempElementData),
			desc
		);
		if (mesh->mMaterialIndex >= 0) {
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			ProcessMaterial(outAsset, subMesh, material, scene);
		}
	}
	void ProcessMaterial(std::shared_ptr<FAsset>& outAsset,
		std::shared_ptr<FMesh> subMesh,
		aiMaterial* material,
		const aiScene* scene
	) {
		std::string albedoPath;
		std::string roughnessPath;
		std::string metallicPath;
		std::string normalPath = DefaultNormalPath;
		std::shared_ptr<FTexture> albedoTex = nullptr;
		std::shared_ptr<FTexture> roughnessTex = nullptr;
		std::shared_ptr<FTexture> metallicTex = nullptr;
		std::shared_ptr<FTexture> normalTex = nullptr;
		auto processTex = [this, material, scene](aiTextureType type, std::string& fullPath) -> std::shared_ptr<FTexture> {
			aiString path;
			auto res = material->Get(AI_MATKEY_TEXTURE(type, 0), path);
			auto tex = std::make_shared<FTexture>();
			int width, height, channels;
			if (!(res == AI_SUCCESS)) {
				fullPath = (type == aiTextureType_METALNESS) ? mDefaultTexMetallicPath: mDefaultTexPath;
#if OPEN_DSA_AND_BINDLESS_TEXTURE
				stbi_info(fullPath.c_str(), &width, &height, &channels);
				tex->applyResource(std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), width, height, false));
				tex->allocateResource(fullPath, true);
#else
				tex->allocateResource(0,
					std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false),
					fullPath
				);
#endif
				return tex;
			}
			fullPath = mRootPath + path.C_Str();
			const aiTexture* aitexture = scene->GetEmbeddedTexture(path.C_Str());
			if (aitexture) {
				unsigned char* data = stbi_load_from_memory(
					reinterpret_cast<unsigned char*>(aitexture->pcData),
					aitexture->mWidth,
					&width,
					&height,
					&channels,
					4
				);
#if OPEN_DSA_AND_BINDLESS_TEXTURE
				tex->applyResource(std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), width, height, false));
				tex->allocateResource(data, true);
#else
				tex->allocateResource(0,
					std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), width, height, false),
					data
				);
#endif
				return tex;
			}
			else {
				if (!stbi_info(fullPath.c_str(), &width, &height, &channels)) {
					throw std::runtime_error("cannot get correct img info");
					return nullptr;
				}
#if OPEN_DSA_AND_BINDLESS_TEXTURE
				tex->applyResource(std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), width, height, false));
				tex->allocateResource(fullPath, true);	
#else
				tex->allocateResource(0,
					std::make_shared<FTextureDesc>(GetGLFormat(TextureFormat::RGBA8), 0, 0, false),
					fullPath
				);
#endif
				return tex;
			}
			};

		if (material->Get(AI_MATKEY_TEXTURE(aiTextureType_BASE_COLOR, 0), albedoPath) == AI_SUCCESS) {
			albedoTex = processTex(aiTextureType_BASE_COLOR, albedoPath);
		}
		else{
			albedoTex = processTex(aiTextureType_DIFFUSE, albedoPath);
		}
		roughnessTex = processTex(aiTextureType_DIFFUSE_ROUGHNESS, roughnessPath);
		metallicTex = processTex(aiTextureType_METALNESS, metallicPath);
		//roughnessTex = processTex(aiTextureType_GLTF_METALLIC_ROUGHNESS, roughnessPath);
		//metallicTex = processTex(aiTextureType_GLTF_METALLIC_ROUGHNESS, metallicPath);
		if (mNormalTexture) {
			normalTex = processTex(aiTextureType_NORMALS, normalPath);
		}
		size_t hash = hash_utils::hash_strings(
			albedoPath,
			roughnessPath,
			metallicPath,
			normalPath
		);
		if (outAsset->mMaterial.contains(hash)) {
			outAsset->mMesh[subMesh] = hash;
		}
		else {
			auto mat = std::make_shared<FMaterial>(
				hash,
				albedoTex,
				roughnessTex,
				metallicTex,
				normalTex
			);
			outAsset->mMaterial[hash] = mat;
			outAsset->mMesh[subMesh] = hash;
		}
	}
};