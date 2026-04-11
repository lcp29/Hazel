//
// Created by helmholtz on 2026/4/7.
//

#pragma once

template <typename T, int dataSize>
struct Padded
{
    T data;
    uint8_t padding[dataSize - sizeof(T)];
};