#include "cpu/o3/cyclone/timing_table.hh"

#include <vector>

namespace gem5 {

namespace o3 {

namespace cyclone {

TimingTable::TimingTable(int numPhysicalRegs)
    : regReadyTime(numPhysicalRegs, 0) {}

int TimingTable::getReadyTime(int regIdx) const {
  return this->regReadyTime[regIdx];
}

void TimingTable::setReadyTime(int regIdx, int readyTime) {
  regReadyTime[regIdx] = readyTime;
}

void TimingTable::reset() {
  std::fill(regReadyTime.begin(), regReadyTime.end(), 0);
}

} // namespace cyclone
} // namespace o3
} // namespace gem5
