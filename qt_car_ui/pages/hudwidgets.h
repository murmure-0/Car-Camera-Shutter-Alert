#ifndef HUDWIDGETS_H
#define HUDWIDGETS_H

#include <QColor>
#include <QFrame>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>
#include <QWidget>

class LineChartWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit LineChartWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(120);
    }

    void setSeries(QVector<double> values, QColor color)
    {
        m_values = std::move(values);
        m_color = color;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const QRectF r = rect().adjusted(6, 6, -6, -6);
        p.fillRect(rect(), Qt::transparent);

        if (m_values.size() < 2) {
            return;
        }

        double minV = m_values[0];
        double maxV = m_values[0];
        for (double v : m_values) {
            minV = qMin(minV, v);
            maxV = qMax(maxV, v);
        }
        const double range = qMax(0.0001, maxV - minV);

        QPainterPath path;
        for (int i = 0; i < m_values.size(); ++i) {
            const double t = (m_values.size() == 1) ? 0.0 : (double(i) / double(m_values.size() - 1));
            const double x = r.left() + t * r.width();
            const double yn = (m_values[i] - minV) / range;
            const double y = r.bottom() - yn * r.height();
            if (i == 0) {
                path.moveTo(x, y);
            } else {
                const double prevT = double(i - 1) / double(m_values.size() - 1);
                const double prevX = r.left() + prevT * r.width();
                const double prevYn = (m_values[i - 1] - minV) / range;
                const double prevY = r.bottom() - prevYn * r.height();
                const double cx = (prevX + x) * 0.5;
                path.cubicTo(cx, prevY, cx, y, x, y);
            }
        }

        QPainterPath fill = path;
        fill.lineTo(r.right(), r.bottom());
        fill.lineTo(r.left(), r.bottom());
        fill.closeSubpath();

        QColor fillColor = m_color;
        fillColor.setAlphaF(0.12);
        p.fillPath(fill, fillColor);

        QPen pen(m_color);
        pen.setWidthF(2.0);
        p.setPen(pen);
        p.drawPath(path);

        QPen grid(QColor(255, 255, 255, 18));
        grid.setWidthF(1.0);
        p.setPen(grid);
        for (int i = 1; i <= 3; ++i) {
            const double y = r.top() + (r.height() * (double(i) / 4.0));
            p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
        }
    }

private:
    QVector<double> m_values;
    QColor m_color = QColor("#22c55e");
};

class MapBackgroundWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit MapBackgroundWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_OpaquePaintEvent, false);
    }

    void setMapImage(const QImage &image)
    {
        m_mapImage = image;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        if (!m_mapImage.isNull()) {
            const QSize dst = size();
            const QImage scaled = m_mapImage.scaled(dst, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            const QPoint topLeft((dst.width() - scaled.width()) / 2, (dst.height() - scaled.height()) / 2);
            p.drawImage(QPointF(topLeft), scaled);
        } else {
            QLinearGradient g(rect().topLeft(), rect().bottomRight());
            g.setColorAt(0.0, QColor("#0b1220"));
            g.setColorAt(1.0, QColor("#1f2937"));
            p.fillRect(rect(), g);

            QPen grid(QColor(255, 255, 255, 18));
            grid.setWidth(1);
            p.setPen(grid);
            const int step = 40;
            for (int x = 0; x < width(); x += step) {
                p.drawLine(x, 0, x, height());
            }
            for (int y = 0; y < height(); y += step) {
                p.drawLine(0, y, width(), y);
            }
        }

        QRadialGradient vign(QPointF(width() * 0.55, height() * 0.45), qMax(width(), height()) * 0.75);
        vign.setColorAt(0.0, QColor(0, 0, 0, 0));
        vign.setColorAt(1.0, QColor(0, 0, 0, 160));
        p.fillRect(rect(), vign);
    }

private:
    QImage m_mapImage;
};

struct CameraDetectionOverlay {
    QRectF rect;
    QString label;
    float score = 0.0f;
};

class CameraPreviewWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit CameraPreviewWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_OpaquePaintEvent, false);
    }

    void setFrame(const QImage &img)
    {
        m_frame = img;
        update();
    }

    void setDetections(const QVector<CameraDetectionOverlay> &detections)
    {
        m_detections = detections;
        update();
    }

    void setOverlayText(const QStringList &lines)
    {
        m_overlayLines = lines;
        update();
    }

    void setFatigueInfo(const QStringList &info)
    {
        m_fatigueInfo = info;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        QLinearGradient g(rect().topLeft(), rect().bottomRight());
        g.setColorAt(0.0, QColor("#111827"));
        g.setColorAt(1.0, QColor("#000000"));
        p.fillRect(rect(), g);

        if (!m_frame.isNull()) {
            const QSize dst = size();
            const QSize src = m_frame.size();
            const QSize scaled = src.scaled(dst, Qt::KeepAspectRatio);
            const QRect r(QPoint((dst.width() - scaled.width()) / 2, (dst.height() - scaled.height()) / 2), scaled);
            p.drawImage(r, m_frame);
            if (!m_detections.isEmpty()) {
                const float sx = float(r.width()) / float(src.width());
                const float sy = float(r.height()) / float(src.height());
                for (const auto &det : m_detections) {
                    const QRectF rect(r.left() + det.rect.left() * sx,
                                      r.top() + det.rect.top() * sy,
                                      det.rect.width() * sx,
                                      det.rect.height() * sy);
                    bool is_tired = det.label == "TIRED";
                    QColor boxColor = is_tired ? QColor(239, 68, 68, 200) : QColor(34, 197, 94, 200);
                    QColor fillColor = is_tired ? QColor(239, 68, 68, 30) : QColor(34, 197, 94, 25);

                    p.setPen(Qt::NoPen);
                    p.setBrush(fillColor);
                    p.drawRoundedRect(rect, 4, 4);

                    const qreal cornerLen = qMin(rect.width(), rect.height()) * 0.2;
                    const qreal cornerWidth = 3.0;
                    QPen cornerPen(boxColor);
                    cornerPen.setWidthF(cornerWidth);
                    cornerPen.setCapStyle(Qt::RoundCap);
                    p.setPen(cornerPen);
                    p.setBrush(Qt::NoBrush);

                    QPainterPath corners;
                    // top-left
                    corners.moveTo(rect.left(), rect.top() + cornerLen);
                    corners.lineTo(rect.left(), rect.top());
                    corners.lineTo(rect.left() + cornerLen, rect.top());
                    // top-right
                    corners.moveTo(rect.right() - cornerLen, rect.top());
                    corners.lineTo(rect.right(), rect.top());
                    corners.lineTo(rect.right(), rect.top() + cornerLen);
                    // bottom-right
                    corners.moveTo(rect.right(), rect.bottom() - cornerLen);
                    corners.lineTo(rect.right(), rect.bottom());
                    corners.lineTo(rect.right() - cornerLen, rect.bottom());
                    // bottom-left
                    corners.moveTo(rect.left() + cornerLen, rect.bottom());
                    corners.lineTo(rect.left(), rect.bottom());
                    corners.lineTo(rect.left(), rect.bottom() - cornerLen);
                    p.drawPath(corners);

                    QPen thinPen(boxColor);
                    thinPen.setWidthF(1.0);
                    thinPen.setStyle(Qt::DashLine);
                    p.setPen(thinPen);
                    QPainterPath dashPath;
                    dashPath.moveTo(rect.left() + cornerLen, rect.top());
                    dashPath.lineTo(rect.right() - cornerLen, rect.top());
                    dashPath.moveTo(rect.right(), rect.top() + cornerLen);
                    dashPath.lineTo(rect.right(), rect.bottom() - cornerLen);
                    dashPath.moveTo(rect.right() - cornerLen, rect.bottom());
                    dashPath.lineTo(rect.left() + cornerLen, rect.bottom());
                    dashPath.moveTo(rect.left(), rect.bottom() - cornerLen);
                    dashPath.lineTo(rect.left(), rect.top() + cornerLen);
                    p.drawPath(dashPath);

                    if (!det.label.isEmpty()) {
                        QFont f = p.font();
                        f.setPointSizeF(10.0);
                        f.setBold(true);
                        p.setFont(f);
                        QString text = det.label;
                        const QSizeF ts = QFontMetrics(f).size(Qt::TextSingleLine, text);
                        QRectF labelBg(rect.left(), rect.top() - ts.height() - 10,
                                       ts.width() + 16, ts.height() + 10);
                        p.setPen(Qt::NoPen);
                        p.setBrush(boxColor);
                        p.drawRoundedRect(labelBg, 4, 4);
                        p.setPen(QColor(255, 255, 255, 240));
                        p.drawText(labelBg.adjusted(8, 2, -8, -2), Qt::AlignVCenter | Qt::AlignLeft, text);
                    }
                }
            }
            if (!m_fatigueInfo.isEmpty()) {
                QFont f = p.font();
                f.setPointSizeF(11.0);
                f.setBold(true);
                p.setFont(f);
                int lineHeight = QFontMetrics(f).height();
                int maxWidth = 0;
                for (const QString &line : m_fatigueInfo) {
                    maxWidth = qMax(maxWidth, QFontMetrics(f).size(Qt::TextSingleLine, line).width());
                }
                int x = r.right() - maxWidth - 14;
                int y = r.top() + 18;
                for (const QString &line : m_fatigueInfo) {
                    const QSize ts = QFontMetrics(f).size(Qt::TextSingleLine, line);
                    QRect bg(QPoint(x - 8, y - lineHeight - 6), QSize(ts.width() + 16, lineHeight + 14));
                    p.setPen(Qt::NoPen);
                    p.setBrush(QColor(0, 0, 0, 180));
                    p.drawRoundedRect(bg, 8, 8);
                    if (line.contains("Fatigue") || line.contains("fatigue")) {
                        p.setPen(QColor(255, 80, 80, 230));
                    } else if (line.contains("Normal") || line.contains("normal")) {
                        p.setPen(QColor(80, 255, 80, 230));
                    } else if (line.contains("Eyes Closed") || line.contains("Yawning")) {
                        p.setPen(QColor(255, 200, 80, 230));
                    } else {
                        p.setPen(QColor(255, 255, 255, 230));
                    }
                    p.drawText(QPoint(x, y + 2), line);
                    y += lineHeight + 8;
                }
            }
            
            if (!m_overlayLines.isEmpty()) {
                QFont f = p.font();
                f.setPointSizeF(12.0);
                f.setBold(true);
                p.setFont(f);
                int lineHeight = QFontMetrics(f).height();
                int x = r.left() + 14;
                int y = r.top() + 18;
                for (const QString &line : m_overlayLines) {
                    if (line.isEmpty()) {
                        y += lineHeight + 10;
                        continue;
                    }
                    const QSize ts = QFontMetrics(f).size(Qt::TextSingleLine, line);
                    QRect bg(QPoint(x - 8, y - lineHeight - 6), QSize(ts.width() + 16, lineHeight + 14));
                    p.setPen(Qt::NoPen);
                    p.setBrush(QColor(0, 0, 0, 140));
                    p.drawRoundedRect(bg, 8, 8);
                    p.setPen(QColor(255, 255, 255, 230));
                    p.drawText(QPoint(x, y + 2), line);
                    y += lineHeight + 10;
                }
            }
            return;
        }

        QPen grid(QColor(255, 255, 255, 60));
        grid.setWidthF(1.0);
        p.setPen(grid);
        const double w = width();
        const double h = height();
        for (int i = 1; i <= 2; ++i) {
            p.drawLine(QPointF(w * (double(i) / 3.0), 0), QPointF(w * (double(i) / 3.0), h));
            p.drawLine(QPointF(0, h * (double(i) / 3.0)), QPointF(w, h * (double(i) / 3.0)));
        }
    }

