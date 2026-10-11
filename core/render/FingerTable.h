#ifndef DIZITABFORGE_CORE_RENDER_FINGERTABLE_H
#define DIZITABFORGE_CORE_RENDER_FINGERTABLE_H

#include <array>
#include <string>

#include "core/model/FingerPattern.h"
#include "core/model/Note.h"

namespace dizi {

using Fingering = FingerPattern;

class FingerTable {
public:
    FingerTable();

    Fingering lookup(const Note &note) const;

    const std::string &keyName() const;

private:
    std::string m_keyName;
};

std::string playableRangeText();

}

#endif
