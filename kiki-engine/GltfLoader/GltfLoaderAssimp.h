#pragma once

#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <iostream>
#include <stb_image.h>
#include <Components/ColourComponent.hpp>
#include <assimp/GltfMaterial.h>
#include <spdlog/spdlog.h>
#include "Animation/Skeleton.h"
#include "Animation/AnimationLoader.h"

#define ASSIMP_FLAGS aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_LimitBoneWeights

using namespace std;
struct Mmesh {
	std::string name;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
	std::vector<uint32_t> indices;

	std::vector<glm::ivec4> boneIDs;
	std::vector<glm::vec4> weights;

	int matIndex;

	std::vector<glm::vec4> tangents;
	std::vector<glm::vec3> bitangents;

};

struct RGBA {
	uint8_t r, g, b, a;
};
enum class alphaMode {
	OPAQUE,
	MASK,
	BLEND
};
struct Mtexture {
	bool hastexture = true;
	std::vector<RGBA> data;
	std::vector<uint8_t> rawData; // For compressed textures
	stbi_uc* rawDataPtr = nullptr;
	stbi_uc* roughness = nullptr;
	stbi_uc* normalMap = nullptr;
	int width = 0;
	int height = 0;
	int roughWidth = 0;
	int roughHeight = 0;
	int normalMapWidth = 0;
	int normalMapHeight = 0;
	int channels = 0;
	std::string name;
	alphaMode mode = alphaMode::OPAQUE;
	float alphaCutoff = 0;
	glm::vec4 baseColour = glm::vec4(1.f);
	float roughnessFactor = 1.f;
	float metallicFactor = 1.f;
	bool hasNormalMap = false;


	~Mtexture() {
		if (rawDataPtr) {
			stbi_image_free(rawDataPtr);
		}
		if (roughness) {
			stbi_image_free(roughness);
		}

		if (normalMap) {
			stbi_image_free(normalMap);
		}
	}

	Mtexture() = default;

	Mtexture(const Mtexture&) = delete;
	Mtexture& operator=(const Mtexture&) = delete;

	Mtexture(Mtexture&& other) noexcept
		: hastexture(other.hastexture),
		data(std::move(other.data)),
		rawData(std::move(other.rawData)),
		rawDataPtr(other.rawDataPtr),
		roughness(other.roughness),
		normalMap(other.normalMap),
		width(other.width),
		height(other.height),
		channels(other.channels),
		roughWidth(other.roughWidth),
		roughHeight(other.roughHeight),
		normalMapWidth(other.normalMapWidth),
		normalMapHeight(other.normalMapHeight),
		name(std::move(other.name)),
		mode(other.mode),
		alphaCutoff(other.alphaCutoff),
		baseColour(other.baseColour),
		roughnessFactor(other.roughnessFactor),
		metallicFactor(other.metallicFactor),
		hasNormalMap(other.hasNormalMap)
	{
		other.rawDataPtr = nullptr;
		other.roughness = nullptr;
		other.normalMap = nullptr;
	}
	explicit Mtexture(bool hasTex)
		: hastexture(hasTex) {
	}
	Mtexture& operator=(Mtexture&& other) noexcept {
		if (this != &other) {
			if (rawDataPtr) stbi_image_free(rawDataPtr);
			if (roughness) stbi_image_free(roughness);
			if (normalMap) stbi_image_free(normalMap);

			hastexture = other.hastexture;
			data = std::move(other.data);
			rawData = std::move(other.rawData);
			rawDataPtr = other.rawDataPtr;
			roughness = other.roughness;
			normalMap = other.normalMap;
			width = other.width;
			height = other.height;
			channels = other.channels;
			roughWidth = other.roughWidth;
			roughHeight = other.roughHeight;
			normalMapWidth = other.normalMapWidth;
			normalMapHeight = other.normalMapHeight;
			name = std::move(other.name);
			mode = other.mode;
			alphaCutoff = other.alphaCutoff;
			baseColour = other.baseColour;
			roughnessFactor = other.roughnessFactor;
			metallicFactor = other.metallicFactor;
			hasNormalMap = other.hasNormalMap;

			other.rawDataPtr = nullptr;
			other.roughness = nullptr;
			other.normalMap = nullptr;
		}
		return *this;
	}
};

