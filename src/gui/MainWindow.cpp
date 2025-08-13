#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "RoiView.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>
#include <QPixmap>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QBrush>
#include <QColor>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "core/pipeline.h"
#include "ops/canny.h"
#include "ops/morph.h"
#include "ops/threshold.h"
#include "measure/caliper.h"
#include "measure/calibration.h"
#include "measure/report.h"
#include "measure/gauges.h"
#include "measure/geometry.h"

using namespace mp;

static QImage matToQ(const cv::Mat& bgr){
  cv::Mat rgb; cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
  return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
}

MainWindow::MainWindow(QWidget* parent): QMainWindow(parent), ui(new Ui::MainWindow){
  ui->setupUi(this);
  connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::onOpen);
  connect(ui->actionRun,  &QAction::triggered, this, &MainWindow::onRun);
  connect(ui->btnRect, &QPushButton::clicked, this, &MainWindow::onModeRect);
  connect(ui->btnRing, &QPushButton::clicked, this, &MainWindow::onModeRing);
  connect(ui->btnPoly, &QPushButton::clicked, this, &MainWindow::onModePoly);
  connect(ui->btnClear, &QPushButton::clicked, this, &MainWindow::onClearRoi);

  // Replace placeholder labelInput with RoiView
  roiView_ = new RoiView(this);
  ui->layoutInput->addWidget(roiView_);

  ui->tableResults->setColumnCount(4);
  ui->tableResults->setHorizontalHeaderLabels({"Metric","Value","Spec","OK"});
  ui->tableResults->horizontalHeader()->setStretchLastSection(true);
}

MainWindow::~MainWindow(){ delete ui; }

void MainWindow::onOpen(){
  auto fn = QFileDialog::getOpenFileName(this, "Open", {}, "Images (*.png *.jpg *.jpeg *.bmp)");
  if (fn.isEmpty()) return;
  cv::Mat img = cv::imread(fn.toStdString());
  if (img.empty()){ QMessageBox::warning(this, "Error", "Failed to open image."); return; }
  roiView_->setImage(matToQ(img));
  ui->labelOutput->setPixmap(QPixmap()); // clear out
  ui->labelOutput->setText("Output");
}

void MainWindow::clearResults(){ ui->tableResults->setRowCount(0); }

void MainWindow::appendResultRow(const QString& name, const QString& value, const QString& spec, bool ok){
  int r = ui->tableResults->rowCount();
  ui->tableResults->insertRow(r);
  ui->tableResults->setItem(r,0,new QTableWidgetItem(name));
  ui->tableResults->setItem(r,1,new QTableWidgetItem(value));
  ui->tableResults->setItem(r,2,new QTableWidgetItem(spec));
  auto okItem = new QTableWidgetItem(ok? "OK" : "NG");
  okItem->setBackground(ok? QBrush(QColor(200,255,200)) : QBrush(QColor(255,200,200)));
  ui->tableResults->setItem(r,3, okItem);
}

