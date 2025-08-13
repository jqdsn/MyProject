#include <QtNetwork>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include "core/pipeline.h"
#include "ops/canny.h"
#include "ops/morph.h"
#include "ops/threshold.h"
#include "measure/geometry.h"
#include "measure/gauges.h"
#include "measure/calibration.h"
#include "backend/specs_store.h"
#include "backend/json_utils.h"

using namespace mp;

static QJsonObject measureImage(const cv::Mat& img, const QJsonObject& payload){
    // Get specs / calibration
    double mm_per_px = payload.value("mm_per_px").toDouble(0.02);
    Calibration cal; cal.scale_mm_per_px = mm_per_px;

    // ROI mask from payload
    cv::Mat mask(img.rows, img.cols, CV_8UC1, cv::Scalar(255)); // default full
    auto roi = payload.value("roi").toObject();
    QString type = roi.value("type").toString();
    if (!type.isEmpty()){
        mask.setTo(0);
        if (type == "rect"){
            int x = roi.value("x").toInt();
            int y = roi.value("y").toInt();
            int w = roi.value("w").toInt();
            int h = roi.value("h").toInt();
            cv::rectangle(mask, cv::Rect(x,y,w,h), cv::Scalar(255), cv::FILLED);
        } else if (type == "polygon"){
            QJsonArray pts = roi.value("points").toArray();
            std::vector<cv::Point> poly; poly.reserve(pts.size());
            for (auto v: pts){
                auto a = v.toArray();
                poly.emplace_back(a.at(0).toInt(), a.at(1).toInt());
            }
            if (poly.size()>=3){
                std::vector<std::vector<cv::Point>> polys{poly};
                cv::fillPoly(mask, polys, cv::Scalar(255));
            }
        } else if (type == "ring"){
            int cx = roi.value("cx").toInt();
            int cy = roi.value("cy").toInt();
            int r_in = roi.value("r_in").toInt();
            int r_out = roi.value("r_out").toInt();
            cv::circle(mask, {cx,cy}, r_out, 255, cv::FILLED);
            cv::circle(mask, {cx,cy}, r_in, 0, cv::FILLED);
        }
    }

    // Process pipeline
    Pipeline p;
    p.add(std::make_shared<op::Canny>(50,150,3,true));
    p.add(std::make_shared<op::Morph>(cv::MORPH_CLOSE, 3, 1));
    p.add(std::make_shared<op::Threshold>(128.0, cv::THRESH_BINARY));
    cv::Mat masked; p.run(Frame{img,"api"}).mat.copyTo(masked, mask);

    // Extract contours inside mask bbox
    cv::Rect roiRect(0,0,img.cols,img.rows);
    if (type=="rect"){
        roiRect = cv::Rect(roi.value("x").toInt(), roi.value("y").toInt(),
                           roi.value("w").toInt(), roi.value("h").toInt()) & cv::Rect(0,0,img.cols,img.rows);
    } else if (type=="polygon" || type=="ring"){
        // approximate by mask bbox
        std::vector<std::vector<cv::Point>> cc;
        cv::findContours(mask, cc, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        if (!cc.empty()) roiRect = cv::boundingRect(cc[0]);
    }

    cv::Mat gray; if (masked.channels()==3) cv::cvtColor(masked, gray, cv::COLOR_BGR2GRAY); else gray=masked;
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(gray, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    std::sort(contours.begin(), contours.end(), [](auto& a, auto& b){ return cv::contourArea(a) > cv::contourArea(b); });

    // Fit circles
    QJsonObject result;
    QJsonArray outMetrics;

    std::vector<cv::Point2f> ptsA, ptsB;
    if (contours.size() >= 1) for (auto& p: contours[0]) ptsA.push_back(cv::Point2f(p) + cv::Point2f((float)roiRect.x,(float)roiRect.y));
    if (contours.size() >= 2) for (auto& p: contours[1]) ptsB.push_back(cv::Point2f(p) + cv::Point2f((float)roiRect.x,(float)roiRect.y));

    bool hasA=false, hasB=false;
    Circle circA{{0,0},0}, circB{{0,0},0};
    if (ptsA.size() >= 12){ circA = fitCircleKasa(ptsA); hasA=true; }
    if (ptsB.size() >= 12){ circB = fitCircleKasa(ptsB); hasB=true; }

    // Lines from top/bottom halves
    std::vector<cv::Point2f> topPts, botPts;
    for (auto& c : contours){
        for (auto& p : c){
            cv::Point pt = p + cv::Point(roiRect.x, roiRect.y);
            if (pt.y < roiRect.y + roiRect.height*0.5) topPts.push_back(pt);
            else botPts.push_back(pt);
        }
    }
    bool hasTop=false, hasBot=false; Line2D Ltop{{0,0},{1,0}}, Lbot{{0,0},{1,0}};
    if (topPts.size() >= 20){ Ltop = fitLineLSQ(topPts); hasTop=true; }
    if (botPts.size() >= 20){ Lbot = fitLineLSQ(botPts); hasBot=true; }

    auto pushMetric = [&](const QString& name, double val, const QString& unit, bool ok, const QString& note=QString()){
        QJsonObject m; m["name"]=name; m["value"]=val; m["unit"]=unit; m["ok"]=ok; if (!note.isEmpty()) m["note"]=note;
        outMetrics.push_back(m);
    };

    // Specs from payload
    auto specs = payload.value("specs").toObject();

    // Line gap & parallelism
    if (hasTop && hasBot){
        auto mGap = gauge::metricLineGapMM(Ltop, Lbot, roiRect, cal);
        double target = specs.value("line_gap").toObject().value("target").toDouble(0);
        double tol = specs.value("line_gap").toObject().value("tol").toDouble(0);
        bool okGap = (std::abs(mGap.value_mm - target) <= tol + 1e-9);
        pushMetric("line_gap", mGap.value_mm, "mm", okGap, QString("%1±%2").arg(target).arg(tol));

        auto mPar = gauge::metricParallelismDeg(Ltop, Lbot);
        double maxdeg = specs.value("parallelism").toObject().value("max_deg").toDouble(1.0);
        bool okPar = (std::abs(mPar.value_mm) <= maxdeg + 1e-9);
        pushMetric("parallelism", mPar.value_mm, "deg", okPar, QString("≤%1").arg(maxdeg));
    }

    // Circle metrics
    if (hasA){
        auto mDia = gauge::metricDiameterMM(circA, cal);
        double target = specs.value("diameter").toObject().value("target").toDouble(0);
        double tol = specs.value("diameter").toObject().value("tol").toDouble(0);
        bool okDia = (std::abs(mDia.value_mm - target) <= tol + 1e-9);
        pushMetric("diameter_A", mDia.value_mm, "mm", okDia, QString("%1±%2").arg(target).arg(tol));

        auto mRnd = gauge::metricRoundnessMM(ptsA, cal);
        double maxmm = specs.value("roundness").toObject().value("max_mm").toDouble(0.05);
        bool okRnd = (mRnd.value_mm <= maxmm + 1e-9);
        pushMetric("roundness_A", mRnd.value_mm, "mm", okRnd, QString("≤%1").arg(maxmm));
    }

    if (hasA && hasB){
        auto mCon = gauge::metricConcentricityMM(circA, circB, cal);
        double maxmm = specs.value("concentricity").toObject().value("max_mm").toDouble(0.1);
        bool okCon = (mCon.value_mm <= maxmm + 1e-9);
        pushMetric("concentricity_AB", mCon.value_mm, "mm", okCon, QString("≤%1").arg(maxmm));
    }

    result["metrics"] = outMetrics;
    return result;
}

class HttpServer : public QTcpServer {
    Q_OBJECT
public:
    HttpServer(const QString& specsPath, QObject* parent=nullptr)
      : QTcpServer(parent), store_(specsPath) { store_.load(); }
protected:
    void incomingConnection(qintptr sd) override {
        auto* sock = new QTcpSocket(this);
        sock->setSocketDescriptor(sd);
        connect(sock, &QTcpSocket::readyRead, this, [this, sock](){
            buf_ += sock->readAll();
            // very small/simple HTTP parser: handle one request per connection
            if (!buf_.contains("\r\n\r\n")) return;
            auto headerEnd = buf_.indexOf("\r\n\r\n");
            QByteArray head = buf_.left(headerEnd);
            QByteArray body = buf_.mid(headerEnd+4);
            QList<QByteArray> lines = head.split('\n');
            QByteArray reqline = lines.first().trimmed();
            auto parts = reqline.split(' ');
            if (parts.size()<2){ writePlain(sock, 400, "Bad Request", "bad"); return; }
            QByteArray method = parts[0];
            QByteArray path = parts[1];

            if (method=="GET" && path.startsWith("/health")){
                writeJson(sock, 200, QJsonObject{{"status","ok"}});
            }
            else if (method=="GET" && path.startsWith("/specs/")){
                QString id = QString::fromUtf8(path.mid(strlen("/specs/")));
                auto spec = store_.get(id);
                if (spec.isEmpty()) writeJson(sock, 404, QJsonObject{{"error","not found"},{"id",id}});
                else writeJson(sock, 200, spec);
            }
            else if (method=="POST" && path == "/specs"){
                auto doc = QJsonDocument::fromJson(body);
                if (!doc.isObject()){ writePlain(sock, 400, "Bad Request", "invalid json"); return; }
                auto obj = doc.object();
                QString id = obj.value("id").toString();
                QJsonObject spec = obj.value("spec").toObject();
                if (id.isEmpty() || spec.isEmpty()){ writePlain(sock, 400, "Bad Request", "missing id/spec"); return; }
                store_.put(id, spec);
                store_.save();
                writeJson(sock, 200, QJsonObject{{"ok", true},{"id", id}});
            }
            else if (method=="POST" && path == "/measure"){
                auto doc = QJsonDocument::fromJson(body);
                if (!doc.isObject()){ writePlain(sock, 400, "Bad Request", "invalid json"); return; }
                auto obj = doc.object();
                // Resolve image
                QString imgPath = obj.value("image_path").toString();
                cv::Mat img = cv::imread(imgPath.toStdString());
                if (img.empty()){ writePlain(sock, 400, "Bad Request", "bad image path"); return; }
                // Resolve specs: inline or by id
                QJsonObject specs;
                if (obj.contains("specs")) specs = obj.value("specs").toObject();
                else if (obj.contains("spec_id")) specs = store_.get(obj.value("spec_id").toString());
                // attach calibration
                if (!specs.contains("mm_per_px")){
                    double s = obj.value("mm_per_px").toDouble(0.02);
                    specs.insert("mm_per_px", s);
                }
                QJsonObject payload;
                payload["mm_per_px"] = specs.value("mm_per_px");
                payload["specs"] = specs;
                if (obj.contains("roi")) payload["roi"] = obj.value("roi").toObject();

                auto result = measureImage(img, payload);
                writeJson(sock, 200, result);
            }
            else {
                writePlain(sock, 404, "Not Found", "not found");
            }
            sock->disconnectFromHost();
            buf_.clear();
        });
        connect(sock, &QTcpSocket::disconnected, sock, &QObject::deleteLater);
    }
private:
    void writeJson(QTcpSocket* sock, int code, const QJsonObject& obj){
        auto bytes = toBytes(obj);
        QByteArray resp;
        resp += "HTTP/1.1 " + QByteArray::number(code) + " OK\r\n";
        resp += "Content-Type: application/json\r\n";
        resp += "Content-Length: " + QByteArray::number(bytes.size()) + "\r\n";
        resp += "Connection: close\r\n\r\n";
        resp += bytes;
        sock->write(resp);
    }
    void writePlain(QTcpSocket* sock, int code, const char* text, const char* body){
        QByteArray resp;
        resp += "HTTP/1.1 " + QByteArray::number(code) + " "; resp += text; resp += "\r\n";
        resp += "Content-Type: text/plain\r\n";
        resp += "Content-Length: " + QByteArray::number(strlen(body)) + "\r\n";
        resp += "Connection: close\r\n\r\n";
        resp += body;
        sock->write(resp);
    }
    QByteArray buf_;
    SpecsStore store_;
};

#include "server.moc"

int main(int argc, char** argv){
    QCoreApplication app(argc, argv);
    QString cfg = QCoreApplication::applicationDirPath() + "/../../config/specs.json";
    HttpServer s(cfg);
    if (!s.listen(QHostAddress::AnyIPv4, 8080)){
        qWarning() << "Listen failed";
        return 1;
    }
    qInfo() << "REST http://localhost:8080  (POST /measure, GET /specs/{id}, POST /specs)";
    return app.exec();
}
