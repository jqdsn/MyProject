#include "ops/canny.h"
#include <opencv2/imgproc.hpp>
namespace mp::op {
mp::Frame Canny::process(const mp::Frame& in){
  mp::Frame out=in; if (in.mat.empty()) return out;
  cv::Mat gray; if (in.mat.channels()==3) cv::cvtColor(in.mat, gray, cv::COLOR_BGR2GRAY); else gray=in.mat;
  cv::Mat e; cv::Canny(gray, e, t1_, t2_, ap_, l2_);
  cv::cvtColor(e, out.mat, cv::COLOR_GRAY2BGR);
  out.tag = in.tag + "|canny"; return out;
}
}
