#pragma once
#include <QWidget>
#include <QImage>
#include <QPointF>
#include <vector>

class RoiView : public QWidget {
    Q_OBJECT
public:
    enum class Mode { Rect, Ring, Polygon };

    explicit RoiView(QWidget* parent=nullptr);
    void setImage(const QImage& img);
    QImage image() const { return img_; }

    void setMode(Mode m){ mode_ = m; update(); }
    Mode mode() const { return mode_; }
    void clearRoi();

    // ROI outputs
    QRect roiRect() const;                // bounding rect of the ROI
    QImage maskImage() const;             // 8-bit mask same size as image (white in ROI)
signals:
    void roiChanged();
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*) override;
private:
    QPointF toImage(const QPointF& v) const;
    QPointF toView(const QPointF& imgPt) const;

    QImage img_;
    Mode mode_ = Mode::Rect;

    // Rect
    bool dragging_ = false;
    QPointF dragStartImg_, dragCurImg_;
    QRectF rectImg_;

    // Ring (annulus): center + inner/outer radius
    QPointF centerImg_{-1,-1};
    double rInner_ = 20, rOuter_ = 60;
    enum class RingDrag { None, MoveCenter, ResizeInner, ResizeOuter } ringDrag_ = RingDrag::None;

    // Polygon
    std::vector<QPointF> polyImg_;
    int polyDragIdx_ = -1;
};
