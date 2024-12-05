#ifndef __CPU_O3_TIMING_TABLE_HH__
#define __CPU_O3_TIMING_TABLE_HH__

#include <vector>

namespace gem5
{

namespace o3
{

namespace cyclone {
class TimingTable
{
  private:
    std::vector<int> regReadyTime; 

  public:
    TimingTable(int numPhysicalRegs)
        : regReadyTime(numPhysicalRegs, 0) {}


    int getReadyTime(int regIdx) const {
        return regReadyTime[regIdx];
    }


    void setReadyTime(int regIdx, int readyTime) {
        regReadyTime[regIdx] = readyTime;
    }


    void reset() {
        std::fill(regReadyTime.begin(), regReadyTime.end(), 0);
    }
};



} // namespace cyclone
} // namespace o3
} // namespace gem5

#endif // __CPU_O3_TIMING_TABLE_HH__
