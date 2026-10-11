#include "core/util/ProgressTicker.h"

namespace dizi {

namespace {

constexpr int kUploadCeiling = 40;
constexpr int kWaitingCeiling = 99;
constexpr int kTickMilliseconds = 400;
constexpr int kRampDivisor = 18;

}

ProgressTicker::ProgressTicker(QObject *parent)
    : QObject(parent) {
    m_timer.setInterval(kTickMilliseconds);
    connect(&m_timer, &QTimer::timeout, this, &ProgressTicker::advance);
}

void ProgressTicker::begin() {
    m_running = true;
    m_percent = 0;
    m_timer.start();
    emit percentChanged(m_percent);
}

void ProgressTicker::reportUpload(qint64 sent, qint64 total) {
    if (!m_running || total <= 0) {
        return;
    }
    const int uploaded = static_cast<int>(sent * kUploadCeiling / total);
    if (uploaded > m_percent) {
        publish(uploaded);
    }
}

void ProgressTicker::advance() {
    if (!m_running || m_percent >= kWaitingCeiling) {
        return;
    }
    const int step = (kWaitingCeiling - m_percent) / kRampDivisor + 1;
    publish(m_percent + step);
}

void ProgressTicker::complete() {
    m_running = false;
    m_timer.stop();
    publish(kWaitingCeiling);
}

void ProgressTicker::publish(int percent) {
    m_percent = percent < 0 ? 0 : (percent > 100 ? 100 : percent);
    emit percentChanged(m_percent);
}

}
