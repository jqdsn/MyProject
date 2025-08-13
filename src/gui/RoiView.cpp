#include "RoiView.h"
#include <QPainter>
#include <QMouseEvent>
#include <QCursor>
#include <QtMath>
#include <QPainterPath>

RoiView::RoiView(QWidget* parent): QWidget(parent){
    setMouseTracking(true);
    setMinimumSize(320,240);
}

void RoiView::setImage(const QImage& img){
    img_ = img;
    rectImg_ = QRectF();
    centerImg_ = QPointF(-1,-1);
    polyImg_.clear();
    update();
}

void RoiView::clearRoi(){
    rectImg_ = QRectF();
    centerImg_ = QPointF(-1,-1);
    polyImg_.clear();
    update();
    emit roiChanged();
}

QPointF RoiView::toImage(const QPointF& v) const{
    if (img_.isNull()) return v;
    // Fit image into widget preserving aspect; compute letterbox transform
    double sx = double(width()) / img_.width();
    double sy = double(height()) / img_.height();
    double s = qMin(sx, sy);
    double w = img_.width()*s, h = img_.height()*s;
    double ox = (width()-w)/2.0, oy = (height()-h)/2.0;
    return QPointF( (v.x()-ox)/s, (v.y()-oy)/s );
}
QPointF RoiView::toView(const QPointF& p) const{
    if (img_.isNull()) return p;
    double sx = double(width()) / img_.width();
    double sy = double(height()) / img_.height();
    double s = qMin(sx, sy);
    double w = img_.width()*s, h = img_.height()*s;
    double ox = (width()-w)/2.0, oy = (height()-h)/2.0;
    return QPointF( p.x()*s + ox, p.y()*s + oy );
}

void RoiView::paintEvent(QPaintEvent*){
    QPainter g(this);
    g.fillRect(rect(), Qt::black);
    if (!img_.isNull()){
        // draw image
        double sx = double(width()) / img_.width();
        double sy = double(height()) / img_.height();
        double s = qMin(sx, sy);
        double w = img_.width()*s, h = img_.height()*s;
        double ox = (width()-w)/2.0, oy = (height()-h)/2.0;
        g.drawImage(QRectF(ox,oy,w,h), img_);
    }
    // overlay ROI
    g.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(QColor(0,255,0)); pen.setWidth(2); g.setPen(pen);
    QBrush brush(QColor(0,255,0,50));
    if (mode_==Mode::Rect && rectImg_.isValid()){
        QRectF vr = QRectF(toView(rectImg_.topLeft()), toView(rectImg_.bottomRight()));
        g.fillRect(vr, brush); g.drawRect(vr);
    } else if (mode_==Mode::Ring && centerImg_.x()>=0){
        QPointF c = toView(centerImg_);
        double sx = double(width()) / img_.width();
        double sy = double(height()) / img_.height();
        double s = qMin(sx, sy);
        double rIn = rInner_*s, rOut = rOuter_*s;
        g.setBrush(Qt::NoBrush);
        g.drawEllipse(c, rOut, rOut);
        QPen pin(QColor(0,200,255)); pin.setWidth(2); g.setPen(pin);
        g.drawEllipse(c, rIn, rIn);
    } else if (mode_==Mode::Polygon && polyImg_.size()>=2){
        QPolygonF poly;
        for (auto&p: polyImg_) poly << toView(p);
        g.setBrush(QColor(255,255,0,50));
        g.setPen(QPen(QColor(255,255,0),2));
        g.drawPolygon(poly);
        for (auto&p: poly){
            g.setBrush(Qt::yellow);
            g.drawEllipse(p, 3,3);
        }
    }
}

