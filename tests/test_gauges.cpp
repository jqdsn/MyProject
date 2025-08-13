#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "measure/gauges.h"

using namespace mp;

TEST(Gauges, ParallelismZeroForSameDir){
  Line2D a{{0,0},{1,0}}, b{{0,5},{2,0}};
  auto deg = gauge::metricParallelismDeg(a,b).value_mm; // note: we'll store degrees in value_mm for simplicity
  EXPECT_LT(deg, 1e-3);
}

TEST(Gauges, LineGapRoughlyConstant){
  Line2D a{{0,0},{1,0}}, b{{0,10},{1,0}};
  cv::Rect roi(0,0,100,100);
  Calibration cal; cal.scale_mm_per_px = 0.1;
  auto m = gauge::metricLineGapMM(a,b, roi, cal);
  EXPECT_NEAR(m.value_mm, cal.toMM(10.0), 0.5);
}

TEST(Gauges, CircleMetrics){
  Circle A{{50,50}, 20}, B{{60,50}, 10};
  Calibration cal; cal.scale_mm_per_px = 0.05;
  auto d = gauge::metricDiameterMM(A, cal);
  auto con = gauge::metricConcentricityMM(A,B, cal);
  EXPECT_NEAR(d.value_mm, cal.toMM(40.0), 1e-6);
  EXPECT_NEAR(con.value_mm, cal.toMM(10.0), 1e-6);
}

TEST(Gauges, RoundnessSynthetic){
  // perfect circle; roundness ~0 (within tolerance due to integer sampling)
  std::vector<cv::Point2f> pts;
  cv::Point2f c(100,100); float r=30;
  for (int i=0;i<180;++i){
    float t = float(i) * float(CV_PI/90.0);
    pts.push_back(c + cv::Point2f(std::cos(t), std::sin(t))*r);
  }
  Calibration cal; cal.scale_mm_per_px = 0.02;
  auto m = gauge::metricRoundnessMM(pts, cal);
  EXPECT_LT(m.value_mm, 0.05); // 50 microns at this scale
}
