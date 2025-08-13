#pragma once
#include <opencv2/core.hpp>
#include <array>
namespace mp {
cv::Mat estimateH(const std::array<cv::Point2f,4>& src, const std::array<cv::Point2f,4>& dst);
cv::Mat warpWithH(const cv::Mat& img, const cv::Mat& H, cv::Size outSize);
}
