#include "core/render/NoteFormatter.h"

#include <array>

namespace note_format {

namespace {

constexpr int kUnsupported = 0;
constexpr int kNatural = 1;
constexpr int kSharp = 2;
constexpr int kFlat = 3;

int accidentalKind(const dizi::Note &note) {
    if (note.number < 1 || note.number > 7) {
        return kUnsupported;
    }
    switch (note.accidental) {
        case dizi::Accidental::Natural:
            return kNatural;
        case dizi::Accidental::Sharp:
            return kSharp;
        case dizi::Accidental::Flat:
            return kFlat;
    }
    return kUnsupported;
}

std::string digitOf(const dizi::Note &note) {
    if (note.number < 1 || note.number > 7) {
        return std::string();
    }
    return std::string(1, static_cast<char>('0' + note.number));
}

}

std::string asciiGlyph(const dizi::Note &note) {
    if (note.rest) {
        return "0";
    }
    std::string text = digitOf(note);
    if (accidentalKind(note) == kSharp) {
        text += "#";
    }
    if (accidentalKind(note) == kFlat) {
        text += "b";
    }
    if (note.octave >= 1) {
        text += "'";
    }
    if (note.octave <= -1) {
        text += ",";
    }
    return text;
}

std::string glyph(const dizi::Note &note) {
    if (note.rest) {
        return "\xe2\x80\x97";
    }
    std::string text = digitOf(note);
    const int kind = accidentalKind(note);
    if (kind == kSharp) {
        text += "\xe2\x99\xaf";
    }
    if (kind == kFlat) {
        text += "\xe2\x99\xad";
    }
    if (note.octave >= 1) {
        text += "\xe2\x80\xb2";
    }
    if (note.octave <= -1) {
        text += "\xe2\x80\x9a";
    }
    return text;
}

std::string accidentalToken(const dizi::Note &note) {
    const int kind = accidentalKind(note);
    if (kind == kSharp) {
        return "#";
    }
    if (kind == kFlat) {
        return "b";
    }
    return std::string();
}

std::string durationLabel(const dizi::Note &note) {
    if (note.beats < 1) {
        return std::string();
    }
    if (note.beats == 1) {
        return std::string();
    }
    return std::string(static_cast<std::size_t>(note.beats - 1), '-');
}

}
