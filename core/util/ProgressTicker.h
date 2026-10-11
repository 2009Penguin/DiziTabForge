#ifndef DIZITABFORGE_CORE_UTIL_PROGRESSTICKER_H
#define DIZITABFORGE_CORE_UTIL_PROGRESSTICKER_H

#include <QObject>
#include <QTimer>
#include <QtGlobal>

namespace dizi {

class ProgressTicker : public QObject {
    Q_OBJECT

public:
    explicit ProgressTicker(QObject *parent = nullptr);

    void begin();

    void reportUpload(qint64 sent, qint64 total);

    void complete();

signals:
    void percentChanged(int percent);

private slots:
    void advance();

private:
    void publish(int percent);

    QTimer m_timer;
    int m_percent = 0;
    bool m_running = false;
};

}

#endif
