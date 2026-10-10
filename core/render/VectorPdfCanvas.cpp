#include "core/render/VectorPdfCanvas.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include <algorithm>
#include <string>
#include <vector>

#include "core/render/CjkFontFile.h"
#include "core/render/EmbeddedFont.h"
#include "core/render/PdfWriter.h"
#include "core/render/TrueTypeFace.h"

namespace dizi {

namespace {

constexpr double kDefaultLineWidth = 0.6;

QStringList bundledFontPaths() {
    const QDir base(QCoreApplication::applicationDirPath());
    const QStringList names{QStringLiteral("WenYuanSerifSC-Regular.ttf"),
                            QStringLiteral("WenYuanSerifSC-Medium.ttf")};
    QStringList paths;
    for (const QString &name : names) {
        paths.append(base.filePath(QStringLiteral("fonts/") + name));
        paths.append(base.filePath(QStringLiteral("res/fonts/") + name));
    }
    return paths;
}

QStringList systemFontPaths() {
    QStringList directories;
    const QByteArray windir = qgetenv("WINDIR");
    if (!windir.isEmpty()) {
        directories.append(QDir(QString::fromLocal8Bit(windir)).filePath(QStringLiteral("Fonts")));
    }
    directories.append(QStringLiteral("/usr/share/fonts"));
    directories.append(QStringLiteral("/usr/share/fonts/truetype"));
    const QStringList names{QStringLiteral("simhei.ttf"), QStringLiteral("Deng.ttf"),
                            QStringLiteral("simkai.ttf")};
    QStringList paths;
    for (const QString &directory : directories) {
        for (const QString &name : names) {
            paths.append(QDir(directory).filePath(name));
        }
    }
    return paths;
}

bool openChineseFont(CjkFontFile &font) {
    const QString resource = QStringLiteral(":/fonts/WenYuanSerifSC-Regular.ttf");
    QFile file(resource);
    if (file.open(QIODevice::ReadOnly)) {
        const QByteArray bytes = file.readAll();
        file.close();
        const std::vector<std::uint8_t> data(bytes.constData(), bytes.constData() + bytes.size());
        if (font.openFromMemory(data, resource.toStdString())) {
            return true;
        }
    }
    QStringList paths = bundledFontPaths();
    paths.append(systemFontPaths());
    for (const QString &candidate : paths) {
        if (font.open(candidate.toLocal8Bit().toStdString())) {
            return true;
        }
    }
    return false;
}

std::vector<char32_t> collectCodepoints(const QStringList &texts) {
    std::vector<char32_t> codepoints;
    for (char32_t code = 0x20; code <= 0x7E; ++code) {
        codepoints.push_back(code);
    }
    codepoints.push_back(0x2018);
    codepoints.push_back(0x2019);
    for (const QString &text : texts) {
        const std::string utf8 = text.toUtf8().toStdString();
        for (char32_t code : TrueTypeFace::decodeUtf8(utf8)) {
            codepoints.push_back(code);
        }
    }
    std::sort(codepoints.begin(), codepoints.end());
    codepoints.erase(std::unique(codepoints.begin(), codepoints.end()), codepoints.end());
    return codepoints;
}

bool buildEmbeddedFont(const QStringList &texts, EmbeddedFont &embedded) {
    CjkFontFile font;
    if (!openChineseFont(font)) {
        return false;
    }
    std::string problem;
    if (!font.buildFor(collectCodepoints(texts), embedded, problem)) {
        return false;
    }
    return embedded.usable;
}

bool isAscii(const QString &text) {
    for (const QChar symbol : text) {
        if (symbol.unicode() >= 0x80) {
            return false;
        }
    }
    return true;
}

QString renderableText(const PdfWriter &writer, const QString &text) {
    QString rendered;
    for (const QChar symbol : text) {
        if (symbol.unicode() < 0x80 || writer.canRenderCjk(symbol.unicode())) {
            rendered.append(symbol);
        } else {
            rendered.append(QLatin1Char('?'));
        }
    }
    return rendered;
}

}

VectorPdfCanvas::VectorPdfCanvas() = default;

VectorPdfCanvas::~VectorPdfCanvas() = default;

bool VectorPdfCanvas::prepare(const QStringList &texts, const QString &outputPath, QSizeF pageSize,
                              QString &error) {
    m_outputPath = outputPath;
    m_pageHeight = pageSize.height();
    m_writer = std::make_unique<PdfWriter>(pageSize.width(), pageSize.height());
    m_writer->setLineWidth(kDefaultLineWidth);
    EmbeddedFont embedded;
    if (buildEmbeddedFont(texts, embedded)) {
        m_writer->setEmbeddedFont(embedded);
    }
    error.clear();
    return true;
}

void VectorPdfCanvas::addPage() {
    if (m_writer != nullptr) {
        m_writer->addPage();
    }
}

void VectorPdfCanvas::setFontSize(double points) {
    if (m_writer != nullptr && points > 0.0) {
        m_writer->setFontSize(points);
    }
}

void VectorPdfCanvas::setLineWidth(double points) {
    if (points > 0.0) {
        m_lineWidth = points;
    }
    if (m_writer != nullptr) {
        m_writer->setLineWidth(m_lineWidth);
    }
}

void VectorPdfCanvas::drawText(double left, double baseline, const QString &text) {
    if (m_writer == nullptr || text.isEmpty()) {
        return;
    }
    const QString safe = renderableText(*m_writer, text);
    if (safe.isEmpty()) {
        return;
    }
    const double y = m_pageHeight - baseline;
    if (isAscii(safe) || !m_writer->hasEmbeddedFont()) {
        m_writer->drawText(left, y, safe.toStdString());
        return;
    }
    m_writer->drawCjkText(left, y, safe.toUtf8().toStdString());
}

void VectorPdfCanvas::drawHole(double centerX, double centerY, double radius, bool filled) {
    if (m_writer == nullptr || radius <= 0.0) {
        return;
    }
    m_writer->drawCircle(centerX, m_pageHeight - centerY, radius, filled);
}

bool VectorPdfCanvas::finish(QString &error) {
    if (m_writer == nullptr) {
        error = QStringLiteral("PDF 写入器没有初始化。");
        return false;
    }
    std::string problem;
    if (!m_writer->save(m_outputPath.toStdString(), problem)) {
        error = QStringLiteral("写入 PDF 失败：%1").arg(QString::fromStdString(problem));
        return false;
    }
    m_writer.reset();
    error.clear();
    return true;
}

}
