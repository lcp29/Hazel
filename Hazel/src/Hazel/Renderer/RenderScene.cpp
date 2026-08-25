// Implements render scene.
// Created: 2026-03-29.

#include "RenderScene.h"

#include "Renderer.h"

namespace Aster
{
    RenderScene::RenderScene(Hazel::Renderer* renderer)
        : m_Renderer(renderer)
    {}

    void RenderScene::Update(const std::vector<RenderSceneUpdatePayload>& payload)
    {
        std::unique_lock lock(m_RenderObjectsMutex);
        for (const auto& object : payload)
        {
            switch (object.type)
            {
                case RenderSceneUpdatePayload::Type::ChangeTransform:
                    {
                        if (m_RenderObjects.contains(object.entity))
                        {
                            m_RenderObjects.at(object.entity)->transform = object.changeTransform.transform;
                        }
                        break;
                    }
                case RenderSceneUpdatePayload::Type::ChangeMaterial:
                    {
                        if (m_RenderObjects.contains(object.entity))
                        {
                            m_RenderObjects.at(object.entity)->material = object.changeMaterial.material;
                        }
                        break;
                    }
                case RenderSceneUpdatePayload::Type::ChangeMesh:
                    {
                        if (m_RenderObjects.contains(object.entity))
                        {
                            m_RenderObjects.at(object.entity)->mesh = object.changeMesh.mesh;
                        }
                        break;
                    }
                case RenderSceneUpdatePayload::Type::Add:
                    {
                        auto renderObject = std::make_unique<RenderObject>(object.add.renderObject);
                        auto* pointer = renderObject.get();
                        m_RenderObjects[object.entity] = std::move(renderObject);
                        m_RenderObjectsUnsorted.insert(pointer);
                        break;
                    }
                case RenderSceneUpdatePayload::Type::Remove:
                    {
                        m_RenderObjects.erase(object.entity);
                        break;
                    }
            }
        }
    }

    void RenderScene::Clear()
    {
        std::unique_lock lock(m_RenderObjectsMutex);
        m_RenderObjects.clear();
    }

    void RenderScene::SortRenderObjectShader()
    {
        std::unique_lock lock(m_RenderObjectsMutex);

        for (auto it = m_RenderObjectsUnsorted.begin(); it != m_RenderObjectsUnsorted.end();)
        {
            auto material = m_Renderer->ResolveGPUAsset((*it)->material, AssetType::Material);
            if (material.asset)
            {
                auto itInMultimap = m_RenderObjectsSortedByShader.emplace(
                    static_cast<CachedMaterial*>(material.asset)->GetShader(), *it);
                m_RenderObjectLocationInMap[*it] = itInMultimap;
                it = m_RenderObjectsUnsorted.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
} // namespace Aster
