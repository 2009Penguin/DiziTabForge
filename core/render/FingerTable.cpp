#include "core/render/FingerTable.h"

namespace dizi {

namespace {

constexpr int kRowCount = 16;
constexpr int kCovered = 1;

using HolePattern = std::array<int, kHoleCount>;

struct PatternRow {
    int octave;
    int number;
    HolePattern holes;
};

const std::array<PatternRow, kRowCount> &patternTable() {
    static const std::array<PatternRow, kRowCount> rows{{
            {-1, 5, {kCovered, kCovered, kCovered, kCovered, kCovered, kCovered}},
            {-1, 6, {kCovered, kCovered, kCovered, kCovered, kCovered, 0}},
            {-1, 7, {kCovered, kCovered, kCovered, kCovered, 0, 0}},
            {0, 1, {kCovered, kCovered, kCovered, 0, 0, 0}},
            {0, 2, {kCovered, kCovered, 0, 0, 0, 0}},
            {0, 3, {kCovered, 0, 0, 0, 0, 0}},
            {0, 4, {0, kCovered, kCovered, 0, 0, 0}},
            {0, 5, {0, kCovered, kCovered, kCovered, kCovered, kCovered}},
            {0, 6, {kCovered, kCovered, kCovered, kCovered, kCovered, 0}},
            {0, 7, {kCovered, kCovered, kCovered, kCovered, 0, 0}},
            {1, 1, {kCovered, kCovered, kCovered, 0, 0, 0}},
            {1, 2, {kCovered, kCovered, 0, 0, 0, 0}},
            {1, 3, {kCovered, 0, 0, 0, 0, 0}},
            {1, 4, {0, kCovered, kCovered, kCovered, kCovered, 0}},
            {1, 5, {0, kCovered, kCovered, kCovered, kCovered, kCovered}},
            {1, 6, {kCovered, kCovered, 0, kCovered, kCovered, 0}},
    }};
    return rows;
}

bool matches(const PatternRow &row, const Note &note) {
    return row.octave == note.octave && row.number == note.number &&
           note.accidental == Accidental::Natural;
}

Fingering makeFingering(const PatternRow &row) {
    Fingering fingering;
    for (int index = 0; index < kHoleCount; ++index) {
        const std::size_t slot = static_cast<std::size_t>(index);
        fingering.holes.at(slot) = row.holes.at(slot) != 0 ? HoleState::Covered : HoleState::Open;
    }
    return fingering;
}

Fingering unknownFingering() {
    Fingering fingering;
    fingering.playable = false;
    return fingering;
}

}

std::string playableRangeText() {
    std::string text;
    for (const PatternRow &row : patternTable()) {
        if (!text.empty()) {
            text += ' ';
        }
        text += static_cast<char>('0' + row.number);
        if (row.octave <= -1) {
            text += ',';
        } else if (row.octave >= 1) {
            text += '\'';
        }
    }
    return text;
}

FingerTable::FingerTable()
    : m_keyName("\u7b52\u97f3\u4f5c5") {
}

const std::string &FingerTable::keyName() const {
    return m_keyName;
}

Fingering FingerTable::lookup(const Note &note) const {
    if (note.rest) {
        return unknownFingering();
    }
    for (const PatternRow &row : patternTable()) {
        if (matches(row, note)) {
            return makeFingering(row);
        }
    }
    return unknownFingering();
}

}
