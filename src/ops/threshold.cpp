#include "ops/threshold.h"
#include <opencv2/imgproc.hpp>
namespace mp::op {
mp::Frame Threshold::process(const mp::Frame& in){
  mp::Frame out=in; if (in.mat.empty()) return out;
  cv::Mat gray; if (in.mat.channels()==3) cv::cvtColor(in.mat, gray, cv::COLOR_BGR2GRAY); else gray=in.mat;
  cv::Mat bw; cv::threshold(gray, bw, thr_, 255, type_);
  cv::cvtColor(bw, out.mat, cv::COLOR_GRAY2BGR);
  out.tag = in.tag + "|thr"; return out;
}
}
