#pragma once

#include <entt.hpp>
#include "Hazel/Core/UUID.h"
#include "Hazel/Renderer/Camera.h"
#include "Hazel/Renderer/Mesh.h"
#include "Hazel/Renderer/Material.h"
#include "Hazel/Scripting/ScriptEngine.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Project/Project.h"

#include <glm/gtx/quaternion.hpp>
#include <yaml-cpp/yaml.h>

namespace YAML
{
    template <>
    struct convert<glm::vec2>
    {
        static Node encode(const glm::vec2& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, glm::vec2& rhs)
        {
            if (!node.IsSequence() || node.size() != 2)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            return true;
        }
    };

    template <>
    struct convert<glm::vec3>
    {
        static Node encode(const glm::vec3& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, glm::vec3& rhs)
        {
            if (!node.IsSequence() || node.size() != 3)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            return true;
        }
    };

    template <>
    struct convert<glm::vec4>
    {
        static Node encode(const glm::vec4& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.push_back(rhs.w);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, glm::vec4& rhs)
        {
            if (!node.IsSequence() || node.size() != 4)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            rhs.w = node[3].as<float>();
            return true;
        }
    };

    template <>
    struct convert<Hazel::UUID>
    {
        static Node encode(const Hazel::UUID& uuid)
        {
            Node node;
            node.push_back((uint64_t)uuid);
            return node;
        }

        static bool decode(const Node& node, Hazel::UUID& uuid)
        {
            uuid = node.as<uint64_t>();
            return true;
        }
    };
} // namespace YAML

namespace Hazel
{
    struct IDComponent
    {
        UUID ID;

        IDComponent() = default;
        IDComponent(const IDComponent&) = default;

        IDComponent(const UUID& id)
            : ID(id) {}

        YAML::Node Serialize() const
        {
            YAML::Node node;
            node["ID"] = ID;
            return node;
        }

        static IDComponent Deserialize(const YAML::Node& node)
        {
            IDComponent component;
            component.ID = node["ID"][0].as<UUID>();
            return component;
        }
    };

    struct TagComponent
    {
        std::string tag;

        TagComponent() = default;
        TagComponent(const TagComponent&) = default;

        TagComponent(const std::string& tag)
            : tag(tag) {}

        YAML::Node Serialize() const
        {
            YAML::Node node;
            node["Tag"] = tag;
            return node;
        }

        static TagComponent Deserialize(const YAML::Node& node)
        {
            TagComponent component;
            component.tag = node["Tag"].as<std::string>();
            return component;
        }
    };

    struct TransformComponent
    {
        glm::vec3 translation = {0.0f, 0.0f, 0.0f};
        glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
        glm::vec3 scale = {1.0f, 1.0f, 1.0f};

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;

        TransformComponent(const glm::vec3& translation)
            : translation(translation) {}

        TransformComponent(const glm::mat4& transform)
        {
            SetTransform(transform);
        }

        glm::mat4 GetTransform() const
        {
            glm::mat4 rotationMatrix = glm::toMat4(glm::quat(rotationMatrix));

            return glm::translate(glm::mat4(1.0f), translation) * rotationMatrix * glm::scale(glm::mat4(1.0f), scale);
        }

        void SetTransform(const glm::mat4& transform)
        {
            glm::quat orientation;
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(transform, scale, orientation, translation, skew, perspective);
            rotation = glm::eulerAngles(orientation);
        }

        YAML::Node Serialize() const
        {
            YAML::Node node;
            node["Translation"] = translation;
            node["Rotation"] = rotation;
            node["Scale"] = scale;
            return node;
        }

        static TransformComponent Deserialize(const YAML::Node& node)
        {
            TransformComponent component;
            component.translation = node["Translation"].as<glm::vec3>();
            component.rotation = node["Rotation"].as<glm::vec3>();
            component.scale = node["Scale"].as<glm::vec3>();
            return component;
        }
    };

    struct EntityRelationshipComponent
    {
        entt::entity parent = entt::null;
        entt::entity firstChild = entt::null;
        entt::entity nextSibling = entt::null;
        entt::entity prevSibling = entt::null;
        uint32_t childCount = 0;

        YAML::Node Serialize(entt::registry& registry) const
        {
            YAML::Node node;
            node["Parent"] = (parent == entt::null) ? UUID(-1) : registry.get<IDComponent>(parent).ID;
            node["FirstChild"] = (firstChild == entt::null) ? UUID(-1) : registry.get<IDComponent>(firstChild).ID;
            node["NextSibling"] = (nextSibling == entt::null) ? UUID(-1) : registry.get<IDComponent>(nextSibling).ID;
            node["PrevSibling"] = (prevSibling == entt::null) ? UUID(-1) : registry.get<IDComponent>(prevSibling).ID;
            node["ChildCount"] = childCount;
            return node;
        }

