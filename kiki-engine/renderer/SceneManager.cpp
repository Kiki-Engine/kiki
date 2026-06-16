#include "SceneManager.hpp"
#include "GltfLoader/GltfLoaderAssimp.h"
#include <volk.h>
#include <glm/gtx/matrix_decompose.hpp> 

#include "physics/PhysicsComponents.hpp" 
#include "physics/PhysicsUtils.hpp"   
#include "physics/PhysicsSystem.hpp" 
#include "Components/MiscComponent.hpp"
#include "Components/RoughnessMetallicFactorComponent.hpp"
#include "Components/TriggerComponent.hpp"
#include "Components/LevelEntityTag.hpp"
#include "Components/DoorComponent.hpp"

#include "Animation/AnimationLoader.h"
#include "Animation/AnimationComponent.h"
#include "Components/SimpleAnimationComponent.hpp"

#include <assimp/Importer.hpp> 

#include <spdlog/spdlog.h>

namespace Kiki {
    SceneManager& SceneManager::get() {
        static SceneManager instance;
        return instance;
    }

     int SceneManager::createMaterial(Mtexture& texture) {
         materials.emplace_back(RenderManager::get().allocateMaterial(texture));

         return materials.size() - 1;
     }

    Material const& SceneManager::getMaterial(int id) {
        return materials[id];
    }

    int SceneManager::createMesh(
        std::vector<glm::vec3> const& positions,
        std::vector<std::uint32_t> const& indices,
        std::vector<glm::vec3> const& normals,
        std::vector<glm::vec2> const& texCoords,
        std::vector<glm::vec4> const& tangents,
        std::vector<glm::ivec4> const& boneIDs,  
        std::vector<glm::vec4> const& weights  
    )
    {
        std::vector<float> p, t, n, tangents_dst;

        for (glm::vec3 pos : positions) {
            p.emplace_back(pos.x);
            p.emplace_back(pos.y);
            p.emplace_back(pos.z);
        }

        for (glm::vec2 texCoord : texCoords) {
            t.emplace_back(texCoord.x);
            t.emplace_back(texCoord.y);
        }

        for (glm::vec3 norm : normals) {
            n.emplace_back(norm.x);
            n.emplace_back(norm.y);
            n.emplace_back(norm.z);
        }

        std::vector<int> bIDs;
        std::vector<float> bWeights;

        for (const auto& id : boneIDs) {
            bIDs.push_back(id.x); bIDs.push_back(id.y); bIDs.push_back(id.z); bIDs.push_back(id.w);
        }
        for (const auto& w : weights) {
            bWeights.push_back(w.x); bWeights.push_back(w.y); bWeights.push_back(w.z); bWeights.push_back(w.w);
        }

        //spdlog::info("Allocating Mesh -> Pos: {}, Indices: {}, Normals: {}, UVs: {}, Bones: {}, Weights: {}", p.size(), indices.size(), n.size(), t.size(), bIDs.size(), bWeights.size());

        for (glm::vec4 tan : tangents) {
            tangents_dst.emplace_back(tan.x);
            tangents_dst.emplace_back(tan.y);
            tangents_dst.emplace_back(tan.z);
            tangents_dst.emplace_back(tan.w);
        }

        meshes.emplace_back(RenderManager::get().allocateMesh(p, indices, n, t, tangents_dst, bIDs, bWeights));

        return meshes.size() - 1;
    }

    Mesh& SceneManager::getMesh(int id) {
        return meshes[id];
    }

    void SceneManager::clearLevel() {
        {
            std::lock_guard<std::mutex> lock(RenderManager::get().queueMutex);
            vkDeviceWaitIdle(RenderManager::get().getDevice());

            auto& registry = World::Get().Registry();

            auto view = World::Get().Query<LevelEntityTag>();

            {
                std::lock_guard<std::mutex> lock(registryMutex);
                std::vector<entt::entity> toDestroy;
                for (auto [e,tag] : view.each()) {
                    toDestroy.push_back(e);
                }
                for (auto e : toDestroy) {
                    registry.destroy(e);
                }
            }

            materials.clear();
            meshes.clear();
            RenderManager::get().lights.clear();
        }
    }

