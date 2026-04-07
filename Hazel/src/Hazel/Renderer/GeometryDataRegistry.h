//
// Created by helmholtz on 2026/4/7.
//

#pragma once
#include <cstdint>

namespace Hazel
{
    class Renderer;

    class GeometryDataRegistry
    {
    public:
        GeometryDataRegistry() = delete;

        GeometryDataRegistry(Renderer* renderer);

        uint32_t RegisterMesh();

    private:
        Renderer* m_Renderer;
        
    };
} // Hazel