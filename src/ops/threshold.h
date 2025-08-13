#pragma once
#include "core/pipeline.h"
#include <opencv2/imgproc.hpp>
namespace mp::op {
class Threshold : public mp::IModule {
public:
  Threshold(double thr=128.0, int type=cv::THRESH_BINARY): thr_(thr), type_(type) {}
  std::string name() const override { return "op.threshold"; }
  mp::Frame process(const mp::Frame& in) override;
private: double thr_; int type_;
};
}
