#ifndef BAIDU_GESTURE_CLIENT_H
#define BAIDU_GESTURE_CLIENT_H

#include <QDateTime>
#include <QImage>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>

class QNetworkAccessManager;
class QNetworkReply;

class BaiduGestureClient final : public QObject
{
    Q_OBJECT
public:
    explicit BaiduGestureClient(QObject *parent = nullptr);

    void setCredentials(QString apiKey, QString secretKey);
    void infer(const QImage &image);

signals:
    void inferenceFinished(bool ok, const QStringList &lines);

private:
    bool hasValidToken() const;
    void requestToken();
    void requestGesture(const QByteArray &jpegBytes);
    void finishError(const QStringList &lines);

    QNetworkAccessManager *m_network = nullptr;
    QString m_apiKey;
    QString m_secretKey;
    QString m_accessToken;
    QDateTime m_tokenExpiresAt;
    bool m_tokenInFlight = false;

    QByteArray m_pendingJpeg;
    bool m_gestureInFlight = false;
    bool m_retryTokenOnce = false;
    QPointer<QNetworkReply> m_tokenReply;
    QPointer<QNetworkReply> m_gestureReply;
};

#endif
