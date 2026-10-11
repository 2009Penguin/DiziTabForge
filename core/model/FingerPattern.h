#ifndef DIZITABFORGE_CORE_MODEL_FINGERPATTERN_H
#define DIZITABFORGE_CORE_MODEL_FINGERPATTERN_H

#include <array>

namespace dizi {

constexpr int kHoleCount = 6;

enum class HoleState {
    Open = 0,
    Covered = 1
};

struct FingerPattern {
    std::array<HoleState, kHoleCount> holes{};
    bool playable = true;
};

}

#endif
