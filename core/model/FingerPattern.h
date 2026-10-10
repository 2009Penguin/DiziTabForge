#ifndef DIZITABFORGE_CORE_MODEL_FINGERPATTERN_H
#define DIZITABFORGE_CORE_MODEL_FINGERPATTERN_H

#include <array>
#include <string>

namespace dizi {

constexpr int kHoleCount = 6;

enum class HoleState {
    Open = 0,
    Covered = 1
};

struct FingerPattern {
    std::string pitchName;
    std::array<HoleState, kHoleCount> holes{};
    bool overblown = false;
    bool playable = true;
};

}

#endif
