#include "measure/report.h"
#include <sstream>
namespace mp {
std::string toJson(const std::vector<Item>& items){
  std::ostringstream os; os << "{\"results\":[";
  for (size_t i=0;i<items.size();++i){
    const auto& it=items[i];
    os << "{\"name\":\"" << it.name << "\",\"value\":" << it.value << ",\"unit\":\"" << it.unit << "\"}";
    if (i+1<items.size()) os << ",";
  }
  os << "]}"; return os.str();
}
}
