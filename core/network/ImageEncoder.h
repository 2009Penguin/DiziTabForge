#ifndef DIZITABFORGE_CORE_NETWORK_IMAGEENCODER_H
#define DIZITABFORGE_CORE_NETWORK_IMAGEENCODER_H

#include <QString>

namespace dizi {

struct EncodedImage {
    QString fileName;
    QString mimeType;
    QString dataUri;
};

class ImageEncoder {
public:
    bool encode(const QString &path, EncodedImage &target, QString &error) const;
};

}

#endif
