//
// Created by helmholtz on 2026/3/13.
//

#pragma once

#include "RHIDesc.h"
#include "RHIInstance.h"

namespace Hazel
{
    std::optional<Scope<RHIInstance>> CreateInstance(const RHIInstanceDesc &desc);
} // Hazel
