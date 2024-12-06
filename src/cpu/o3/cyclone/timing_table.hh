#ifndef __CPU_O3_TIMING_TABLE_HH__
#define __CPU_O3_TIMING_TABLE_HH__

#include <vector>
#include "cpu/o3/fu_pool.hh"
#include "cpu/op_class.hh"
#include "cpu/o3/dyn_inst.hh"
namespace gem5 {

namespace o3 {

namespace cyclone {

class TimingTable {
private:
  std::vector<int> regReadyTime;

public:
  TimingTable() {}
  TimingTable(int numPhysicalRegs);
  int getReadyTime(int regIdx) const;
  void setReadyTime(int regIdx, int readyTime);
  void reset();
    void calculateDelayAndSetReadyTime(const DynInstPtr &inst,
                                       const FUPool &fuPool);
};

} // namespace cyclone
} // namespace o3
} // namespace gem5

#endif // __CPU_O3_TIMING_TABLE_HH__
