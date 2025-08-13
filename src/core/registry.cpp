#include "core/registry.h"
#include <stdexcept>
namespace mp {
Registry& Registry::inst(){ static Registry r; return r; }
void Registry::reg(const std::string& key, Factory f){ fs_[key]=std::move(f); }
std::shared_ptr<IModule> Registry::make(const std::string& key) const{
  auto it=fs_.find(key); if (it==fs_.end()) throw std::runtime_error("Unknown module: "+key);
  return it->second();
}
std::vector<std::string> Registry::keys() const{
  std::vector<std::string> k; k.reserve(fs_.size());
  for (auto& kv:fs_) k.push_back(kv.first);
  return k;
}
}
