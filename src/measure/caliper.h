#pragma once
#include <opencv2/core.hpp>
namespace mp {
struct CaliperResult { int index=-1; cv::Point2f position; float response=0.f; };
CaliperResult caliper1D(const cv::Mat& gray, cv::Point2f a, cv::Point2f b, int samples=256);
}
