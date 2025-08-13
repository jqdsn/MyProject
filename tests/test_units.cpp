#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "measure/caliper.h"
#include "measure/geometry.h"
#include "measure/calibration.h"
using namespace mp;
TEST(Caliper, FindsEdge){
  cv::Mat img = cv::Mat::zeros(100,200,CV_8UC1);
  cv::rectangle(img, {80,40}, {160,60}, 255, cv::FILLED);
  auto r = caliper1D(img, {0,50}, {199,50}, 200);
  ASSERT_GE(r.index, 0);
}
TEST(Geometry, FitLineSlope){
  std::vector<cv::Point2f> pts;
  for (int i=0;i<50;++i) pts.push_back({(float)i, 2.f*(float)i + 1.f});
  auto L = fitLineLSQ(pts);
  EXPECT_NEAR(L.v.y/L.v.x, 2.0, 1e-2);
}
TEST(Calibration, PxToMm){
  Calibration c; c.scale_mm_per_px = 0.02;
  EXPECT_NEAR(c.toMM(100.0), 2.0, 1e-6);
}
