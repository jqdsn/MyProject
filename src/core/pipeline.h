#pragma once
#include <opencv2/core.hpp>
#include <memory>
#include <string>
#include <vector>

namespace mp {
struct Frame { cv::Mat mat; std::string tag; };
class IModule {
public: virtual ~IModule() = default;
  virtual std::string name() const = 0;
  virtual Frame process(const Frame& in) = 0;
};
class Pipeline {
public:
  using Ptr = std::shared_ptr<IModule>;
  void add(const Ptr& m){ mods_.push_back(m); }
  Frame run(const Frame& input) const;
  const std::vector<Ptr>& modules() const { return mods_; }
private: std::vector<Ptr> mods_;
};
}
