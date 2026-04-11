//
// Created by helmholtz on 2026/3/20.
//

#include "Components.h"
#include "Hazel/Scene/Entity.h"

namespace Hazel
{
#define WRITE_SCRIPT_FIELD(fieldType, type)                                                                            \
    case ScriptFieldType::fieldType:                                                                                   \
        fieldNode["Data"] = scriptField.GetValue<type>();                                                              \
        break;

    YAML::Node ScriptComponent::Serialize(Entity& entity) const
    {
        YAML::Node node;
        node["ClassName"] = className;
        auto entityClass = ScriptEngine::GetEntityClass(className);
        if (const auto& fields = entityClass->GetFields(); fields.size() > 0)
        {
            YAML::Node fieldsNode;
            auto& entityFields = ScriptEngine::GetScriptFieldMap(entity);
            for (const auto& [name, field] : fields)
            {
                if (!fields.contains(name))
                    continue;

                YAML::Node fieldNode;
                fieldNode["Name"] = name;
                fieldNode["Type"] = Utils::ScriptFieldTypeToString(field.Type);

                auto& scriptField = entityFields.at(name);

                switch (field.Type)
                {
                    WRITE_SCRIPT_FIELD(Float, float)
                    WRITE_SCRIPT_FIELD(Double, double)
                    WRITE_SCRIPT_FIELD(Bool, bool)
                    WRITE_SCRIPT_FIELD(Char, char)
                    WRITE_SCRIPT_FIELD(Byte, int8_t)
                    WRITE_SCRIPT_FIELD(Short, int16_t)
                    WRITE_SCRIPT_FIELD(Int, int32_t)
                    WRITE_SCRIPT_FIELD(Long, int64_t)
                    WRITE_SCRIPT_FIELD(UByte, uint8_t)
                    WRITE_SCRIPT_FIELD(UShort, uint16_t)
                    WRITE_SCRIPT_FIELD(UInt, uint32_t)
                    WRITE_SCRIPT_FIELD(ULong, uint64_t)
                    WRITE_SCRIPT_FIELD(Vector2, glm::vec2)
                    WRITE_SCRIPT_FIELD(Vector3, glm::vec3)
                    WRITE_SCRIPT_FIELD(Vector4, glm::vec4)
                    WRITE_SCRIPT_FIELD(Entity, UUID)
                    default:
                        break;
                }
                fieldsNode.push_back(fieldNode);
            }
            fieldsNode.SetStyle(YAML::EmitterStyle::Block);
            node["ScriptFields"] = fieldsNode;
        }

        return node;
    }

#undef WRITE_SCRIPT_FIELD

#define READ_SCRIPT_FIELD(FieldType, Type)                                                                             \
    case ScriptFieldType::FieldType:                                                                                   \
        {                                                                                                              \
            Type data = scriptField["Data"].as<Type>();                                                                \
            fieldInstance.SetValue(data);                                                                              \
            break;                                                                                                     \
        }

    ScriptComponent ScriptComponent::Deserialize(const YAML::Node& node, Entity& entity)
    {
        ScriptComponent sc;
        sc.className = node["ClassName"].as<std::string>();

        if (auto scriptFields = node["ScriptFields"])
        {
            if (auto entityClass = ScriptEngine::GetEntityClass(sc.className))
            {
                const auto& fields = entityClass->GetFields();
                auto& entityFields = ScriptEngine::GetScriptFieldMap(entity);

                for (auto scriptField : scriptFields)
                {
                    auto name = scriptField["Name"].as<std::string>();
                    auto typeString = scriptField["Type"].as<std::string>();
                    ScriptFieldType type = Utils::ScriptFieldTypeFromString(typeString);

                    ScriptFieldInstance& fieldInstance = entityFields[name];

                    if (!fields.contains(name))
                        continue;

                    fieldInstance.Field = fields.at(name);

                    switch (type)
                    {
                        READ_SCRIPT_FIELD(Float, float)
                        READ_SCRIPT_FIELD(Double, double)
                        READ_SCRIPT_FIELD(Bool, bool)
                        READ_SCRIPT_FIELD(Char, char)
                        READ_SCRIPT_FIELD(Byte, int8_t)
                        READ_SCRIPT_FIELD(Short, int16_t)
                        READ_SCRIPT_FIELD(Int, int32_t)
                        READ_SCRIPT_FIELD(Long, int64_t)
                        READ_SCRIPT_FIELD(UByte, uint8_t)
                        READ_SCRIPT_FIELD(UShort, uint16_t)
                        READ_SCRIPT_FIELD(UInt, uint32_t)
                        READ_SCRIPT_FIELD(ULong, uint64_t)
                        READ_SCRIPT_FIELD(Vector2, glm::vec2)
                        READ_SCRIPT_FIELD(Vector3, glm::vec3)
                        READ_SCRIPT_FIELD(Vector4, glm::vec4)
                        READ_SCRIPT_FIELD(Entity, UUID)
                        default:
                            break;
                    }
                }
            }
        }

        return sc;
    }

#undef READ_SCRIPT_FIELD
} // namespace Hazel