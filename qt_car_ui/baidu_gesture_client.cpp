#include "baidu_gesture_client.h"

#define ENABLE_GESTURE_API 1

#include <algorithm>
#include <QBuffer>
#include <QByteArray>
#include <QImageWriter>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

static QStringList makeNetworkErrorLines(const QString &title, int httpCode, const QString &errorString, const QByteArray &payload)
{
    QStringList lines;
    lines << title;
    if (httpCode > 0) {
        lines << QStringLiteral("HTTP %1").arg(httpCode);
    }
    if (!errorString.isEmpty()) {
        lines << errorString;
    }
    if (!payload.isEmpty()) {
        lines << QString::fromUtf8(payload.left(256));
    }
    return lines;
}

static QByteArray imageToEncodedBytes(const QImage &image)
{
    QImage src = image;
    if (src.isNull()) {
        return {};
    }
    if (src.format() != QImage::Format_RGB888 && src.format() != QImage::Format_ARGB32 && src.format() != QImage::Format_RGBA8888) {
        src = src.convertToFormat(QImage::Format_RGB888);
    }
    const int maxSide = 640;
    if (src.width() > maxSide || src.height() > maxSide) {
        src = src.scaled(QSize(maxSide, maxSide), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    QByteArray out;
    QBuffer buf(&out);
    buf.open(QIODevice::WriteOnly);
    QImageWriter writerJpg(&buf, "JPG");
    writerJpg.setQuality(85);
    if (!writerJpg.write(src)) {
        out.clear();
        buf.close();
        QBuffer pngBuf(&out);
        pngBuf.open(QIODevice::WriteOnly);
        QImageWriter writerPng(&pngBuf, "PNG");
        if (!writerPng.write(src)) {
            return {};
        }
    }
    return out;
}

static QUrl makeProxyBaseUrl()
{
    QString base = qEnvironmentVariable("BAIDU_PROXY_BASE");
    if (base.isEmpty()) {
        base = QStringLiteral("https://aip.baidubce.com");
    }
    QUrl url(base);
    if (!url.isValid()) {
        url = QUrl(QStringLiteral("https://aip.baidubce.com"));
    }
    return url;
}

BaiduGestureClient::BaiduGestureClient(QObject *parent)
    : QObject(parent)
{
    m_network = new QNetworkAccessManager(this);
    m_apiKey = qEnvironmentVariable("BAIDU_API_KEY");
    m_secretKey = qEnvironmentVariable("BAIDU_SECRET_KEY");
}

void BaiduGestureClient::setCredentials(QString apiKey, QString secretKey)
{
    m_apiKey = std::move(apiKey);
    m_secretKey = std::move(secretKey);
}

bool BaiduGestureClient::hasValidToken() const
{
    if (m_accessToken.isEmpty()) {
        return false;
    }
    if (!m_tokenExpiresAt.isValid()) {
        return true;
    }
    return QDateTime::currentDateTimeUtc() < m_tokenExpiresAt;
}

void BaiduGestureClient::infer(const QImage &image)
{
    if (m_gestureInFlight) {
        return;
    }

#if !ENABLE_GESTURE_API
    // Gesture API disabled — returning empty result
    emit inferenceFinished(true, QStringList());
    return;
#endif

    if (m_apiKey.isEmpty() || m_secretKey.isEmpty()) {
        finishError(QStringList{QStringLiteral("[Gesture] API credentials not configured. Set BAIDU_API_KEY and BAIDU_SECRET_KEY environment variables")});
        return;
    }

    m_pendingJpeg = imageToEncodedBytes(image);
    if (m_pendingJpeg.isEmpty()) {
        finishError(QStringList{QStringLiteral("[Gesture] Image encoding failed — unable to convert frame to JPEG/PNG")});
        return;
    }

    if (hasValidToken()) {
        requestGesture(m_pendingJpeg);
        return;
    }
    requestToken();
}

void BaiduGestureClient::requestToken()
{
    if (m_tokenInFlight) {
        return;
    }
    m_tokenInFlight = true;

    QUrl url = makeProxyBaseUrl();
    url.setPath(QStringLiteral("/oauth/2.0/token"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("client_credentials"));
    query.addQueryItem(QStringLiteral("client_id"), m_apiKey);
    query.addQueryItem(QStringLiteral("client_secret"), m_secretKey);
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("qt_car_ui"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
    m_tokenReply = m_network->post(req, QByteArray());

    auto *tokenTimeout = new QTimer(m_tokenReply);
    tokenTimeout->setSingleShot(true);
    tokenTimeout->setInterval(15000);
    connect(tokenTimeout, &QTimer::timeout, m_tokenReply, [reply = m_tokenReply]() {
        if (reply && reply->isRunning()) {
            reply->abort();
        }
    });
    tokenTimeout->start();

    connect(m_tokenReply, &QNetworkReply::finished, this, [this]() {
        m_tokenInFlight = false;
        if (!m_tokenReply) {
            finishError(QStringList{QStringLiteral("[OAuth] Token request failed — null reply")});
            return;
        }
        const int httpCode = m_tokenReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QString errorString = m_tokenReply->errorString();
        const QByteArray payload = m_tokenReply->readAll();
        const QNetworkReply::NetworkError err = m_tokenReply->error();
        m_tokenReply->deleteLater();
        m_tokenReply = nullptr;

        if (err != QNetworkReply::NoError) {
            finishError(makeNetworkErrorLines(QStringLiteral("[OAuth] Token request network error"), httpCode, errorString, payload));
            return;
        }

        QJsonParseError jerr{};
        const QJsonDocument doc = QJsonDocument::fromJson(payload, &jerr);
        if (jerr.error != QJsonParseError::NoError || !doc.isObject()) {
            finishError(QStringList{QStringLiteral("[OAuth] Token response JSON parse error"), QString::fromUtf8(payload.left(256))});
            return;
        }
        const QJsonObject obj = doc.object();
        const QString token = obj.value(QStringLiteral("access_token")).toString();
        const int expiresIn = obj.value(QStringLiteral("expires_in")).toInt(0);
        if (token.isEmpty()) {
            QString msg = obj.value(QStringLiteral("error_description")).toString();
            if (msg.isEmpty()) {
                msg = QString::fromUtf8(payload);
            }
            finishError(QStringList{QStringLiteral("[OAuth] Access token is empty in response"), msg});
            return;
        }
        m_accessToken = token;
        if (expiresIn > 0) {
            m_tokenExpiresAt = QDateTime::currentDateTimeUtc().addSecs(std::max(0, expiresIn - 60));
        } else {
            m_tokenExpiresAt = QDateTime();
        }
        if (m_retryTokenOnce) {
            m_retryTokenOnce = false;
        }

        if (!m_pendingJpeg.isEmpty()) {
            requestGesture(m_pendingJpeg);
        }
    });
}

void BaiduGestureClient::requestGesture(const QByteArray &jpegBytes)
{
    if (m_gestureInFlight) {
        return;
    }
    if (!hasValidToken()) {
        requestToken();
        return;
    }
    m_gestureInFlight = true;

    QByteArray b64 = jpegBytes.toBase64();
    const QByteArray encoded = QUrl::toPercentEncoding(QString::fromLatin1(b64));
    QByteArray body = "image_type=BASE64&image=";
    body += encoded;

    QUrl url = makeProxyBaseUrl();
    url.setPath(QStringLiteral("/rest/2.0/image-classify/v1/gesture"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("access_token"), m_accessToken);
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded; charset=UTF-8"));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("qt_car_ui"));
    req.setRawHeader("Accept", "application/json");

    m_gestureReply = m_network->post(req, body);

    auto *gestureTimeout = new QTimer(m_gestureReply);
    gestureTimeout->setSingleShot(true);
    gestureTimeout->setInterval(15000);
    connect(gestureTimeout, &QTimer::timeout, m_gestureReply, [reply = m_gestureReply]() {
        if (reply && reply->isRunning()) {
            reply->abort();
        }
    });
    gestureTimeout->start();

    connect(m_gestureReply, &QNetworkReply::finished, this, [this]() {
        m_gestureInFlight = false;
        if (!m_gestureReply) {
            finishError(QStringList{QStringLiteral("[Gesture] Recognition request failed — null reply")});
            return;
        }
        const int httpCode = m_gestureReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QString errorString = m_gestureReply->errorString();
        const QByteArray payload = m_gestureReply->readAll();
        const QNetworkReply::NetworkError err = m_gestureReply->error();
        m_gestureReply->deleteLater();
        m_gestureReply = nullptr;

        if (err != QNetworkReply::NoError) {
            finishError(makeNetworkErrorLines(QStringLiteral("[Gesture] Recognition request network error"), httpCode, errorString, payload));
            return;
        }

        QJsonParseError jerr{};
        const QJsonDocument doc = QJsonDocument::fromJson(payload, &jerr);
        if (jerr.error != QJsonParseError::NoError || !doc.isObject()) {
            finishError(QStringList{QStringLiteral("[Gesture] Response JSON parse error"), QString::fromUtf8(payload.left(256))});
            return;
        }
        const QJsonObject obj = doc.object();
        if (obj.contains(QStringLiteral("error_code"))) {
            const int codeValue = obj.value(QStringLiteral("error_code")).toInt();
            const QString code = QString::number(codeValue);
            const QString msg = obj.value(QStringLiteral("error_msg")).toString();
            if ((codeValue == 100 || codeValue == 110 || codeValue == 111) && !m_retryTokenOnce) {
                m_retryTokenOnce = true;
                m_accessToken.clear();
                requestToken();
                return;
            }
            finishError(QStringList{QStringLiteral("[Gesture] API error code=%1").arg(code), msg});
            return;
        }

        QStringList lines;
        const QJsonArray results = obj.value(QStringLiteral("result")).toArray();
        const int n = qMin(3, results.size());
        for (int i = 0; i < n; ++i) {
            const QJsonObject r = results.at(i).toObject();
            const QString name = r.value(QStringLiteral("classname")).toString();
            const double prob = r.value(QStringLiteral("probability")).toDouble(-1.0);
            if (prob >= 0.0) {
                lines << QStringLiteral("%1. %2  %3").arg(i + 1).arg(name).arg(prob, 0, 'f', 3);
            } else {
                lines << QStringLiteral("%1. %2").arg(i + 1).arg(name);
            }
        }
        if (lines.isEmpty()) {
            lines << QStringLiteral("No gesture detected");
        }
        emit inferenceFinished(true, lines);
    });
}

void BaiduGestureClient::finishError(const QStringList &lines)
{
    emit inferenceFinished(false, lines);
}
