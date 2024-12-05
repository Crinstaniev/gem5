#ifndef __CPU_O3_TIMING_TABLE_HH__
#define __CPU_O3_TIMING_TABLE_HH__

#include <vector>

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
};

} // namespace cyclone
} // namespace o3
} // namespace gem5

#endif // __CPU_O3_TIMING_TABLE_HH__
