#include "ops/morph.h"
#include <opencv2/imgproc.hpp>
namespace mp::op {
mp::Frame Morph::process(const mp::Frame& in){
  mp::Frame out=in; if (in.mat.empty()) return out;
  cv::Mat gray; if (in.mat.channels()==3) cv::cvtColor(in.mat, gray, cv::COLOR_BGR2GRAY); else gray=in.mat;
  cv::Mat bw; cv::threshold(gray, bw, 0,255, cv::THRESH_OTSU);
  cv::Mat k = cv::getStructuringElement(cv::MORPH_RECT, {k_,k_});
  cv::morphologyEx(bw, bw, op_, k, {}, it_);
  cv::cvtColor(bw, out.mat, cv::COLOR_GRAY2BGR);
  out.tag = in.tag + "|morph"; return out;
}
}
