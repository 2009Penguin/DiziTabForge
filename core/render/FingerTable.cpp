#include "core/render/FingerTable.h"

#include <algorithm>

namespace dizi {

namespace {

constexpr int kHoleCountLocal = 6;
constexpr int kRowCount = 21;
constexpr int kRegisterCount = 3;
constexpr int kCovered = 1;

struct PatternRow {
    int number;
    int accidental;
    int registerMask;
    const char *pitchName;
    std::array<int, kRegisterCount> holes;
};

const std::array<PatternRow, kRowCount> &patternTable() {
    static const std::array<PatternRow, kRowCount> rows{{
            {1, 0, 0b111, "C5", {kCovered, 0, 0, 0, 0, 0}},
            {2, 0, 0b111, "D5", {0, 0, 0, 0, 0, 0}},
            {3, 0, 0b111, "E5", {0, 0, 0, 0, 0, kCovered}},
            {4, 0, 0b111, "F5", {kCovered, kCovered, kCovered, kCovered, kCovered, kCovered}},
            {5, 0, 0b111, "G5", {kCovered, kCovered, kCovered, kCovered, kCovered, kCovered}},
            {6, 0, 0b111, "A5", {kCovered, kCovered, kCovered, kCovered, kCovered, 0}},
            {7, 0, 0b111, "B5", {kCovered, kCovered, kCovered, kCovered, 0, 0}},

            {1, 0, 0b010, "C4", {kCovered, kCovered, kCovered, kCovered, kCovered, kCovered}},
            {2, 0, 0b010, "D4", {kCovered, kCovered, kCovered, kCovered, kCovered, 0}},
            {3, 0, 0b010, "E4", {kCovered, kCovered, kCovered, kCovered, 0, 0}},
            {4, 0, 0b010, "F4", {kCovered, kCovered, kCovered, 0, 0, 0}},
            {5, 0, 0b010, "G4", {kCovered, kCovered, 0, 0, 0, 0}},
            {6, 0, 0b010, "A4", {kCovered, 0, 0, 0, 0, 0}},
            {7, 0, 0b010, "B4", {0, 0, 0, 0, 0, 0}},

            {3, 0, 0b100, "E6", {kCovered, kCovered, kCovered, kCovered, kCovered, 0}},
            {5, 0, 0b100, "G6", {kCovered, kCovered, kCovered, kCovered, kCovered, kCovered}},
            {6, 0, 0b100, "A6", {kCovered, kCovered, kCovered, kCovered, kCovered, kCovered}},
            {7, 0, 0b100, "B6", {kCovered, kCovered, kCovered, kCovered, kCovered, 0}},
    }};
    return rows;
}

int registerBit(int octave) {
    if (octave <= -1) {
        return 0b001;
    }
    if (octave >= 1) {
        return 0b100;
    }
    return 0b010;
}

bool matches(const PatternRow &row, const Note &note, int bit) {
    return row.number == note.number &&
           row.accidental == static_cast<int>(note.accidental) &&
           (row.registerMask & bit) != 0;
}

Fingering makeFingering(const PatternRow &row) {
    Fingering fingering;
    fingering.pitchName = row.pitchName;
    for (int index = 0; index < kHoleCountLocal; ++index) {
        fingering.holes.at(static_cast<std::size_t>(index)) = row.holes.at(
                static_cast<std::size_t>(index)) != 0;
    }
    return fingering;
}

Fingering unknownFingering(const Note &note) {
    Fingering fingering;
    fingering.pitchName = "?";
    fingering.overblown = note.octave >= 1;
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
    const int bit = registerBit(note.octave);
    for (const PatternRow &row : patternTable()) {
        if (matches(row, note, bit)) {
            return makeFingering(row);
        }
    }
    return unknownFingering(note);
}

}
