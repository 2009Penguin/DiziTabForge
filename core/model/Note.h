#ifndef DIZITABFORGE_CORE_MODEL_NOTE_H
#define DIZITABFORGE_CORE_MODEL_NOTE_H

#include <string>
#include <vector>

namespace dizi {

enum class Accidental {
    Natural = 0,
    Sharp = 1,
    Flat = -1
};

struct Note {
    int number = 0;
    Accidental accidental = Accidental::Natural;
    int octave = 0;
    bool rest = false;
    int beats = 1;

    Note() = default;

    Note(int noteNumber, Accidental noteAccidental, int noteOctave, bool isRest, int beatCount)
        : number(noteNumber),
          accidental(noteAccidental),
          octave(noteOctave),
          rest(isRest),
          beats(beatCount) {
    }
};

using Score = std::vector<Note>;

}

#endif
