//
// Created by helmholtz on 2026/3/13.
//

#pragma once

#include "../Core/Base.h"
#include "RHIAdapter.h"

#include <vector>

namespace Hazel
{
    class RHIInstance
    {
    public:
        virtual bool IsValid() const { return false; }

        virtual std::vector<Ref<RHIAdapter>> GetAdapters() = 0;

        virtual ~RHIInstance() = default;
    };
} // Hazel