enum class MlightType {
	POINT,
	DIRECTIONAL,
	SPOT
};
enum class MbodyType {
	STATIC,
	DYNAMIC,
	KINEMATIC
};
enum class McolliderType {
	NONE,
	BOX,
	SPHERE,
	CAPSULE,
	MESH,
	CONVEX_HULL
};
enum class MmiscTags {
	NONE,
	FLOOR,
	LAVA,
	SPEED_BOOST,
	DOUBLE_JUMP,
	DASH,
	TRIGGER,
	PLAYER,
	GOAL,
	SPAWN,
	DOOR
};

enum class MtriggerKind {
	NONE,
	GOAL,
	TELEPORT
};

enum class MsimpleAnimType {
	NONE,
	UP_DOWN,
	DOWN_UP,
	LEFT_RIGHT,
	RIGHT_LEFT,
	FORWARD_BACKWARD,
	BACKWARD_FORWARD,
	ROTATE_CLOCKWISE,
	ROTATE_COUNTERCLOCKWISE
};


struct MmeshInstance {
	int meshIndex;
	glm::mat4 transform;
	MbodyType bodyType = MbodyType::STATIC;
	McolliderType colliderType = McolliderType::NONE;
	MmiscTags miscTag = MmiscTags::NONE;
	MsimpleAnimType simpleAnim = MsimpleAnimType::NONE;
	float anim_distance = 5.f;
	float anim_speed = 1.f;
	float anim_rotation_speed = 90.f;

	MtriggerKind triggerKind = MtriggerKind::NONE;
	glm::vec3    triggerHalfExtents = { 1.f, 1.f, 0.1f };
	int          teleportLoopNum = 0;
	int          teleportOrder   = 0;

	float doorRadius      = 2.0f;
	float doorAngleDeg    = 90.0f;
	float doorSpeedDegSec = 540.0f;
};
struct MemtpyInstance {
	glm::mat4 transform;
	MbodyType bodyType = MbodyType::STATIC;
	McolliderType colliderType = McolliderType::NONE;
	MmiscTags miscTag = MmiscTags::NONE;

	MtriggerKind triggerKind = MtriggerKind::NONE;
	glm::vec3    triggerHalfExtents = { 1.f, 1.f, 0.1f };
	int          teleportLoopNum = 0;
	int          teleportOrder   = 0;
};
struct Mlights {
	std::string name;
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
	MlightType type = MlightType::POINT;
	glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f);
	
};
struct Mscene {
	std::vector<MemtpyInstance> emptyInstances;
	std::vector<MmeshInstance> instances;
	std::vector<Mmesh> meshes;
	std::vector<Mtexture> textures;
	std::vector<Mlights> lights;
	glm::mat4 worldTransform;
	// === 新增：用于存储动画和骨骼数据 ===
	std::unique_ptr<Kiki::Skeleton> skeleton = nullptr;
	std::map<std::string, std::unique_ptr<Kiki::Animation>> animations;
	bool hasAnimations = false;
};

namespace Kiki {
	class GltfLoaderAssimp {
	public:
		static void AddBoneData(
			glm::ivec4& ids,
			glm::vec4& weights,
			int boneID,
			float weight
		) {
			for (int i = 0; i < 4; i++) {
				if (weights[i] == 0.0f) {
					ids[i] = boneID;
					weights[i] = weight;
					return;
				}
			}
		}

		static Mmesh loadMesh(
			const std::filesystem::path& path,
			int index,
			const Skeleton& skeleton)
		{
			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(path.string(), ASSIMP_FLAGS);
			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
				throw std::runtime_error("Failed to load glTF file: " + path.string() + " with error: " + importer.GetErrorString());
			}

