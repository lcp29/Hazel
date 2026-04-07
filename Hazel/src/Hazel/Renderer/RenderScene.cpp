//
// Created by helmholtz on 2026/3/29.
//

#include "RenderScene.h"

namespace Hazel
{
    RenderScene::RenderScene(Renderer* renderer): m_Renderer(renderer) {}

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
                        m_RenderObjects.at(object.entity).transform = object.changeTransform.transform;
                    }
                    break;
                }
                case RenderSceneUpdatePayload::Type::ChangeMaterial:
                {
                    if (m_RenderObjects.contains(object.entity))
                    {
                        m_RenderObjects.at(object.entity).material = object.changeMaterial.material;
                    }
                    break;
                }
                case RenderSceneUpdatePayload::Type::ChangeMesh:
                {
                    if (m_RenderObjects.contains(object.entity))
                    {
                        m_RenderObjects.at(object.entity).mesh = object.changeMesh.mesh;
                    }
                    break;
                }
                case RenderSceneUpdatePayload::Type::Add:
                {
                    m_RenderObjects[object.entity] = object.add.renderObject;
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
} // Hazel