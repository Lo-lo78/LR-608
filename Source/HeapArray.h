// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <array>
#include <cstddef>
#include <memory>

namespace lr608
{
template <typename T, std::size_t Size>
class HeapArray
{
public:
    HeapArray() : values(std::make_unique<std::array<T, Size>>()) {}
    T& operator[] (std::size_t index) noexcept { return (*values)[index]; }
    const T& operator[] (std::size_t index) const noexcept { return (*values)[index]; }
    void fill (const T& value) { values->fill(value); }

private:
    std::unique_ptr<std::array<T, Size>> values;
};
}
