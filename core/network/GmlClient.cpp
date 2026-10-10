#include "core/network/GmlClient.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <QDebug>

#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "core/util/PathUtil.h"

using namespace nlohmann;

namespace dizi {

struct GmlClient::ResponseSink {
    QByteArray body;
    long httpStatus = 0;
    QString retryAfter;
    GmlClient::ProgressCallback progress;
};

namespace {

constexpr int kMaxAttempts = 3;
constexpr int kFirstRetryDelaySeconds = 2;
constexpr int kMaxRetryDelaySeconds = 90;

using CurlHandle = std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>;
using CurlHeaders = std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)>;

size_t collectBody(char *data, size_t size, size_t count, void *userData) {
    auto *sink = static_cast<GmlClient::ResponseSink *>(userData);
    sink->body.append(data, static_cast<int>(size * count));
    return size * count;
}

size_t collectHeader(char *data, size_t size, size_t count, void *userData) {
    auto *sink = static_cast<GmlClient::ResponseSink *>(userData);
    const QByteArray line(data, static_cast<int>(size * count));
    const QByteArray name = QByteArrayLiteral("retry-after:");
    if (line.left(name.size()).toLower() == name) {
        sink->retryAfter = QString::fromLatin1(line.mid(name.size())).trimmed();
    }
    return size * count;
}

int reportProgress(void *userData, curl_off_t downloadTotal, curl_off_t downloadNow,
                   curl_off_t uploadTotal, curl_off_t uploadNow) {
    auto *sink = static_cast<GmlClient::ResponseSink *>(userData);
    if (!sink->progress) {
        return 0;
    }
    const auto total = static_cast<qint64>(uploadTotal + downloadTotal);
    const auto done = static_cast<qint64>(uploadNow + downloadNow);
    sink->progress(done, total);
    return 0;
}

int retryDelaySeconds(const GmlClient::ResponseSink &sink) {
    bool valid = false;
    const int reported = sink.retryAfter.toInt(&valid);
    if (valid && reported > 0) {
        return reported < kMaxRetryDelaySeconds ? reported : kMaxRetryDelaySeconds;
    }
    return 0;
}

json buildPayload(const GmlRequest &request) {
    json content = json::array();
    content.push_back({{"type", "text"}, {"text", request.prompt.toStdString()}});
    for (const QString &uri : request.imageDataUris) {
        content.push_back(
                {{"type", "image_url"}, {"image_url", {{"url", uri.toStdString()}}}});
    }
    json message = {{"role", "user"}, {"content", content}};
    return json{{"model", request.model.toStdString()},
                {"messages", json::array({message})},
                {"temperature", 0.1},
                {"stream", false}};
}

QString buildAuthHeader(const QString &apiKey) {
    return QStringLiteral("Authorization: Bearer %1").arg(apiKey.trimmed());
}

void applyCommonOptions(CURL *handle, const GmlRequest &request, GmlClient::ResponseSink &sink,
                        const std::string &payload) {
    const QByteArray url = path_util::normalizeChatEndpoint(request.endpoint).toUtf8();
    curl_easy_setopt(handle, CURLOPT_URL, url.constData());
    curl_easy_setopt(handle, CURLOPT_POST, 1L);
    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload.size()));
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &collectBody);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &sink);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, &collectHeader);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, &sink);
    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 20L);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, static_cast<long>(request.timeoutSeconds));
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, &reportProgress);
    curl_easy_setopt(handle, CURLOPT_XFERINFODATA, &sink);
    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);
}

CurlHeaders buildHeaders(const GmlRequest &request) {
    curl_slist *list = nullptr;
    list = curl_slist_append(list, "Content-Type: application/json");
    list = curl_slist_append(list, "Accept: application/json");
    list = curl_slist_append(list, buildAuthHeader(request.apiKey).toUtf8().constData());
    return CurlHeaders(list, &curl_slist_free_all);
}

QString extractApiError(const json &document) {
    if (!document.contains("error")) {
        return QString();
    }
    const json &error = document.at("error");
    if (error.is_string()) {
        return QString::fromStdString(error.get<std::string>());
    }
    if (error.is_object()) {
        return QString::fromStdString(error.value("message", std::string("unknown error")));
    }
    return QStringLiteral("unknown error object");
}

QString readChoiceContent(const json &document, GmlResponse &response) {
    if (!document.contains("choices") || !document.at("choices").is_array() ||
        document.at("choices").empty()) {
        return QStringLiteral("The response has no choices; the image may have been rejected.");
    }
    const json &message = document.at("choices").at(0).value("message", json::object());
    response.content = QString::fromStdString(message.value("content", std::string()));
    if (response.content.trimmed().isEmpty()) {
        return QStringLiteral("The model returned an empty transcription.");
    }
    return QString();
}

QString parseResponse(const GmlClient::ResponseSink &sink, GmlResponse &response) {
    json document;
    try {
        document = json::parse(std::string(sink.body.constData(), sink.body.size()));
    } catch (const json::exception &problem) {
        return QStringLiteral("The server returned data that is not valid JSON: %1")
                .arg(QString::fromUtf8(problem.what()));
    }
    if (!document.is_object()) {
        return QStringLiteral("The server returned a JSON value that is not an object.");
    }
    const QString apiError = extractApiError(document);
    if (!apiError.isEmpty()) {
        return QStringLiteral("The model API reported an error: %1").arg(apiError);
    }
    return readChoiceContent(document, response);
}