			Mmesh out{};
			aiMesh* mesh = scene->mMeshes[index]; // For simplicity, we only load the first mesh
			out.vertices.reserve(mesh->mNumVertices);
			out.normals.reserve(mesh->mNumVertices);
			out.uvs.reserve(mesh->mNumVertices);
			out.matIndex = mesh->mMaterialIndex;
			out.boneIDs.resize(mesh->mNumVertices, glm::ivec4(0));
			out.weights.resize(mesh->mNumVertices, glm::vec4(0.0f));
			for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
				out.vertices.emplace_back(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
				if (mesh->HasNormals()) {
					out.normals.emplace_back(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
				}
				if (mesh->HasTextureCoords(0)) {
					out.uvs.emplace_back(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
				}
			}
			for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
				aiFace face = mesh->mFaces[i];
				for (unsigned int j = 0; j < face.mNumIndices; j++) {
					out.indices.push_back(face.mIndices[j]);
				}
			}
			

			for (unsigned int i = 0; i < mesh->mNumBones; i++) {
				aiBone* bone = mesh->mBones[i];

				std::string boneName = bone->mName.C_Str();

				int boneID = skeleton.FindBoneIndex(boneName);
				if (boneID == -1) continue;

				for (unsigned int w = 0; w < bone->mNumWeights; w++) {
					int vertexID = bone->mWeights[w].mVertexId;
					float weight = bone->mWeights[w].mWeight;

					AddBoneData(
						out.boneIDs[vertexID],
						out.weights[vertexID],
						boneID,
						weight
					);
				}
			}
			for (size_t i = 0; i < out.weights.size(); i++) {
				float sum =
					out.weights[i].x +
					out.weights[i].y +
					out.weights[i].z +
					out.weights[i].w;

				if (sum > 0.0f) {
					out.weights[i] /= sum;
				}
			}
			return out;
		}

		static Mtexture loadTexture(const std::filesystem::path& path, int index) {
			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(path.string(), ASSIMP_FLAGS);
			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
				throw std::runtime_error("Failed to load glTF file: " + path.string() + " with error: " + importer.GetErrorString());
			}
			if (scene->HasTextures() == false) return Mtexture(false);

