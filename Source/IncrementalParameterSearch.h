#pragma once
#include <cstdint>
#include <cwctype>
#include <string>
#ifdef _WIN32
// Win32's min/max function-like macros break std::min/std::max in the JUCE
// translation units that include this header.  Keep this header self-contained:
// unlike the TEST165 console test, the VST3 target did not define NOMINMAX.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
// Also guard against a Windows header having been included earlier without
// NOMINMAX.  Never leak these macros to callers of the search helper.
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif

// Search behavior adapted from LReaParams 0.0.63 TypeAhead.h and
// cross-checked against LSampler-24 TEST165 and LJuno-116 TEST120.
// UI only. Match beginnings of labels or WORDS, prefer whole-label start,
// cycle with repeated single key, reverse with Shift, wrap and 1000 ms timeout.
// All matching is Unicode (wide characters); no parameter or preset state.
namespace lr608 {
class IncrementalParameterSearch final {
public:
    static constexpr std::uint32_t intervalMs = 1000;
    void reset() noexcept { buffer_.clear(); lastTime_ = 0; }
    const std::wstring& text() const noexcept { return buffer_; }

    template <class Label>
    int next(wchar_t character, int current, int count, int direction,
             std::uint32_t eventTime, Label&& label, bool repeated = false) {
        const bool backwards = direction < 0;
        if (repeated || character < L'!' || count <= 0) return -1;
        const wchar_t c = lower(character);
        const bool contiguous = !buffer_.empty() &&
            static_cast<std::uint32_t>(eventTime - lastTime_) <= intervalMs;
        const bool sameLetterCycle = contiguous && buffer_.size() == 1 && buffer_[0] == c;
        const std::wstring candidate = contiguous && !sameLetterCycle ? buffer_ + c : std::wstring(1, c);
        // Single letters cycle from the NEXT entry. A multi-character match
        // must include the CURRENT entry so typing F then I does not skip the
        // Filter entry already reached by F.
        int match = locate(candidate, current, count, backwards,
                           candidate.size() == 1, label);
        if (match < 0 && candidate.size() > 1) {
            // An impossible prefix starts a new search with the last letter.
            // Never leave the keyboard locked to a misspelled partial word.
            match = locate(std::wstring(1, c), current, count, backwards, true, label);
            buffer_.assign(1, c);
        } else if (match >= 0) buffer_ = candidate;
        else buffer_.clear();
        lastTime_ = eventTime;
        return match;
    }
private:
    static wchar_t lower(wchar_t c) noexcept {
#ifdef _WIN32
        // Respect Windows Unicode case conversion also for Italian accents.
        CharLowerBuffW(&c, 1);
        return c;
#else
        return static_cast<wchar_t>(std::towlower(static_cast<wint_t>(c)));
#endif
    }
    // Rank beginning-of-label above beginning-of-word; never match an
    // arbitrary substring, because that makes quick typing unpredictable.
    static int rank(const std::wstring& label, const std::wstring& prefix) {
        if (prefix.empty()) return -1;
        for (std::size_t i = 0; i + prefix.size() <= label.size(); ++i) {
            const bool atWord = i == 0 || !std::iswalnum(static_cast<wint_t>(label[i - 1]));
            if (!atWord) continue;
            bool matches = true;
            for (std::size_t j = 0; j < prefix.size(); ++j) {
                if (lower(label[i + j]) != prefix[j]) { matches = false; break; }
            }
            if (matches) return i == 0 ? 0 : 1;
        }
        return -1;
    }
    template <class Label>
    static int locate(const std::wstring& prefix, int current, int count,
                      bool backwards, bool cycle, Label&& label) {
        int fallback = -1;
        for (int offset = cycle ? 1 : 0; offset < count + (cycle ? 1 : 0); ++offset) {
            const int index = backwards ? (current + count - (offset % count)) % count
                                        : (current + offset) % count;
            const int quality = rank(label(index), prefix);
            if (quality == 0) return index;
            if (quality == 1 && fallback < 0) fallback = index;
        }
        return fallback;
    }
    std::wstring buffer_;
    std::uint32_t lastTime_ = 0;
};
} // namespace lr608