    entt::entity SceneManager::loadModel(const std::string path, const std::string name, PhysicsType type) {
        auto model = World::Get().CreateEntity();
        auto& registry = World::Get().Registry();

        std::string fullPath = (std::filesystem::path(PROJECT_ASSETS_PATH) / path).string();

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(fullPath, ASSIMP_FLAGS);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            spdlog::error("Failed to load glTF file: {} with error: {}", fullPath, importer.GetErrorString());
            return model;
        }

        std::unique_ptr<Skeleton> skeleton = nullptr;
        std::unique_ptr<Animation> animation = nullptr;

        if (scene->HasAnimations()) {
            skeleton = AnimationLoader::LoadSkeleton(scene);
            if (skeleton) {
                animation = AnimationLoader::LoadAnimation(scene, *skeleton, 0);
            }
        }

        Skeleton dummySkeleton;
        Mmesh mesh = Kiki::GltfLoaderAssimp::loadMesh(fullPath, 0, skeleton ? *skeleton : dummySkeleton);
        Mtexture texture = Kiki::GltfLoaderAssimp::loadTexture(fullPath, 0);

        registry.emplace<TransformComponent>(model);
        registry.emplace<MeshComponent>(model, createMesh(mesh.vertices, mesh.indices, mesh.normals, mesh.uvs, mesh.tangents, mesh.boneIDs, mesh.weights));
        registry.emplace<TagComponent>(model, entt::hashed_string(name.c_str()), name);

        registry.emplace<MeshComponent>(model, createMesh(
            mesh.vertices,
            mesh.indices,
            mesh.normals,
            mesh.uvs,
            mesh.tangents,
            mesh.boneIDs, 
            mesh.weights 
        ));

        if (skeleton && scene->HasAnimations()) {
            auto& animComp = registry.emplace<AnimationComponent>(model);
            animComp.skeleton = std::move(skeleton);

			// read all animations and assign them to the state machine based on their names
            for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
                std::string animName = scene->mAnimations[i]->mName.C_Str();
                std::transform(animName.begin(), animName.end(), animName.begin(), ::tolower);

                spdlog::info("Found animation in glTF: {}", animName);

                if (animName.find("idle") != std::string::npos) {
                    animComp.animations[CharacterState::Idle] = AnimationLoader::LoadAnimation(scene, *animComp.skeleton, i);
                }
                else if (animName.find("walking") != std::string::npos) {
                    animComp.animations[CharacterState::Walking] = AnimationLoader::LoadAnimation(scene, *animComp.skeleton, i);
                }
                else if (animName.find("running") != std::string::npos) {
                    animComp.animations[CharacterState::Running] = AnimationLoader::LoadAnimation(scene, *animComp.skeleton, i);
                }
                else if (animName.find("jumping") != std::string::npos) {
                    animComp.animations[CharacterState::Jumping] = AnimationLoader::LoadAnimation(scene, *animComp.skeleton, i);
                }
            }

            animComp.ChangeState(CharacterState::Idle);

            if (animComp.animations.find(CharacterState::Idle) == animComp.animations.end()) {
                spdlog::warn("No 'Idle' animation found! Using the first available animation as default.");
                animComp.animations[CharacterState::Idle] = AnimationLoader::LoadAnimation(scene, *animComp.skeleton, 0);
            }

            animComp.animator.Update(0.0f, *animComp.skeleton, *animComp.animations[CharacterState::Idle]);

