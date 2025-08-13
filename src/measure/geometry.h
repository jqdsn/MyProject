#pragma once
#include <opencv2/core.hpp>
#include <vector>
namespace mp {
struct Line2D{ cv::Point2f p, v; };
struct Circle{ cv::Point2f c; float r; };
Line2D fitLineLSQ(const std::vector<cv::Point2f>& pts);
Circle fitCircleKasa(const std::vector<cv::Point2f>& pts);
inline float distancePointToLine(const cv::Point2f& x, const Line2D& L){
  cv::Point2f w = x-L.p; float num = std::abs(w.x*L.v.y - w.y*L.v.x);
  float den = std::sqrt(L.v.x*L.v.x + L.v.y*L.v.y); return den>0? num/den : 0.f;
}
}
