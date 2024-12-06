#include "cpu/o3/cyclone/utilities.hh"

namespace gem5 {
namespace o3 {
namespace cyclone {

std::string getInstName(const DynInstPtr &inst) {
  return inst->staticInst->getName();
}

} // namespace cyclone
} // namespace o3
} // namespace gem5
