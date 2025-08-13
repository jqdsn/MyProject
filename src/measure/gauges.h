#pragma once
#include <opencv2/core.hpp>
#include <vector>
#include "measure/geometry.h"
#include "measure/calibration.h"


namespace mp::gauge {

struct Metric {
  std::string name;
  double value_mm;     // value in mm
  std::string note;    // optional details
};

// compute distance between two lines as average perpendicular distance between
// projections within a bounding box region (px), then to mm via Calibration.
double lineLineDistancePx(const mp::Line2D& L1, const mp::Line2D& L2, const cv::Rect& roiPx);
double lineLineParallelismDeg(const mp::Line2D& L1, const mp::Line2D& L2);

// distance between circle centers (px) and concentricity
double circleCenterDistancePx(const mp::Circle& A, const mp::Circle& B);
double diameterPx(const mp::Circle& C);
double roundnessPx(const std::vector<cv::Point2f>& contourPts); // max radial deviation

// Convenience wrappers returning Metric in mm using Calibration
Metric metricLineGapMM(const mp::Line2D& L1, const mp::Line2D& L2, const cv::Rect& roiPx, const mp::Calibration& cal);
Metric metricParallelismDeg(const mp::Line2D& L1, const mp::Line2D& L2);
Metric metricCirclesGapMM(const mp::Circle& A, const mp::Circle& B, const mp::Calibration& cal);
Metric metricDiameterMM(const mp::Circle& C, const mp::Calibration& cal);
Metric metricRoundnessMM(const std::vector<cv::Point2f>& contourPts, const mp::Calibration& cal);
Metric metricConcentricityMM(const mp::Circle& A, const mp::Circle& B, const mp::Calibration& cal);

} // namespace mp::gauge