void RoiView::mousePressEvent(QMouseEvent* e){
    if (img_.isNull()) return;
    QPointF ip = toImage(e->pos());
    if (mode_==Mode::Rect){
        dragging_ = true;
        dragStartImg_ = dragCurImg_ = ip;
        rectImg_ = QRectF(dragStartImg_, dragCurImg_).normalized();
    } else if (mode_==Mode::Ring){
        if (centerImg_.x()<0){
            centerImg_ = ip; rInner_=20; rOuter_=60;
        } else {
            // choose drag mode based on distance
            double d = QLineF(toView(centerImg_), e->pos()).length();
            double sx = double(width()) / img_.width();
            double sy = double(height()) / img_.height();
            double s = qMin(sx, sy);
            if (std::abs(d - rOuter_*s) < 8) ringDrag_ = RingDrag::ResizeOuter;
            else if (std::abs(d - rInner_*s) < 8) ringDrag_ = RingDrag::ResizeInner;
            else ringDrag_ = RingDrag::MoveCenter;
        }
    } else if (mode_==Mode::Polygon){
        // Add point or start dragging nearest
        int nearest=-1; double best=1e9;
        for (int i=0;i<(int)polyImg_.size();++i){
            QPointF v = toView(polyImg_[i]);
            double d = QLineF(v, e->pos()).length();
            if (d<best){ best=d; nearest=i; }
        }
        if (best < 10) { polyDragIdx_ = nearest; }
        else { polyImg_.push_back(ip); polyDragIdx_ = (int)polyImg_.size()-1; }
    }
    update();
}

void RoiView::mouseMoveEvent(QMouseEvent* e){
    if (img_.isNull()) return;
    QPointF ip = toImage(e->pos());
    if (mode_==Mode::Rect){
        if (dragging_){
            dragCurImg_ = ip;
            rectImg_ = QRectF(dragStartImg_, dragCurImg_).normalized();
            emit roiChanged();
            update();
        }
    } else if (mode_==Mode::Ring){
        if (ringDrag_==RingDrag::MoveCenter){
            centerImg_ = ip;
            emit roiChanged(); update();
        } else if (ringDrag_==RingDrag::ResizeInner){
            rInner_ = std::max<double>(1.0, QLineF(centerImg_, ip).length());
            rInner_ = std::min(rInner_, rOuter_-1.0);
            emit roiChanged(); update();
        } else if (ringDrag_==RingDrag::ResizeOuter){
            rOuter_ = std::max<double>(rInner_+1.0, QLineF(centerImg_, ip).length());
            emit roiChanged(); update();
        }
    } else if (mode_==Mode::Polygon){
        if (polyDragIdx_>=0 && polyDragIdx_ < (int)polyImg_.size()){
            polyImg_[polyDragIdx_] = ip;
            emit roiChanged(); update();
        }
    }
}

void RoiView::mouseReleaseEvent(QMouseEvent*){
    dragging_ = false;
    ringDrag_ = RingDrag::None;
    polyDragIdx_ = -1;
}

void RoiView::resizeEvent(QResizeEvent*){
    update();
}

QRect RoiView::roiRect() const{
    if (img_.isNull()) return QRect();
    if (mode_==Mode::Rect && rectImg_.isValid()) return rectImg_.toAlignedRect().intersected(img_.rect());
    if (mode_==Mode::Ring && centerImg_.x()>=0){
        QRect r = QRectF((centerImg_.x()-rOuter_), (centerImg_.y()-rOuter_), 2*rOuter_, 2*rOuter_).toAlignedRect();
        return r.intersected(img_.rect());
    }
    if (mode_==Mode::Polygon && polyImg_.size()>=3){
        QRect r = QPolygonF(QVector<QPointF>::fromList(QList<QPointF>::fromVector(QVector<QPointF>(polyImg_.begin(), polyImg_.end())))).boundingRect().toAlignedRect();
        return r.intersected(img_.rect());
    }
    return QRect();
}

QImage RoiView::maskImage() const{
    if (img_.isNull()) return QImage();
    QImage m(img_.size(), QImage::Format_Grayscale8);
    m.fill(0);
    QPainter g(&m);
    g.setRenderHint(QPainter::Antialiasing, true);
    g.setPen(Qt::NoPen);
    g.setBrush(Qt::white);
    if (mode_==Mode::Rect && rectImg_.isValid()){
        g.drawRect(rectImg_);
    } else if (mode_==Mode::Ring && centerImg_.x()>=0){
        QPainterPath path;
        path.addEllipse(centerImg_, rOuter_, rOuter_);
        QPainterPath hole;
        hole.addEllipse(centerImg_, rInner_, rInner_);
        g.drawPath(path.subtracted(hole));
    } else if (mode_==Mode::Polygon && polyImg_.size()>=3){
        QPolygonF poly; for (auto&p: polyImg_) poly << p;
        g.drawPolygon(poly);
    }
    g.end();
    return m;
}
