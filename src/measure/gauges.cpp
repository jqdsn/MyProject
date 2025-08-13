#include "measure/gauges.h"
#include <cmath>
#include <algorithm>

constexpr double M_PI = 3.14159265358979323846;

namespace mp::gauge {

static double angleRad(const mp::Line2D& L){
  return std::atan2(L.v.y, L.v.x);
}

double lineLineParallelismDeg(const mp::Line2D& L1, const mp::Line2D& L2){
  double a1 = angleRad(L1), a2 = angleRad(L2);
  double d = std::fabs(a1 - a2);
  // normalize to [0, pi/2]
  while (d > M_PI) d -= 2*M_PI;
  if (d > M_PI/2) d = M_PI - d;
  return d * 180.0 / M_PI;
}

static double signedDistance(const cv::Point2f& x, const mp::Line2D& L){
  cv::Point2f n{-L.v.y, L.v.x}; // perpendicular
  double den = std::sqrt(n.x*n.x + n.y*n.y);
  if (den==0) return 0.0;
  cv::Point2f w = x - L.p;
  return (w.x*n.x + w.y*n.y) / den;
}

double lineLineDistancePx(const mp::Line2D& L1, const mp::Line2D& L2, const cv::Rect& roiPx){
  // sample K points along the projection of ROI center line onto L1
  const int K=50;
  double sum=0; int cnt=0;
  cv::Point2f center = (roiPx.tl() + roiPx.br())*0.5f;
  // span along L1 direction within roi width
  cv::Point2f dir = L1.v; double norm = std::sqrt(dir.x*dir.x + dir.y*dir.y);
  if (norm==0) return std::abs(signedDistance(center, L2) - signedDistance(center, L1));
  dir *= (float)(1.0 / norm);
  float span = std::max(roiPx.width, roiPx.height) * 0.5f;
  for (int i=0;i<K;++i){
    float t = (i/(float)(K-1) - 0.5f) * 2.f * span;
    cv::Point2f p = center + dir * t;
    if (roiPx.contains(p)){
      double d1 = signedDistance(p, L1);
      double d2 = signedDistance(p, L2);
      sum += std::abs(d2 - d1);
      ++cnt;
    }
  }
  return cnt>0? sum/cnt : std::abs(signedDistance(center, L2) - signedDistance(center, L1));
}

double circleCenterDistancePx(const mp::Circle& A, const mp::Circle& B){
  return std::hypot(double(A.c.x - B.c.x), double(A.c.y - B.c.y));
}

double diameterPx(const mp::Circle& C){ return C.r * 2.0; }

double roundnessPx(const std::vector<cv::Point2f>& contourPts){
  // Fit circle, compute max |ri - r_fit|
  if (contourPts.size() < 6) return 0.0;
  auto fit = mp::fitCircleKasa(contourPts);
  double maxdev = 0.0;
  for (auto& p : contourPts){
    double ri = std::hypot(double(p.x - fit.c.x), double(p.y - fit.c.y));
    maxdev = std::max(maxdev, std::abs(ri - fit.r));
  }
  return maxdev * 2.0; // roundness often reported as 2*max radial deviation
}

Metric metricLineGapMM(const mp::Line2D& L1, const mp::Line2D& L2, const cv::Rect& roiPx, const mp::Calibration& cal){
  double px = lineLineDistancePx(L1, L2, roiPx);
  return {"line_gap", cal.toMM(px), ""};
}
Metric metricParallelismDeg(const mp::Line2D& L1, const mp::Line2D& L2){
  return {"parallelism", lineLineParallelismDeg(L1, L2), "deg"};
}
Metric metricCirclesGapMM(const mp::Circle& A, const mp::Circle& B, const mp::Calibration& cal){
  return {"circle_center_gap", cal.toMM(circleCenterDistancePx(A,B)), ""};
}
Metric metricDiameterMM(const mp::Circle& C, const mp::Calibration& cal){
  return {"diameter", cal.toMM(diameterPx(C)), "mm"};
}
Metric metricRoundnessMM(const std::vector<cv::Point2f>& contourPts, const mp::Calibration& cal){
  return {"roundness", cal.toMM(roundnessPx(contourPts)), "mm"};
}
Metric metricConcentricityMM(const mp::Circle& A, const mp::Circle& B, const mp::Calibration& cal){
  return {"concentricity", cal.toMM(circleCenterDistancePx(A,B)), "mm"};
}

} // namespace mp::gauge