        // Warning: the UUID map should already be established
        static EntityRelationshipComponent Deserialize(const YAML::Node& node,
                                                       std::unordered_map<UUID, entt::entity>& uuidToEntityMap)
        {
            EntityRelationshipComponent component;
            auto parentUUID = node["Parent"][0].as<UUID>();
            auto firstChildUUID = node["FirstChild"][0].as<UUID>();
            auto nextSiblingUUID = node["NextSibling"][0].as<UUID>();
            auto prevSiblingUUID = node["PrevSibling"][0].as<UUID>();
            component.parent = (parentUUID == UUID(-1)) ? entt::null : uuidToEntityMap[parentUUID];
            component.firstChild = (firstChildUUID == UUID(-1)) ? entt::null : uuidToEntityMap[firstChildUUID];
            component.nextSibling = (nextSiblingUUID == UUID(-1)) ? entt::null : uuidToEntityMap[nextSiblingUUID];
            component.prevSibling = (prevSiblingUUID == UUID(-1)) ? entt::null : uuidToEntityMap[prevSiblingUUID];
            component.childCount = node["ChildCount"].as<uint32_t>();
            return component;
        }
    };

    struct CameraComponent
    {
        Camera camera;
        bool isViewportCamera = false;
        bool isPrimary = false;
        int priority = 0; // the lower, rendered earlier

        CameraComponent() = default;
        CameraComponent(const CameraComponent&) = default;

        YAML::Node Serialize() const
        {
            YAML::Node node;
            node["Camera"] = camera.Serialize();
            node["IsPrimary"] = isPrimary;
            node["IsViewportCamera"] = isViewportCamera;
            node["Priority"] = priority;
            return node;
        }

        static CameraComponent Deserialize(const YAML::Node& node)
        {
            CameraComponent component;
            component.camera = Camera::Deserialize(node["Camera"]);
            component.isPrimary = node["IsPrimary"].as<bool>();
            component.isViewportCamera = node["IsViewportCamera"].as<bool>();
            component.priority = node["Priority"].as<int>();
            return component;
        }
    };

    struct MeshRendererComponent
    {
        UUID meshUUID = UUID(-1);
        UUID materialUUID = UUID(-1);
        MeshAsset* meshAsset = nullptr;
        MaterialAsset* materialAsset = nullptr;

        YAML::Node Serialize() const
        {
            YAML::Node rootNode;
            rootNode["MeshUUID"] = meshUUID;
            rootNode["MaterialUUID"] = materialUUID;
            return rootNode;
        }

        static MeshRendererComponent Deserialize(const YAML::Node& node)
        {
            AssetManager& assetManager = Project::GetActive()->GetAssetManager();
            MeshRendererComponent component;
            component.meshUUID = node["MeshUUID"][0] ? node["MeshUUID"][0].as<UUID>() : UUID(-1);
            component.materialUUID = node["MaterialUUID"][0] ? node["MaterialUUID"][0].as<UUID>() : UUID(-1);
            if (component.meshUUID != UUID(-1))
            {
                component.meshAsset = assetManager.GetAsset<MeshAsset>(component.meshUUID);
            }
            if (component.materialUUID != UUID(-1))
            {
                component.materialAsset = assetManager.GetAsset<MaterialAsset>(component.materialUUID);
            }
            return component;
        }
    };

    struct ScriptComponent
    {
        std::string className;

        ScriptComponent() = default;
        ScriptComponent(const ScriptComponent&) = default;

        YAML::Node Serialize(Entity& entity) const;
        static ScriptComponent Deserialize(const YAML::Node& node, Entity& entity);
    };

    // Forward declaration
    class ScriptableEntity;

    struct NativeScriptComponent
    {
        ScriptableEntity* instance = nullptr;

        ScriptableEntity* (*instantiateScript)();
        void (*destroyScript)(NativeScriptComponent*);

        template <typename T>
        void Bind()
        {
            instantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
            destroyScript = [](NativeScriptComponent* nsc)
            {
                delete nsc->instance;
                nsc->instance = nullptr;
            };
        }
    };

    template <typename... Component>
    struct ComponentGroup {};

    using AllComponents = ComponentGroup<TransformComponent,
                                         MeshRendererComponent,
                                         CameraComponent,
                                         ScriptComponent,
                                         NativeScriptComponent>;
} // namespace Hazel