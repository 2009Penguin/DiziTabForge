#ifndef DIZITABFORGE_CORE_RENDER_NOTEFORMATTER_H
#define DIZITABFORGE_CORE_RENDER_NOTEFORMATTER_H

#include <string>

#include "core/model/Note.h"

namespace note_format {

std::string glyph(const dizi::Note &note);

std::string asciiGlyph(const dizi::Note &note);

std::string durationLabel(const dizi::Note &note);

std::string accidentalToken(const dizi::Note &note);

}

#endif
