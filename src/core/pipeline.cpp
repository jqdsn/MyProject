#include "core/pipeline.h"
namespace mp {
Frame Pipeline::run(const Frame& input) const {
  Frame cur = input;
  for (auto& m : mods_) cur = m->process(cur);
  return cur;
}
}
