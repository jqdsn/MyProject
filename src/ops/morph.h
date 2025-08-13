#pragma once
#include "core/pipeline.h"
#include <opencv2/imgproc.hpp>
namespace mp::op {
class Morph : public mp::IModule {
public:
  Morph(int op=cv::MORPH_OPEN, int ksize=3, int iters=1): op_(op), k_(ksize), it_(iters) {}
  std::string name() const override { return "op.morph"; }
  mp::Frame process(const mp::Frame& in) override;
private: int op_; int k_; int it_;
};
}