QString describeHttpFailure(const GmlClient::ResponseSink &sink) {
    if (sink.httpStatus == 401 || sink.httpStatus == 403) {
        return QStringLiteral("HTTP %1: the API Key was rejected.").arg(sink.httpStatus);
    }
    if (sink.httpStatus == 429) {
        return QStringLiteral("HTTP 429: the free model is rate limited. It was retried three "
                              "times and is still busy, so wait a minute and generate again.");
    }
    const QString body = QString::fromUtf8(sink.body.left(400));
    return QStringLiteral("HTTP %1: %2").arg(sink.httpStatus).arg(body);
}

}

void GmlClient::setRequest(const GmlRequest &request) {
    m_request = request;
}

void GmlClient::setProgressCallback(ProgressCallback callback) {
    m_progress = std::move(callback);
}

GmlResponse GmlClient::prepare() const {
    GmlResponse response;
    if (m_request.apiKey.trimmed().isEmpty()) {
        response.error = QStringLiteral("API Key is empty. Open Settings and enter a key.");
        return response;
    }
    if (m_request.endpoint.trimmed().isEmpty()) {
        response.error = QStringLiteral("The endpoint URL is empty. Check config.json.");
        return response;
    }
    if (m_request.imageDataUris.isEmpty()) {
        response.error = QStringLiteral("No image was prepared for recognition.");
        return response;
    }
    response.ok = true;
    return response;
}

GmlResponse GmlClient::failure(const QString &message) const {
    GmlResponse response;
    response.ok = false;
    response.error = message;
    return response;
}

GmlResponse GmlClient::send() {
    const GmlResponse guard = prepare();
    if (!guard.ok) {
        return guard;
    }
    const std::string payload = buildPayload(m_request).dump();
    ResponseSink sink;
    sink.progress = m_progress;
    GmlResponse response;
    for (int attemptIndex = 1; attemptIndex <= kMaxAttempts; ++attemptIndex) {
        response = attempt(payload, sink);
        if (!shouldRetry(response, sink)) {
            return response;
        }
        waitBeforeRetry(sink, attemptIndex);
    }
    return response;
}

GmlResponse GmlClient::attempt(const std::string &payload, ResponseSink &sink) {
    sink.body.clear();
    sink.httpStatus = 0;
    sink.retryAfter.clear();
    CurlHandle handle(curl_easy_init(), &curl_easy_cleanup);
    if (!handle) {
        return failure(QStringLiteral("libcurl could not create a session handle."));
    }
    const CurlHeaders headers = buildHeaders(m_request);
    curl_easy_setopt(handle.get(), CURLOPT_HTTPHEADER, headers.get());
    applyCommonOptions(handle.get(), m_request, sink, payload);
    qDebug().noquote() << "[http] POST"
                       << path_util::normalizeChatEndpoint(m_request.endpoint)
                       << "\n[http] model =" << m_request.model
                       << " images =" << m_request.imageDataUris.size()
                       << " payloadBytes =" << static_cast<qint64>(payload.size())
                       << " apiKeyChars =" << m_request.apiKey.trimmed().size()
                       << " timeoutSec =" << m_request.timeoutSeconds;
    const CURLcode result = curl_easy_perform(handle.get());
    curl_easy_getinfo(handle.get(), CURLINFO_RESPONSE_CODE, &sink.httpStatus);
    qDebug().noquote() << "[http] curlCode =" << static_cast<int>(result)
                       << curl_easy_strerror(result) << " httpStatus =" << sink.httpStatus
                       << " responseBytes =" << sink.body.size();
    return buildResult(static_cast<int>(result), sink);
}

bool GmlClient::shouldRetry(const GmlResponse &response, const ResponseSink &sink) const {
    if (response.ok) {
        return false;
    }
    return sink.httpStatus == 429 || sink.httpStatus >= 500;
}

void GmlClient::waitBeforeRetry(const ResponseSink &sink, int attemptIndex) {
    const int reported = retryDelaySeconds(sink);
    const int seconds =
            reported > 0 ? reported : kFirstRetryDelaySeconds * (1 << (attemptIndex - 1));
    if (m_progress) {
        m_progress(-1, -1);
    }
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

GmlResponse GmlClient::buildResult(int curlCode, ResponseSink &sink) const {
    if (curlCode != CURLE_OK) {
        qWarning().noquote() << "[http] transport failure:"
                             << curl_easy_strerror(static_cast<CURLcode>(curlCode));
        return failure(QStringLiteral("Network error: %1")
                               .arg(QString::fromLatin1(curl_easy_strerror(
                                       static_cast<CURLcode>(curlCode)))));
    }
    if (sink.httpStatus < 200 || sink.httpStatus >= 300) {
        qWarning().noquote() << "[http] non-2xx body:" << QString::fromUtf8(sink.body.left(500));
        GmlResponse failed = failure(describeHttpFailure(sink));
        failed.httpStatus = sink.httpStatus;
        return failed;
    }
    GmlResponse response;
    response.httpStatus = sink.httpStatus;
    const QString problem = parseResponse(sink, response);
    if (!problem.isEmpty()) {
        qWarning().noquote() << "[http] response parse problem:" << problem
                             << "\n[http] body head:"
                             << QString::fromUtf8(sink.body.left(500));
        response.ok = false;
        response.content.clear();
        response.error = problem;
        return response;
    }
    qDebug().noquote() << "[http] transcription length =" << response.content.size()
                       << " chars";
    response.ok = true;
    return response;
}

}
