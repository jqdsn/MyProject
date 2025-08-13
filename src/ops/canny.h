#pragma once
#include "core/pipeline.h"
namespace mp::op {
class Canny : public mp::IModule {
public:
  Canny(double t1=50.0, double t2=150.0, int ap=3, bool L2=true)
    : t1_(t1), t2_(t2), ap_(ap), l2_(L2) {}
  std::string name() const override { return "op.canny"; }
  mp::Frame process(const mp::Frame& in) override;
private: double t1_, t2_; int ap_; bool l2_;
};
}
