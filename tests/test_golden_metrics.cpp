
#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "core/pipeline.h"
#include "ops/canny.h"
#include "ops/morph.h"
#include "ops/threshold.h"
#include "measure/geometry.h"
#include "measure/gauges.h"
#include "measure/calibration.h"

using namespace mp;

static cv::Rect full(const cv::Mat& m){ return {0,0,m.cols,m.rows}; }

static void extractContours(const cv::Mat& bgr, std::vector<std::vector<cv::Point>>& contours){
  cv::Mat gray;
  if (bgr.channels()==3) cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY); else gray=bgr;
  cv::Mat e; cv::Canny(gray, e, 50, 150, 3, true);
  cv::Mat bw; cv::threshold(e, bw, 0,255, cv::THRESH_OTSU);
  cv::findContours(bw, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
  std::sort(contours.begin(), contours.end(), [](auto& a, auto& b){ return cv::contourArea(a) > cv::contourArea(b); });
}

TEST(Golden, ParallelLinesGapAndParallelism){
  cv::Mat img = cv::imread("tests/data/parallel_lines.png");
  ASSERT_FALSE(img.empty());
  std::vector<std::vector<cv::Point>> cs; extractContours(img, cs);
  // split into top/bottom sets by y
  std::vector<cv::Point2f> topPts, botPts;
  for (auto& c: cs){
    for (auto&p: c){
      if (p.y < img.rows*0.5) topPts.push_back(p);
      else botPts.push_back(p);
    }
  }
  ASSERT_GE(topPts.size(), 50);
  ASSERT_GE(botPts.size(), 50);
  auto Ltop = fitLineLSQ(topPts);
  auto Lbot = fitLineLSQ(botPts);
  Calibration cal; cal.scale_mm_per_px = 0.05; // 50 µm/px
  auto gap = gauge::metricLineGapMM(Ltop, Lbot, full(img), cal);
  auto par = gauge::metricParallelismDeg(Ltop, Lbot);
  // We drew lines 100 px apart; allow a tolerance of ±1 px
  EXPECT_NEAR(gap.value_mm, cal.toMM(100.0), cal.toMM(1.0));
  EXPECT_LE(std::abs(par.value_mm), 0.05); // deg
}

TEST(Golden, ConcentricRingsConcentricityZero){
  cv::Mat img = cv::imread("tests/data/rings_concentric.png");
  ASSERT_FALSE(img.empty());
  std::vector<std::vector<cv::Point>> cs; extractContours(img, cs);
  ASSERT_GE(cs.size(), 2);
  std::vector<cv::Point2f> a, b;
  for (auto&p: cs[0]) a.push_back(p);
  for (auto&p: cs[1]) b.push_back(p);
  auto A = fitCircleKasa(a), B = fitCircleKasa(b);
  Calibration cal; cal.scale_mm_per_px = 0.02;
  auto conc = gauge::metricConcentricityMM(A,B,cal);
  EXPECT_LE(conc.value_mm, cal.toMM(1.0)); // within 1 px
}

TEST(Golden, OffsetRingsConcentricityKnown){
  cv::Mat img = cv::imread("tests/data/rings_offset.png");
  ASSERT_FALSE(img.empty());
  std::vector<std::vector<cv::Point>> cs; extractContours(img, cs);
  ASSERT_GE(cs.size(), 2);
  std::vector<cv::Point2f> a, b;
  for (auto&p: cs[0]) a.push_back(p);
  for (auto&p: cs[1]) b.push_back(p);
  auto A = fitCircleKasa(a), B = fitCircleKasa(b);
  Calibration cal; cal.scale_mm_per_px = 0.02;
  auto conc = gauge::metricConcentricityMM(A,B,cal);
  // We shifted by 20 px: allow ±1 px
  EXPECT_NEAR(conc.value_mm, cal.toMM(20.0), cal.toMM(1.0));
}

TEST(Golden, RoundnessWithinBounds){
  cv::Mat img = cv::imread("tests/data/circle_roundness.png");
  ASSERT_FALSE(img.empty());
  std::vector<std::vector<cv::Point>> cs; extractContours(img, cs);
  ASSERT_GE(cs.size(), 1);
  std::vector<cv::Point2f> pts; for (auto&p: cs[0]) pts.push_back(p);
  Calibration cal; cal.scale_mm_per_px = 0.02;
  auto rnd = gauge::metricRoundnessMM(pts, cal);
  // Expected roundness roughly 2*amplitude (~2 px) => ~4 px; allow range
  EXPECT_GE(rnd.value_mm, cal.toMM(2.0));
  EXPECT_LE(rnd.value_mm, cal.toMM(6.0));
}
