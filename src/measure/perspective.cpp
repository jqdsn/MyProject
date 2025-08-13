#include "measure/perspective.h"
#include <opencv2/imgproc.hpp>
namespace mp {
cv::Mat estimateH(const std::array<cv::Point2f,4>& src, const std::array<cv::Point2f,4>& dst){
  std::vector<cv::Point2f> s(src.begin(), src.end()), d(dst.begin(), dst.end());
  return cv::getPerspectiveTransform(s,d);
}
cv::Mat warpWithH(const cv::Mat& img, const cv::Mat& H, cv::Size outSize){
  cv::Mat out; cv::warpPerspective(img, out, H, outSize, cv::INTER_LINEAR, cv::BORDER_REPLICATE);
  return out;
}
}