void MainWindow::onRun(){
  if (roiView_->image().isNull()){ QMessageBox::information(this, "Info", "Open an image first."); return; }
  // Convert QImage back to cv::Mat (BGR)
  QImage imgQ = roiView_->image().convertToFormat(QImage::Format_RGB888);
  cv::Mat img(imgQ.height(), imgQ.width(), CV_8UC3, const_cast<uchar*>(imgQ.bits()), imgQ.bytesPerLine());
  cv::cvtColor(img, img, cv::COLOR_RGB2BGR);

  // Pipeline
  Pipeline p;
  p.add(std::make_shared<op::Canny>(50,150,3,true));
  p.add(std::make_shared<op::Morph>(cv::MORPH_CLOSE, 3, 1));
  p.add(std::make_shared<op::Threshold>(128.0, cv::THRESH_BINARY));
  auto proc = p.run(Frame{img,"ui"});

  // ROI from interactive widget
  QImage maskQ = roiView_->maskImage();
  if (maskQ.isNull()){ QMessageBox::information(this, "Info", "Please draw an ROI."); return; }
  cv::Mat mask(maskQ.height(), maskQ.width(), CV_8UC1, const_cast<uchar*>(maskQ.bits()), maskQ.bytesPerLine());
  cv::Mat masked; proc.mat.copyTo(masked, mask);

  // Determine roi rect
  QRect qr = roiView_->roiRect();
  cv::Rect roi(qr.x(), qr.y(), qr.width(), qr.height());
  cv::Mat roiImg = masked(roi).clone();
  cv::Mat gray; if (roiImg.channels()==3) cv::cvtColor(roiImg, gray, cv::COLOR_BGR2GRAY); else gray=roiImg;

  // Extract contours
  std::vector<std::vector<cv::Point>> contours;
  cv::findContours(gray, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
  std::sort(contours.begin(), contours.end(), [](auto& a, auto& b){ return cv::contourArea(a) > cv::contourArea(b); });

  // Prepare points in full image coords
  std::vector<cv::Point2f> ptsA, ptsB;
  if (contours.size() >= 1) for (auto& p: contours[0]) ptsA.push_back(cv::Point2f(p) + cv::Point2f((float)roi.x,(float)roi.y));
  if (contours.size() >= 2) for (auto& p: contours[1]) ptsB.push_back(cv::Point2f(p) + cv::Point2f((float)roi.x,(float)roi.y));

  Circle circA{{0,0},0}, circB{{0,0},0};
  bool hasA=false, hasB=false;
  if (ptsA.size() >= 12){ auto c = fitCircleKasa(ptsA); circA=c; hasA=true; }
  if (ptsB.size() >= 12){ auto c = fitCircleKasa(ptsB); circB=c; hasB=true; }

  // Lines: use top/bottom separation within roi
  std::vector<cv::Point2f> topPts, botPts;
  for (auto& c : contours){
    for (auto& p : c){
      cv::Point pt = p + cv::Point(roi.x, roi.y);
      if (pt.y < roi.y + roi.height*0.5) topPts.push_back(pt);
      else botPts.push_back(pt);
    }
  }
  Line2D Ltop{{0,0},{1,0}}, Lbot{{0,0},{1,0}}; bool hasTop=false, hasBot=false;
  if (topPts.size() >= 20){ Ltop = fitLineLSQ(topPts); hasTop=true; }
  if (botPts.size() >= 20){ Lbot = fitLineLSQ(botPts); hasBot=true; }

  // Calibration
  Calibration cal; cal.scale_mm_per_px = ui->spinScale->value();

  // Clear table
  clearResults();

  // Gauges
  if (hasTop && hasBot){
    auto mGap = gauge::metricLineGapMM(Ltop, Lbot, roi, cal);
    auto mPar = gauge::metricParallelismDeg(Ltop, Lbot);
    double specGap = ui->spinSpecLineGap->value();
    double tolGap  = ui->spinTolLineGap->value();
    bool okGap = std::abs(mGap.value_mm - specGap) <= tolGap + 1e-9;
    appendResultRow("Line gap (mm)", QString::number(mGap.value_mm,'f',3),
                    QString("%1±%2").arg(specGap,0,'f',3).arg(tolGap,0,'f',3), okGap);
    double tolPar = ui->spinTolParallelDeg->value();
    bool okPar = std::abs(mPar.value_mm) <= tolPar + 1e-9;
    appendResultRow("Parallelism (deg)", QString::number(mPar.value_mm,'f',3),
                    QString("≤%1").arg(tolPar,0,'f',3), okPar);
  } else {
    appendResultRow("Line gap (mm)", "N/A", "-", false);
    appendResultRow("Parallelism (deg)", "N/A", "-", false);
  }

  if (hasA){
    auto mDia = gauge::metricDiameterMM(circA, cal);
    double specDia = ui->spinSpecDiameter->value();
    double tolDia  = ui->spinTolDiameter->value();
    bool okDia = std::abs(mDia.value_mm - specDia) <= tolDia + 1e-9;
    appendResultRow("Diameter A (mm)", QString::number(mDia.value_mm,'f',3),
                    QString("%1±%2").arg(specDia,0,'f',3).arg(tolDia,0,'f',3), okDia);

    auto mRnd = gauge::metricRoundnessMM(ptsA, cal);
    double tolRnd = ui->spinTolRoundness->value();
    bool okRnd = (mRnd.value_mm <= tolRnd + 1e-9);
    appendResultRow("Roundness A (mm)", QString::number(mRnd.value_mm,'f',3),
                    QString("≤%1").arg(tolRnd,0,'f',3), okRnd);
  } else {
    appendResultRow("Diameter A (mm)", "N/A", "-", false);
    appendResultRow("Roundness A (mm)", "N/A", "-", false);
  }

  if (hasA && hasB){
    auto mCon = gauge::metricConcentricityMM(circA, circB, cal);
    double tolCon = ui->spinTolConcentric->value();
    bool okCon = (mCon.value_mm <= tolCon + 1e-9);
    appendResultRow("Concentricity A-B (mm)", QString::number(mCon.value_mm,'f',3),
                    QString("≤%1").arg(tolCon,0,'f',3), okCon);
  } else {
    appendResultRow("Concentricity A-B (mm)", "N/A", "-", false);
  }

  // Visualization
  cv::Mat vis = img.clone();
  // ROI overlay from mask
  cv::Mat maskColor; cv::cvtColor(mask, maskColor, cv::COLOR_GRAY2BGR);
  cv::Mat overlay = vis.clone();
  overlay.setTo(cv::Scalar(0,255,255), mask);
  cv::addWeighted(overlay, 0.3, vis, 0.7, 0.0, vis);

  // Draw circles
  if (hasA) cv::circle(vis, circA.c, (int)std::round(circA.r), {0,255,0}, 2, cv::LINE_AA);
  if (hasB) cv::circle(vis, circB.c, (int)std::round(circB.r), {255,0,0}, 2, cv::LINE_AA);
  // Draw lines
  auto drawLine = [&](const Line2D& L, const cv::Scalar& col){
    cv::Point2f p0 = L.p - L.v*1000.f, p1 = L.p + L.v*1000.f;
    cv::line(vis, p0, p1, col, 1, cv::LINE_AA);
  };
  if (hasTop) drawLine(Ltop, {0,255,255});
  if (hasBot) drawLine(Lbot, {0,128,255});

  ui->labelOutput->setPixmap(QPixmap::fromImage(matToQ(vis)).scaled(ui->labelOutput->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::onModeRect(){ roiView_->setMode(RoiView::Mode::Rect); }
void MainWindow::onModeRing(){ roiView_->setMode(RoiView::Mode::Ring); }
void MainWindow::onModePoly(){ roiView_->setMode(RoiView::Mode::Polygon); }
void MainWindow::onClearRoi(){ roiView_->clearRoi(); }
