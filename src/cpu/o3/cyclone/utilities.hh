#ifndef __CPU_O3_CYCLONE_UTILITIES_HH__
#define __CPU_O3_CYCLONE_UTILITIES_HH__

#include "cpu/o3/dyn_inst.hh"

#include <string>

namespace gem5 {
namespace o3 {
namespace cyclone {

/**
 * Get opname of a instruction
 */
std::string getInstName(const DynInstPtr &inst) {
  return inst->staticInst->getName();
}

} // namespace cyclone
} // namespace o3
} // namespace gem5

#endif // __CPU_O3_CYCLONE_UTILITIES_HH__
