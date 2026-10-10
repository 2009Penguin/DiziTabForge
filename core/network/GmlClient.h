#ifndef DIZITABFORGE_CORE_NETWORK_GMLCLIENT_H
#define DIZITABFORGE_CORE_NETWORK_GMLCLIENT_H

#include <QString>
#include <QStringList>
#include <functional>
#include <string>

namespace dizi {

struct GmlRequest {
    QString endpoint;
    QString apiKey;
    QString model;
    QString prompt;
    QStringList imageDataUris;
    int timeoutSeconds = 90;
};

struct GmlResponse {
    bool ok = false;
    QString content;
    QString error;
    long httpStatus = 0;
};

class GmlClient {
public:
    using ProgressCallback = std::function<void(qint64, qint64)>;

    struct ResponseSink;

    void setRequest(const GmlRequest &request);

    void setProgressCallback(ProgressCallback callback);

    GmlResponse send();

private:
    GmlResponse prepare() const;

    GmlResponse attempt(const std::string &payload, ResponseSink &sink);

    bool shouldRetry(const GmlResponse &response, const ResponseSink &sink) const;

    void waitBeforeRetry(const ResponseSink &sink, int attemptIndex);

    GmlResponse buildResult(int curlCode, ResponseSink &sink) const;

    GmlResponse failure(const QString &message) const;

    GmlRequest m_request;
    ProgressCallback m_progress;
};

}

#endif
