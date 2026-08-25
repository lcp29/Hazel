// Declares GPU-facing geometry structures.
// Created: 2026-04-07.

#pragma once

template <typename T, int dataSize> struct Padded
{
    T data{};
    uint8_t padding[dataSize - sizeof(T)]{};
};
