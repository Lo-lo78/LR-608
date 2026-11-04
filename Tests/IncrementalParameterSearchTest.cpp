// SPDX-License-Identifier: AGPL-3.0-or-later
// No JUCE dependency: exercise the very same portable search header as the UI.
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
// Regression for Win32 macro pollution (TEST165 LSampler).
#ifdef _WIN32
#ifndef min
#define min(a, b) would_break_std_min(a, b)
#endif
#ifndef max
#define max(a, b) would_break_std_max(a, b)
#endif
#endif // _WIN32
#include "IncrementalParameterSearch.h"
#ifdef _WIN32
#ifdef min
#error "IncrementalParameterSearch.h leaks Win32 min macro"
#endif
#ifdef max
#error "IncrementalParameterSearch.h leaks Win32 max macro"
#endif
#endif // _WIN32

// Unlike CHECK(), CHECK is enabled in Release (GitHub Actions uses NDEBUG).
#define CHECK(expression) do { \
    if (!(expression)) { \
        std::cerr << "FAIL line " << __LINE__ << ": " #expression << "\n"; \
        return 1; \
    } \
} while (false)

int main()
{
    using lr608::IncrementalParameterSearch;
    const std::vector<std::wstring> names { L"Filter Cutoff", L"Pitch Envelope", L"Pan", L"Ping Pong Pan Depth", L"Master Volume", L"Engine 1", L"Engine 2", L"FX Envelope Attack" };
    const auto label = [&names](int row) { return names.at(static_cast<std::size_t>(row)); };
    IncrementalParameterSearch search;
    // Prefix continuation may retain the current item.
    CHECK(search.next(L'p', 0, (int)names.size(), 1, 100, label) == 1);
    CHECK(search.next(L'i', 1, (int)names.size(), 1, 220, label) == 1);
    search.reset();
    CHECK(search.next(L'p', 1, (int)names.size(), 1, 300, label) == 2);
    CHECK(search.next(L'a', 2, (int)names.size(), 1, 390, label) == 2);
    // Whole name begins at preferred match, then fallback to internal word.
    search.reset();
    CHECK(search.next(L'e', 3, (int)names.size(), 1, 500, label) == 5);
    CHECK(search.next(L'n', 5, (int)names.size(), 1, 600, label) == 5);
    search.reset();
    CHECK(search.next(L'a', 6, (int)names.size(), 1, 700, label) == 7); // FX Envelope Attack
    search.reset();
    CHECK(search.next(L'1', 6, (int)names.size(), -1, 800, label) == 5); // Shift+1 reverse finds Engine 1
    search.reset();
    CHECK(search.next(L'2', 5, (int)names.size(), 1, 900, label) == 6);
    search.reset();
    CHECK(search.next(L'e', 4, (int)names.size(), 1, 1000, label) == 5);
    CHECK(search.next(L'e', 5, (int)names.size(), 1, 1100, label) == 6); // repeat cycles
    CHECK(search.next(L'e', 6, (int)names.size(), -1, 1200, label) == 5); // reverse cycles
    search.reset();
    CHECK(search.next(L'x', 0, (int)names.size(), 1, 1300, label) == -1); // no match
    CHECK(search.next(L'm', 0, (int)names.size(), 1, 1400, label) == 4); // no dead buffer
    CHECK(search.next(L'p', 4, (int)names.size(), 1, 2600, label) == 1); // > 1000ms resets
    CHECK(search.next(L'~', 1, (int)names.size(), 1, 2700, label) == -1);
    search.reset();
    // Return to start of list when the best matching item lies before current.
    CHECK(search.next(L'm', 7, (int)names.size(), 1, 3000, label) == 4);
    search.reset();
    CHECK(search.next(L'f', 4, (int)names.size(), -1, 3100, label) == 0);
    // Changes remain inside the UI. std::min/max remain usable.
    CHECK(std::min(2, 3) == 2 && std::max(2, 3) == 3);
    std::cout << "PASS LR608 incremental search: forward/reverse, digits, words, cycling, timeout, macros\n";
}
