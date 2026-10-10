#include "core/render/FingerTable.h"

namespace dizi {

namespace {

constexpr int kRowCount = 7;
constexpr int kCovered = 1;

using HolePattern = std::array<int, kHoleCount>;

struct PatternRow {
    int number;
    int accidental;
    const char *pitchName;
    HolePattern holes;
};

const std::array<PatternRow, kRowCount> &patternTable() {
    static const std::array<PatternRow, kRowCount> rows{{
            {1, 0, "C5", {kCovered, 0, 0, 0, 0, 0}},
            {2, 0, "D5", {0, 0, 0, 0, 0, 0}},
            {3, 0, "E5", {0, 0, 0, 0, 0, kCovered}},
            {4, 0, "F5", {kCovered, kCovered, kCovered, kCovered, kCovered, kCovered}},
            {5, 0, "G5", {kCovered, kCovered, kCovered, kCovered, kCovered, kCovered}},
            {6, 0, "A5", {kCovered, kCovered, kCovered, kCovered, kCovered, 0}},
            {7, 0, "B5", {kCovered, kCovered, kCovered, kCovered, 0, 0}},
    }};
    return rows;
}

bool matches(const PatternRow &row, const Note &note) {
    return row.number == note.number &&
           row.accidental == static_cast<int>(note.accidental);
}

Fingering makeFingering(const PatternRow &row, const Note &note) {
    Fingering fingering;
    fingering.pitchName = row.pitchName;
    fingering.overblown = note.octave != 0;
    for (int index = 0; index < kHoleCount; ++index) {
        const std::size_t slot = static_cast<std::size_t>(index);
        fingering.holes.at(slot) = row.holes.at(slot) != 0 ? HoleState::Covered : HoleState::Open;
    }
    return fingering;
}

Fingering unknownFingering(const Note &note) {
    Fingering fingering;
    fingering.pitchName = "?";
    fingering.overblown = note.octave >= 1;
    fingering.playable = false;
    return fingering;
}

}

FingerTable::FingerTable()
    : m_keyName("\u7b52\u97f3\u4f5c5") {
}

const std::string &FingerTable::keyName() const {
    return m_keyName;
}

Fingering FingerTable::lookup(const Note &note) const {
    if (note.rest) {
        return unknownFingering(note);
    }
    for (const PatternRow &row : patternTable()) {
        if (matches(row, note)) {
            return makeFingering(row, note);
        }
    }
    return unknownFingering(note);
}

}