            animComp.boneMatrixBuffer = RenderManager::get().allocateAnimationBuffer();
            animComp.descriptorSet = RenderManager::get().allocateAnimationDescriptorSet(animComp.boneMatrixBuffer);
            animComp.UpdateGpuBuffer(RenderManager::get().allocator.allocator);
        }

        JPH::Ref<JPH::Shape> colliderShape;
        JPH::EMotionType joltMotionType;
        uint16_t joltLayer;

        switch (type) {
        case PhysicsType::Static:
            colliderShape = CreateTriangleMesh(mesh.vertices, mesh.indices);
            joltMotionType = JPH::EMotionType::Static;
            joltLayer = 0; // NON_MOVING
            break;

        case PhysicsType::Dynamic:
            colliderShape = CreateConvexHull(mesh.vertices);
            joltMotionType = JPH::EMotionType::Dynamic;
            joltLayer = 1; // MOVING
            break;

        case PhysicsType::Kinematic:
            colliderShape = CreateTriangleMesh(mesh.vertices, mesh.indices);
            joltMotionType = JPH::EMotionType::Kinematic;
            joltLayer = 0;
            break;
        }

        if (colliderShape) {
            registry.emplace<MeshColliderComponent>(model, colliderShape);
            registry.emplace<RigidBodyComponent>(model, joltMotionType, joltLayer);
            registry.emplace<PhysicalAttributesComponent>(model);
            spdlog::info("Model {} loaded as {}", path,
                type == PhysicsType::Static ? "Static" : (type == PhysicsType::Dynamic ? "Dynamic" : "Kinematic"));
        }

        if (texture.hastexture) {
            registry.emplace<MaterialComponent>(model, createMaterial(texture));
        }

        registry.emplace<ColourComponent>(model, glm::vec3(1, 0, 0));

        // Kiki::GltfLoaderAssimp::debugPrintMesh(mesh);
        // Kiki::GltfLoaderAssimp::debugPrintTexture(texture);

        return model;
    }


    void SceneManager::loadScene(Mscene scene) {
       

        // // renderLights[0] must be directional
        // if (RenderManager::get().lights.size() == 0) {
        //     for (size_t i = 0; i < scene.lights.size(); i++) {
        //         if (scene.lights[i].type == MlightType::DIRECTIONAL) {
        //             Light rLight;
        //             rLight.colour = glm::vec4(scene.lights[i].color, 1.f);
        //             rLight.position = glm::vec4(scene.lights[i].direction, 1.f);
        //             RenderManager::get().lights.emplace_back(rLight);
        //         }
        //     }
        // }

        // // force one directional light
        // if (RenderManager::get().lights.size() == 0) {
        //     Light rLight;
        //     rLight.colour = glm::vec4(1.f, 1.f, 1.f, 1.f);
        //     rLight.position = glm::vec4(0.f, -1.f, 0.f, 1.f);
        //     RenderManager::get().lights.emplace_back(rLight);
        // }

        // push point lights
        for (size_t i = 0; i < scene.lights.size(); i++) {
            if (scene.lights[i].type == MlightType::POINT) {
                Light rLight;
                rLight.colour = glm::vec4(scene.lights[i].color, 1.f);
                rLight.position = glm::vec4(scene.lights[i].position, 1.f);

                rLight.colour.w = max(max(rLight.colour.x, rLight.colour.y), rLight.colour.z);
                rLight.colour.x /= 54351.4f;
                rLight.colour.y /= 54351.4f;
                rLight.colour.z /= 54351.4f;
                rLight.colour.w /= 54351.4f;

                spdlog::info("[Scene] Loaded point light with colour ({:.3f}, {:.3f}, {:.3f}, {:.3f}) and position ({:.3f}, {:.3f}, {:.3f})",
                    rLight.colour.x, rLight.colour.y, rLight.colour.z, rLight.colour.w,
                    rLight.position.x, rLight.position.y, rLight.position.z
                );

                RenderManager::get().lights.emplace_back(rLight);
            }
        }

        for (int i = 0; i < scene.instances.size(); i++) {
            entt::entity model;
            {
                std::lock_guard<std::mutex> lock(registryMutex);
                model = World::Get().CreateEntity();
            }
            auto& registry = World::Get().Registry();
			const auto& instance = scene.instances[i];
            const Mmesh& mesh = scene.meshes[instance.meshIndex];
            const Mtexture& texture = scene.textures[mesh.matIndex];
            {
                std::lock_guard<std::mutex> lock(registryMutex);
                registry.emplace<LevelEntityTag>(model);
                auto& transform = registry.emplace<TransformComponent>(model);
                glm::vec3 skew;
                glm::vec4 perspective;
                glm::decompose(scene.instances[i].transform, transform.scale, transform.rotation, transform.position, skew, perspective);
                //transform.rotation = glm::conjugate(transform.rotation);
                //transform.scale = {0.3, 0.3, 0.3}; // TODO: this is a temp fix, will probably cause issues
            }

            auto& transform = registry.get<TransformComponent>(model);

            std::vector<glm::ivec4> safeBoneIDs = mesh.boneIDs;
            std::vector<glm::vec4> safeWeights = mesh.weights;

            if (safeBoneIDs.empty() || safeWeights.empty()) {
                safeBoneIDs.assign(mesh.vertices.size(), glm::ivec4(0));
                safeWeights.assign(mesh.vertices.size(), glm::vec4(0.0f));
            }


            {
                std::lock_guard<std::mutex> lock(registryMutex);
                registry.emplace<MeshComponent>(model, createMesh(
                    mesh.vertices,
                    mesh.indices,
                    mesh.normals,
                    mesh.uvs,
                    mesh.tangents,
                    safeBoneIDs,
                    safeWeights
                ));
            }

            if (texture.hastexture) {
                materials.emplace_back(RenderManager::get().allocateMaterial(texture));
				int id = materials.size() - 1;

                {
                    std::lock_guard<std::mutex> lock(registryMutex);
                    registry.emplace<MaterialComponent>(model, id);
                    if (texture.mode == alphaMode::MASK) {
                        registry.emplace<TransparencyComponent>(model); // yeah idk what else to do other then just have this added
                    }
                }
            }


            {
                std::lock_guard<std::mutex> lock(registryMutex);
                registry.emplace<ColourComponent>(model, glm::vec3(texture.baseColour));
                registry.emplace<RoughnessMetallicFactorComponent>(model, glm::vec2(texture.roughnessFactor, texture.metallicFactor));
            }
            //Kiki::GltfLoaderAssimp::debugPrintMesh(mesh);
			//Kiki::GltfLoaderAssimp::debugPrintTexture(texture);

            if (scene.hasAnimations && (instance.miscTag == MmiscTags::PLAYER)) {

                spdlog::info("Instance {} is a PLAYER with animations, attaching AnimationComponent...", i);


                {
                    std::lock_guard<std::mutex> lock(registryMutex);
                    registry.emplace<AnimationComponent>(model);
                }

                auto& animComp = registry.get<AnimationComponent>(model);

                if (scene.skeleton) {
                    animComp.skeleton = std::move(scene.skeleton);
                }

                for (auto& [animName, animPtr] : scene.animations) {
                    if (animName.find("idle") != std::string::npos) {
                        animComp.animations[CharacterState::Idle] = std::move(animPtr);
                    }
                    else if (animName.find("walking") != std::string::npos) {
                        animComp.animations[CharacterState::Walking] = std::move(animPtr);
                    }
                    else if (animName.find("running") != std::string::npos) {
                        animComp.animations[CharacterState::Running] = std::move(animPtr);
                    }
                    else if (animName.find("jumping") != std::string::npos) {
                        animComp.animations[CharacterState::Jumping] = std::move(animPtr);
                    }
                }

                // set default state
                animComp.ChangeState(CharacterState::Idle);

                animComp.boneMatrixBuffer = RenderManager::get().allocateAnimationBuffer();
                animComp.descriptorSet = RenderManager::get().allocateAnimationDescriptorSet(animComp.boneMatrixBuffer);
                animComp.UpdateGpuBuffer(RenderManager::get().allocator.allocator);

                spdlog::info("AnimationComponent initialized for player successfully.");
            }

			// Physics setup


            {
                std::lock_guard<std::mutex> lock(registryMutex);
                registry.emplace<TagComponent>(model, entt::hashed_string(mesh.name.c_str()), mesh.name);
            }
            JPH::Ref<JPH::Shape> colliderShape;
            JPH::EMotionType joltMotionType;
            uint16_t joltLayer;
            //COPIED WHAT WAS DONE IN LOAD MODEL, PLEASE WORK ON THIS FUNCTION FROM NOW AS PREVIOUS FUNCTION HAS DEPRECATED FUNCTIONS

			switch (instance.bodyType) {
			case MbodyType::STATIC:
                //colliderShape = CreateTriangleMesh(mesh.vertices, mesh.indices);
                joltMotionType = JPH::EMotionType::Static;
                joltLayer = 0; // NON_MOVING
                break;
            case MbodyType::DYNAMIC:
                //colliderShape = CreateConvexHull(mesh.vertices);
                joltMotionType = JPH::EMotionType::Dynamic;
                joltLayer = 1; // MOVING
				break;
            case MbodyType::KINEMATIC:
                //colliderShape = CreateTriangleMesh(mesh.vertices, mesh.indices);
                joltMotionType = JPH::EMotionType::Kinematic;
				joltLayer = 0;
                break;
             }

            if (instance.miscTag == MmiscTags::DOOR) {
                joltMotionType = JPH::EMotionType::Kinematic;
                joltLayer = 0;
            }

            switch (instance.colliderType) {
            case McolliderType::BOX:
                {
                    std::lock_guard<std::mutex> lock(registryMutex);
                    registry.emplace<BoxColliderComponent>(model);
                }
                spdlog::info("[Physics] Attached BOX collider to mesh: '{}'", mesh.name);
                break;

            case McolliderType::SPHERE:
                {
                    std::lock_guard<std::mutex> lock(registryMutex);
                    registry.emplace<SphereColliderComponent>(model);
                }
                spdlog::info("[Physics] Attached SPHERE collider to mesh: '{}'", mesh.name);
                break;

            case McolliderType::CAPSULE:
            {
                glm::vec3 minAABB = mesh.vertices[0];
                glm::vec3 maxAABB = mesh.vertices[0];

                for (const auto& v : mesh.vertices) {
                    minAABB = glm::min(minAABB, v);
                    maxAABB = glm::max(maxAABB, v);
                }

                float sizeX = maxAABB.x - minAABB.x;
                float sizeY = maxAABB.y - minAABB.y; 
                float sizeZ = maxAABB.z - minAABB.z;

                float radius = std::max(sizeX, sizeZ) / 2.0f;

                float joltHalfHeight = (sizeY - 2.0f * radius) / 2.0f;

                if (joltHalfHeight < 0.05f) {
                    joltHalfHeight = 0.05f;
                }
                {
                    std::lock_guard<std::mutex> lock(registryMutex);
                    registry.emplace<CapsuleColliderComponent>(model, radius / 2.5f, joltHalfHeight);
                }

                spdlog::info("[Physics] Calculated CAPSULE: Radius = {:.2f}, HalfHeight = {:.2f}, TotalHeight = {:.2f}",
                    radius, joltHalfHeight, sizeY);
                break;
            }

            case McolliderType::CONVEX_HULL:
                if (auto hull = CreateConvexHull(mesh.vertices)) {
                    {
                        std::lock_guard<std::mutex> lock(registryMutex);
                        registry.emplace<MeshColliderComponent>(model, hull);
                    }
                    spdlog::info("[Physics] Attached CONVEX_HULL collider to mesh: '{}'", mesh.name);
                }
                else {
                    spdlog::warn("[Physics] Failed to create CONVEX_HULL for mesh: '{}'", mesh.name);
                }
                break;

            case McolliderType::MESH:
            case McolliderType::NONE:
            default:
                if (auto triMesh = CreateTriangleMesh(mesh.vertices, mesh.indices)) {
                    {
                        std::lock_guard<std::mutex> lock(registryMutex);
                        registry.emplace<MeshColliderComponent>(model, triMesh);
                    }
                    spdlog::info("[Physics] Attached TRIANGLE_MESH (Default) collider to mesh: '{}'", mesh.name);
                }
                else {
                    spdlog::warn("[Physics] Failed to create TRIANGLE_MESH for mesh: '{}'", mesh.name);
                }
                break;
            }

			if (instance.simpleAnim != MsimpleAnimType::NONE) {
                {
                    std::lock_guard<std::mutex> lock(registryMutex);
                    registry.emplace<SimpleAnimationComponent>(model);
                }
				auto& simpleAnimComp = registry.get<SimpleAnimationComponent>(model);
				simpleAnimComp.type = instance.simpleAnim;
				simpleAnimComp.startPosition = transform.position; // Store the initial position for oscillation
				simpleAnimComp.startRotation = transform.rotation; // Store the initial rotation for rotation animations
                simpleAnimComp.distance = instance.anim_distance;
				simpleAnimComp.speed = instance.anim_speed;
                simpleAnimComp.rotationSpeed = instance.anim_rotation_speed;
                spdlog::info("[Animation] Attached SimpleAnimationComponent with animation '{}' to mesh: '{}'", 
                    static_cast<int>(instance.simpleAnim), mesh.name);
            }

            //registry.emplace<RigidBodyComponent>(model, joltMotionType, joltLayer);
            bool isPlayer = (instance.miscTag == MmiscTags::PLAYER);
            float playerFriction = isPlayer ? 0.0f : 0.5f;
            //Set the friction of the capsule body to 0, so there won't be Titanfall's wall-running.
            {
                std::lock_guard<std::mutex> lock(registryMutex);
                registry.emplace<RigidBodyComponent>(model, joltMotionType, joltLayer, 0.0f, playerFriction, false, isPlayer);
                registry.emplace<PhysicalAttributesComponent>(model);
            }

			// Misc tags
            if (instance.miscTag != MmiscTags::NONE) {
                {
                    std::lock_guard<std::mutex> lock(registryMutex);
                    registry.emplace<MiscComponent>(model, instance.miscTag);
                }
			}

            // Backwards compat: legacy GOAL meshes get a GoalTag (CollisionEvent path)
            if (instance.miscTag == MmiscTags::GOAL) {
                std::lock_guard<std::mutex> lock(registryMutex);
                if (!registry.any_of<GoalTag>(model)) {
                    registry.emplace<GoalTag>(model);
                }
            }

            // New TRIGGER tag path on a mesh entity (rare but supported)
            if (instance.miscTag == MmiscTags::TRIGGER) {
                std::lock_guard<std::mutex> lock(registryMutex);
                registry.emplace<TriggerComponent>(model, instance.triggerHalfExtents, true);
                switch (instance.triggerKind) {
                    case MtriggerKind::GOAL:
                        registry.emplace<GoalTag>(model);
                        break;
                    case MtriggerKind::TELEPORT:
                        registry.emplace<TeleportTag>(model, instance.teleportLoopNum, instance.teleportOrder);
                        break;
                    default: break;
                }
            }

            if (instance.miscTag == MmiscTags::DOOR) {
                std::lock_guard<std::mutex> lock(registryMutex);
                DoorComponent door;
                door.triggerRadius   = instance.doorRadius;
                door.openAngleDeg    = instance.doorAngleDeg;
                door.speedDegPerSec  = instance.doorSpeedDegSec;
                door.closedRotation  = transform.rotation;
                registry.emplace<DoorComponent>(model, door);
                spdlog::info("[Door] Attached DoorComponent to mesh '{}' (radius={:.2f}, angle={:.2f}, speed={:.2f})",
                    mesh.name, door.triggerRadius, door.openAngleDeg, door.speedDegPerSec);
            }
        }
        for (int i = 0; i < scene.emptyInstances.size(); i++) {
            entt::entity model;
            {
                std::lock_guard<std::mutex> lock(registryMutex);
                model = World::Get().CreateEntity();
            }
            auto& registry = World::Get().Registry();
            const auto& instance = scene.emptyInstances[i];
            {
                std::lock_guard<std::mutex> lock(registryMutex);
                registry.emplace<LevelEntityTag>(model);
                registry.emplace<TransformComponent>(model);
            }
            auto& transform = registry.get<TransformComponent>(model);
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(instance.transform, transform.scale, transform.rotation, transform.position, skew, perspective);
            //transform.rotation = glm::conjugate(transform.rotation);
            // Misc tags
            if (instance.miscTag != MmiscTags::NONE) {
                {
                    std::lock_guard<std::mutex> lock(registryMutex);
                    registry.emplace<MiscComponent>(model, instance.miscTag);
                }
            }

            if (instance.miscTag == MmiscTags::TRIGGER) {
                std::lock_guard<std::mutex> lock(registryMutex);
                registry.emplace<TriggerComponent>(model, instance.triggerHalfExtents, true);
                switch (instance.triggerKind) {
                    case MtriggerKind::GOAL:
                        registry.emplace<GoalTag>(model);
                        break;
                    case MtriggerKind::TELEPORT:
                        registry.emplace<TeleportTag>(model, instance.teleportLoopNum, instance.teleportOrder);
                        break;
                    default: break;
                }
            } else if (instance.miscTag == MmiscTags::GOAL) {
                // Legacy: empty marker tagged "goal" -> only attach GoalTag (no plane crossing)
                std::lock_guard<std::mutex> lock(registryMutex);
                registry.emplace<GoalTag>(model);
            }
        }
    }

    bool SceneManager::validMaterial(int id) {
        return id >= 0 && id < materials.size();
    }

    bool SceneManager::validMesh(int id) {
        return id >= 0 && id < meshes.size();
    }

    void SceneManager::shutdown() {
        materials.clear();
        meshes.clear();
    }
}