private:
    QImage m_frame;
    QVector<CameraDetectionOverlay> m_detections;
    QStringList m_overlayLines;
    QStringList m_fatigueInfo;
};

class ArtificialHorizonWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit ArtificialHorizonWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumSize(220, 220);
    }

    void setPitchRoll(double pitchDeg, double rollDeg)
    {
        m_pitchDeg = pitchDeg;
        m_rollDeg = rollDeg;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const QRectF r = rect().adjusted(8, 8, -8, -8);
        const QPointF c = r.center();
        const double radius = qMin(r.width(), r.height()) * 0.5;

        QPainterPath clip;
        clip.addEllipse(c, radius, radius);
        p.setClipPath(clip);

        p.save();
        p.translate(c);
        p.rotate(m_rollDeg);
        p.translate(-c);

        const double pitchPx = (m_pitchDeg * 2.0);
        QRectF skyRect = QRectF(r.left(), r.top() - r.height() * 0.5 + pitchPx, r.width(), r.height() * 1.5);
        QLinearGradient sky(skyRect.topLeft(), skyRect.bottomLeft());
        sky.setColorAt(0.0, QColor("#0ea5e9"));
        sky.setColorAt(1.0, QColor("#7dd3fc"));
        p.fillRect(skyRect, sky);

        QRectF groundRect = QRectF(r.left(), c.y() + pitchPx, r.width(), r.bottom() - (c.y() + pitchPx));
        if (groundRect.height() > 0) {
            QLinearGradient ground(groundRect.topLeft(), groundRect.bottomLeft());
            ground.setColorAt(0.0, QColor("#854d0e"));
            ground.setColorAt(1.0, QColor("#451a03"));
            p.fillRect(groundRect, ground);
        }

        QPen horizon(QColor(255, 255, 255, 220));
        horizon.setWidthF(2.0);
        p.setPen(horizon);
        p.drawLine(QPointF(r.left(), c.y() + pitchPx), QPointF(r.right(), c.y() + pitchPx));

        p.restore();
        p.setClipping(false);

        QPen ring(QColor("#374151"));
        ring.setWidthF(6.0);
        p.setPen(ring);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(c, radius, radius);

        QPen cross(QColor("#ef4444"));
        cross.setWidthF(4.0);
        cross.setCapStyle(Qt::RoundCap);
        p.setPen(cross);
        p.drawLine(QPointF(c.x() - 26, c.y()), QPointF(c.x() + 26, c.y()));
        QPen cross2(QColor("#ef4444"));
        cross2.setWidthF(4.0);
        cross2.setCapStyle(Qt::SquareCap);
        p.setPen(cross2);
        p.drawLine(QPointF(c.x(), c.y() - 8), QPointF(c.x(), c.y() + 8));
    }

private:
    double m_pitchDeg = 0.0;
    double m_rollDeg = 0.0;
};

inline QWidget *makeGlassPanel(QWidget *parent)
{
    auto *w = new QFrame(parent);
    w->setProperty("glass", true);
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return w;
}

#endif // HUDWIDGETS_H
