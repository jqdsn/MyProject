#pragma once
#include "core/pipeline.h"
#include <unordered_map>
#include <functional>
#include <vector>

namespace mp {
class Registry {
public:
  using Factory = std::function<std::shared_ptr<IModule>()>;
  static Registry& inst();
  void reg(const std::string& key, Factory f);
  std::shared_ptr<IModule> make(const std::string& key) const;
  std::vector<std::string> keys() const;
private: std::unordered_map<std::string, Factory> fs_;
};
}