			const aiMaterial* mat = scene->mMaterials[index];
			aiString textureName;// = scene->mTextures[0]; // For simplicity, we only load the first texture
			aiString roughName;// = scene->mTextures[1]; // For simplicity, we only load the first texture
			mat->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &textureName);
			mat->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &roughName);

			std::cout << textureName.C_Str() << "~" << roughName.C_Str() << std::endl;

			
			stbi_set_flip_vertically_on_load(1);
			Mtexture out{};
			//out.name = texture->mFilename.C_Str();
			//out.data.reserve(texture->mWidth * texture->mHeight);
			if (textureName.length > 0) {
				int i = std::stoi(textureName.C_Str() + 1);
				const aiTexture* texture = scene->mTextures[i];
				out.rawDataPtr = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(texture->pcData), texture->mWidth, &out.width, &out.height, &out.channels, 4);
			} else {
				out.rawDataPtr = stbi_load((std::filesystem::path(PROJECT_ASSETS_PATH) / "empty.png").string().c_str(), &out.width, &out.height, &out.channels, 4);
			}

			if (roughName.length > 0) {
				int j = std::stoi(roughName.C_Str() + 1);
				const aiTexture* rough = scene->mTextures[j];
				out.roughness = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(rough->pcData), rough->mWidth, &out.width, &out.height, &out.channels, 4);

			} else {
				out.roughness = stbi_load((std::filesystem::path(PROJECT_ASSETS_PATH) / "empty.png").string().c_str(), &out.width, &out.height, &out.channels, 4);
			}
			
			// if (texture->mHeight == 0) {
			// 	// Compressed texture
			// 	const uint8_t* raw = reinterpret_cast<const uint8_t*>(texture->pcData);
			// 	out.rawData.assign(raw, raw + texture->mWidth);
			// 	out.width = 0;
			// 	out.height = 0;
			// 	return out;
			// }
			// for (unsigned int i = 0; i < texture->mWidth * texture->mHeight; i++) {
			// 	aiTexel texel = texture->pcData[i];
			// 	out.data.emplace_back(texel.r, texel.g, texel.b, texel.a);
			// }
			// out.width = texture->mWidth;
			// out.height = texture->mHeight;
			// out.channels = 4; // Assimp always loads textures as RGBA
			return out;
		}
		

		static glm::mat4 toglmMat4(const aiMatrix4x4& t) {
			return glm::mat4(
				t.a1, t.b1, t.c1, t.d1,
				t.a2, t.b2, t.c2, t.d2,
				t.a3, t.b3, t.c3, t.d3,
				t.a4, t.b4, t.c4, t.d4
			);
		}
		static MmiscTags parseMiscTag(aiNode* node) {
			if (!node || !node->mMetaData) {
				return MmiscTags::NONE;
			}

			aiString miscTagStr;
			if (!node->mMetaData->Get("misc", miscTagStr)) {
				return MmiscTags::NONE;
			}

			std::string s = miscTagStr.C_Str();

			if (s == "floor")   return MmiscTags::FLOOR;
			if (s == "lava")    return MmiscTags::LAVA;
			if (s == "speed_boost") return MmiscTags::SPEED_BOOST;
			if (s == "double_jump") return MmiscTags::DOUBLE_JUMP;
			if (s == "dash") return MmiscTags::DASH;
			if (s == "trigger") return MmiscTags::TRIGGER;
			if (s == "player")  return MmiscTags::PLAYER;
			if (s == "goal")    return MmiscTags::GOAL;
			if (s == "spawn")   return MmiscTags::SPAWN;
			if (s == "door")    return MmiscTags::DOOR;

			return MmiscTags::NONE;

		}

		static MtriggerKind parseTriggerKind(aiNode* node) {
			if (!node || !node->mMetaData) {
				return MtriggerKind::NONE;
			}
			aiString s;
			if (!node->mMetaData->Get("trigger_kind", s)) {
				return MtriggerKind::NONE;
			}
			std::string str = s.C_Str();
			if (str == "goal")     return MtriggerKind::GOAL;
			if (str == "teleport") return MtriggerKind::TELEPORT;
			return MtriggerKind::NONE;
		}

		static float parseFloatExtra(aiNode* node, const char* key, float dflt) {
			if (!node || !node->mMetaData) return dflt;
			float v = dflt;
			if (node->mMetaData->Get(key, v)) return v;
			double d;
			if (node->mMetaData->Get(key, d)) return (float)d;
			return dflt;
		}

		static int parseIntExtra(aiNode* node, const char* key, int dflt) {
			if (!node || !node->mMetaData) return dflt;
			int v = dflt;
			if (node->mMetaData->Get(key, v)) return v;
			float f;
			if (node->mMetaData->Get(key, f)) return (int)f;
			double d;
			if (node->mMetaData->Get(key, d)) return (int)d;
			return dflt;
		}

		static MsimpleAnimType parseSimpleAnimType(aiNode* node) {
			if (!node || !node->mMetaData) {
				return MsimpleAnimType::NONE;
			}
			aiString animTypeStr;
			if (!node->mMetaData->Get("anim", animTypeStr)) {
				return MsimpleAnimType::NONE;
			}
			std::string s = animTypeStr.C_Str();
			if (s == "up_down") return MsimpleAnimType::UP_DOWN;
			if (s == "down_up") return MsimpleAnimType::DOWN_UP;
			if (s == "left_right") return MsimpleAnimType::LEFT_RIGHT;
			if (s == "right_left") return MsimpleAnimType::RIGHT_LEFT;
			if (s == "forward_backward") return MsimpleAnimType::FORWARD_BACKWARD;
			if (s == "backward_forward") return MsimpleAnimType::BACKWARD_FORWARD;
			if (s == "rotate_clockwise") return MsimpleAnimType::ROTATE_CLOCKWISE;
			if (s == "rotate_counterclockwise") return MsimpleAnimType::ROTATE_COUNTERCLOCKWISE;
			return MsimpleAnimType::NONE;
		}

		static void collectNodeInstances(
			aiNode* node,
			const glm::mat4& parentTransform,
			Mscene& out)
		{
			glm::mat4 localTransform = toglmMat4(node->mTransformation);
			glm::mat4 worldTransform = parentTransform * localTransform;
			
			for (unsigned int i = 0; i < node->mNumMeshes; i++) {
				MmeshInstance instance;
				instance.meshIndex = node->mMeshes[i];
				instance.transform = worldTransform;

				if (node->mMetaData) {
					aiString bodyTypeStr;
					node->mMetaData->Get("body", bodyTypeStr);

					//cout << "Body type for node " << node->mName.C_Str() << ": " << bodyTypeStr.C_Str() << endl;

					if (string(bodyTypeStr.C_Str()) == "dynamic") {
						instance.bodyType = MbodyType::DYNAMIC;
					}
					else if (string(bodyTypeStr.C_Str()) == "kinematic") {
						instance.bodyType = MbodyType::KINEMATIC;
					}
					else {
						instance.bodyType = MbodyType::STATIC;
					}

					aiString colliderTypeStr;
					node->mMetaData->Get("collider", colliderTypeStr);

					//cout << "Collider type for node " << node->mName.C_Str() << ": " << colliderTypeStr.C_Str() << endl;

					if (string(colliderTypeStr.C_Str()) == "box") {
						instance.colliderType = McolliderType::BOX;
					}
					else if (string(colliderTypeStr.C_Str()) == "sphere") {
						instance.colliderType = McolliderType::SPHERE;
					}
					else if (string(colliderTypeStr.C_Str()) == "capsule") {
						instance.colliderType = McolliderType::CAPSULE;
					}
					else if (string(colliderTypeStr.C_Str()) == "mesh") {
						instance.colliderType = McolliderType::MESH;
					}
					else if (string(colliderTypeStr.C_Str()) == "convex_hull") {
						instance.colliderType = McolliderType::CONVEX_HULL;
					}
					else {
						instance.colliderType = McolliderType::NONE;
					}

					instance.miscTag = parseMiscTag(node);

					instance.triggerKind = parseTriggerKind(node);
					instance.triggerHalfExtents = glm::vec3(
						parseFloatExtra(node, "trigger_half_x", instance.triggerHalfExtents.x),
						parseFloatExtra(node, "trigger_half_y", instance.triggerHalfExtents.y),
						parseFloatExtra(node, "trigger_half_z", instance.triggerHalfExtents.z));
					instance.teleportLoopNum = parseIntExtra(node, "teleport_loop_num", 0);
					instance.teleportOrder   = parseIntExtra(node, "teleport_order",   0);

					instance.simpleAnim = parseSimpleAnimType(node);
					float animDistance;
					if (node->mMetaData->Get("anim_distance", animDistance)) {
						instance.anim_distance = animDistance;
					}
					float animSpeed;
					if (node->mMetaData->Get("anim_speed", animSpeed)) {
						instance.anim_speed = animSpeed;
					}
					float animRotationSpeed;
					if (node->mMetaData->Get("anim_rotation_speed", animRotationSpeed)) {
						instance.anim_rotation_speed = animRotationSpeed;
					}

					instance.doorRadius      = parseFloatExtra(node, "door_radius", instance.doorRadius);
					instance.doorAngleDeg    = parseFloatExtra(node, "door_angle",  instance.doorAngleDeg);
					instance.doorSpeedDegSec = parseFloatExtra(node, "door_speed",  instance.doorSpeedDegSec);
				}

				out.instances.push_back(instance);
			}
			MmiscTags miscTag = parseMiscTag(node);
			if (miscTag != MmiscTags::NONE && node->mNumMeshes == 0) {
				MemtpyInstance emptyInstance;
				emptyInstance.transform = worldTransform;
				emptyInstance.bodyType = MbodyType::STATIC; // Default to static for empty instances
				emptyInstance.colliderType = McolliderType::NONE; // Default to no collider for empty instances
				emptyInstance.miscTag = miscTag;
				emptyInstance.triggerKind = parseTriggerKind(node);
				emptyInstance.triggerHalfExtents = glm::vec3(
					parseFloatExtra(node, "trigger_half_x", emptyInstance.triggerHalfExtents.x),
					parseFloatExtra(node, "trigger_half_y", emptyInstance.triggerHalfExtents.y),
					parseFloatExtra(node, "trigger_half_z", emptyInstance.triggerHalfExtents.z));
				emptyInstance.teleportLoopNum = parseIntExtra(node, "teleport_loop_num", 0);
				emptyInstance.teleportOrder   = parseIntExtra(node, "teleport_order",   0);
				spdlog::info("Empty instance for node {} with misc tag: {}", node->mName.C_Str(), static_cast<int>(miscTag));
				spdlog::info("Transform for empty instance:");
				for (int i = 0; i < 4; i++) {
					spdlog::info("{} {} {} {}", worldTransform[i][0], worldTransform[i][1], worldTransform[i][2], worldTransform[i][3]);
				}
				out.emptyInstances.push_back(emptyInstance);
			}

			for (unsigned int i = 0; i < node->mNumChildren; i++) {
				collectNodeInstances(node->mChildren[i], worldTransform, out);
			}
		}

		static glm::mat4 getNodeWorldTransform(const aiNode* node) {
			glm::mat4 result(1.0f);

			const aiNode* current = node;
			while (current) {
				result = toglmMat4(current->mTransformation) * result;
				current = current->mParent;
			}

			return result;
		}

		static void collectLights(const aiScene* scene, Mscene& out) {

			for (unsigned int i = 0; i < scene->mNumLights; i++) {
				aiLight* aiLight = scene->mLights[i];
				if (!aiLight) continue;
				
				Mlights light;
				light.name = aiLight->mName.C_Str();

				const aiNode* lightNode = scene->mRootNode->FindNode(aiLight->mName);
				glm::mat4 nodeworld = glm::mat4(1.0f);
				if (lightNode) {
					nodeworld = getNodeWorldTransform(lightNode);
				}

				glm::vec4 localPos(aiLight->mPosition.x, aiLight->mPosition.y, aiLight->mPosition.z, 1.0f);
				glm::vec4 worldPos = nodeworld * localPos;
				light.position = glm::vec3(worldPos);
				glm::vec3 rawcolor(aiLight->mColorDiffuse.r, aiLight->mColorDiffuse.g, aiLight->mColorDiffuse.b);
				light.type = (aiLightSourceType)aiLight->mType == aiLightSource_POINT ? MlightType::POINT :
					(aiLightSourceType)aiLight->mType == aiLightSource_DIRECTIONAL ? MlightType::DIRECTIONAL :
					(aiLightSourceType)aiLight->mType == aiLightSource_SPOT ? MlightType::SPOT : MlightType::POINT;
				if (light.type == MlightType::DIRECTIONAL) {
					glm::vec4 localDir(aiLight->mDirection.x, aiLight->mDirection.y, aiLight->mDirection.z, 0.0f);
					glm::vec4 worldDir = nodeworld * localDir;
					light.direction = glm::normalize(glm::vec3(worldDir));
				}

				// std::cout << "Light " << light.name << " position: " << light.position.x << ", " << light.position.y << ", " << light.position.z << std::endl;
				// std::cout << "Light " << light.name << " color: " << rawcolor.r << ", " << rawcolor.g << ", " << rawcolor.b << std::endl;

				// std::cout << "Light " << light.name << " type: " <<
				// (light.type == MlightType::POINT ? "POINT" : light.type == MlightType::DIRECTIONAL ? "DIRECTIONAL" : light.type == MlightType::SPOT ? "SPOT" : "UNKNOWN")
				// << std::endl;

				// std::cout << "Light " << light.name << " direction: " << light.direction.x << ", " << light.direction.y << ", " << light.direction.z << std::endl;
				light.color = rawcolor;
				out.lights.push_back(light);
			}
		}

		static Mscene loadScene(const std::filesystem::path& path){
			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(path.string(), ASSIMP_FLAGS);
			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
				throw std::runtime_error("Failed to load glTF file: " + path.string() + " with error: " + importer.GetErrorString());
			}
			Mscene out{};
			auto& t = scene->mRootNode->mTransformation;
			out.worldTransform = toglmMat4(t);

			if (scene->HasAnimations()) {
				out.hasAnimations = true;
				out.skeleton = AnimationLoader::LoadSkeleton(scene);

				if (out.skeleton) {
					for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
						std::string animName = scene->mAnimations[i]->mName.C_Str();
						std::transform(animName.begin(), animName.end(), animName.begin(), ::tolower);
						out.animations[animName] = AnimationLoader::LoadAnimation(scene, *out.skeleton, i);
					}
				}
			}

			collectNodeInstances(scene->mRootNode, glm::mat4(1.0f), out);

			collectLights(scene, out);

			for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
				Mmesh mesh;
				aiMesh* aiMesh = scene->mMeshes[i];
				mesh.name = aiMesh->mName.C_Str();
				mesh.vertices.reserve(aiMesh->mNumVertices);
				mesh.normals.reserve(aiMesh->mNumVertices);
				mesh.uvs.reserve(aiMesh->mNumVertices);
				mesh.matIndex = aiMesh->mMaterialIndex;
				mesh.tangents.reserve(aiMesh->mNumVertices);
				mesh.bitangents.reserve(aiMesh->mNumVertices);
				for (unsigned int j = 0; j < aiMesh->mNumVertices; j++) {
					mesh.vertices.emplace_back(aiMesh->mVertices[j].x, aiMesh->mVertices[j].y, aiMesh->mVertices[j].z);
					if (aiMesh->HasNormals()) {
						mesh.normals.emplace_back(aiMesh->mNormals[j].x, aiMesh->mNormals[j].y, aiMesh->mNormals[j].z);
					}
					if (aiMesh->HasTextureCoords(0)) {
						mesh.uvs.emplace_back(aiMesh->mTextureCoords[0][j].x, aiMesh->mTextureCoords[0][j].y);
					}
					if(aiMesh->HasTangentsAndBitangents()){
						glm::vec3 T(aiMesh->mTangents[j].x, aiMesh->mTangents[j].y, aiMesh->mTangents[j].z);
						glm::vec3 B(aiMesh->mBitangents[j].x, aiMesh->mBitangents[j].y, aiMesh->mBitangents[j].z);
						glm::vec3 N(aiMesh->mNormals[j].x, aiMesh->mNormals[j].y, aiMesh->mNormals[j].z);

						float handedness;

						if (glm::dot(glm::cross(N, T), B) < 0.f) {
							handedness = -1.f;
						}
						else {
							handedness = 1.f;
						}

						mesh.tangents.emplace_back(aiMesh->mTangents[j].x, aiMesh->mTangents[j].y, aiMesh->mTangents[j].z, handedness);
						mesh.bitangents.emplace_back(aiMesh->mBitangents[j].x, aiMesh->mBitangents[j].y, aiMesh->mBitangents[j].z);
					}
				}
				for (unsigned int j = 0; j < aiMesh->mNumFaces; j++) {
					aiFace face = aiMesh->mFaces[j];
					for (unsigned int k = 0; k < face.mNumIndices; k++) {
						mesh.indices.push_back(face.mIndices[k]);
					}
				}

				mesh.boneIDs.resize(aiMesh->mNumVertices, glm::ivec4(0));
				mesh.weights.resize(aiMesh->mNumVertices, glm::vec4(0.0f));

				if (out.skeleton) {
					for (unsigned int b = 0; b < aiMesh->mNumBones; b++) {
						aiBone* bone = aiMesh->mBones[b];
						std::string boneName = bone->mName.C_Str();

						int boneID = out.skeleton->FindBoneIndex(boneName);
						if (boneID == -1) continue;

						for (unsigned int w = 0; w < bone->mNumWeights; w++) {
							int vertexID = bone->mWeights[w].mVertexId;
							float weight = bone->mWeights[w].mWeight;

							AddBoneData(
								mesh.boneIDs[vertexID],
								mesh.weights[vertexID],
								boneID,
								weight
							);
						}
					}

					for (size_t v = 0; v < mesh.weights.size(); v++) {
						float sum = mesh.weights[v].x + mesh.weights[v].y + mesh.weights[v].z + mesh.weights[v].w;
						if (sum > 0.0f) mesh.weights[v] /= sum;
					}
				}
				out.meshes.push_back(mesh);
			}
			for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
				Mtexture texture;
				const aiMaterial* mat = scene->mMaterials[i];
				

				aiColor4D baseColour(1.f, 1.f, 1.f, 1.f);
				if (mat->Get(AI_MATKEY_BASE_COLOR, baseColour) == AI_SUCCESS) {
					texture.baseColour = glm::vec4(baseColour.r, baseColour.g, baseColour.b, baseColour.a);
				}

				float rF = 1.f;
				float mF = 1.f;

				if (mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, rF) == AI_SUCCESS) {
					texture.roughnessFactor = rF;
				}

				if (mat->Get(AI_MATKEY_METALLIC_FACTOR, mF) == AI_SUCCESS) {
					texture.metallicFactor = mF;
				}


				aiString textureName;
				aiString roughName;
				aiString normalMapName;
				mat->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &textureName);
				mat->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &roughName);
				mat->GetTexture(aiTextureType_NORMALS, 0, &normalMapName);
				if (textureName.length > 0) {
					texture.hastexture = true;
					int j = std::stoi(textureName.C_Str() + 1);
					const aiTexture* aiTexture = scene->mTextures[j];
					texture.rawDataPtr = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(aiTexture->pcData), aiTexture->mWidth, &texture.width, &texture.height, &texture.channels, 4);
				} else {
					texture.hastexture = 0;
					texture.rawDataPtr = stbi_load((std::filesystem::path(PROJECT_ASSETS_PATH) / "empty.png").string().c_str(), &texture.width, &texture.height, &texture.channels, 4);
				}
				if (roughName.length > 0) {
					int j = std::stoi(roughName.C_Str() + 1);
					const aiTexture* aiTexture = scene->mTextures[j];
					texture.roughness = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(aiTexture->pcData), aiTexture->mWidth, &texture.roughWidth, &texture.roughHeight, &texture.channels, 4);
				} else {
					texture.roughness = stbi_load((std::filesystem::path(PROJECT_ASSETS_PATH) / "empty.png").string().c_str(), &texture.roughWidth, &texture.roughHeight, &texture.channels, 4);
				}

				if (normalMapName.length > 0) {
					texture.hasNormalMap = true;
					int j = std::stoi(normalMapName.C_Str() + 1);
					const aiTexture* aiTexture = scene->mTextures[j];
					texture.normalMap = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(aiTexture->pcData), aiTexture->mWidth, &texture.normalMapWidth, &texture.normalMapHeight, &texture.channels, 4);
				}
				else {
					texture.hasNormalMap = false;
					texture.normalMap = stbi_load((std::filesystem::path(PROJECT_ASSETS_PATH) / "empty_normals.png").string().c_str(), &texture.normalMapWidth, &texture.normalMapHeight, &texture.channels, 4);
				}

				aiString alphaMode;
				if (mat->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS) {
					std::string mode = alphaMode.C_Str();
					if (mode == "BLEND") {
						texture.mode = alphaMode::BLEND;
					}
					else if (mode == "MASK") {
						texture.mode = alphaMode::MASK;
						float cutoff = 0.5f;
						if (mat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, cutoff) == AI_SUCCESS) {
							texture.alphaCutoff = cutoff;
						}
					}
					else {
						texture.mode = alphaMode::OPAQUE;
					}
				}


				out.textures.push_back(std::move(texture));
			}


			return out;
		}


		static void debugPrintMesh(const Mmesh& mesh) {
			// std::cout << "Vertices: " << mesh.vertices.size() << std::endl;
			// for (const auto& vertex : mesh.vertices) {
			// 	std::cout << "  " << vertex.x << ", " << vertex.y << ", " << vertex.z << std::endl;
			// }
			// std::cout << "Normals: " << mesh.normals.size() << std::endl;
			// for (const auto& normal : mesh.normals) {
			// 	std::cout << "  " << normal.x << ", " << normal.y << ", " << normal.z << std::endl;
			// }
			// std::cout << "UVs: " << mesh.uvs.size() << std::endl;
			// for (const auto& uv : mesh.uvs) {
			// 	std::cout << "  " << uv.x << ", " << uv.y << std::endl;
			// }
			// std::cout << "Indices: " << mesh.indices.size() << std::endl;
			// for (const auto& index : mesh.indices) {
			// 	std::cout << "  " << index << std::endl;
			// }

			// std::cout << "UVs: " << mesh.uvs.size() << std::endl;
			// for (const auto& uv : mesh.uvs) {
			// 	std::cout << uv << std::endl;
			// }
		}

		static void debugPrintTexture(const Mtexture& texture) {
			std::cout << "Texture Name: " << texture.name << std::endl;
			std::cout << "Width: " << texture.width << std::endl;
			std::cout << "Height: " << texture.height << std::endl;
			std::cout << "Channels: " << texture.channels << std::endl;
			//std::cout << "Has Transparency: " << (texture.hasTransparency() ? "Yes" : "No") << std::endl;
			std::cout << "Alpha Mode: " << (texture.mode == alphaMode::BLEND ? "BLEND" : (texture.mode == alphaMode::MASK ? "MASK" : "OPAQUE")) << std::endl;
			std::cout << "Alpha Cutoff: " << texture.alphaCutoff << std::endl;

		}
	};
}
