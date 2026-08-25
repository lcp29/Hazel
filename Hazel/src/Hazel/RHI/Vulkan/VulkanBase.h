// Declares the Vulkan base backend.
// Created: 2026-03-16.

#pragma once

#define RHI_VK_CLASS_IMPL(className) template <> class className##Impl<RHIBackend::Vulkan>

#define RHI_VK_FUNC_IMPL(className, funcName) className##Impl<RHIBackend::Vulkan>::funcName