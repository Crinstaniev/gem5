#include "cpu/o3/cyclone/timing_table.hh"
#include "cpu/o3/dyn_inst.hh"
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

void TimingTable::calculateDelayAndSetReadyTime(const DynInstPtr &inst,
                                                FUPool *fuPool) {

  OpClass opClass = inst->opClass();
  Cycles opLatency = fuPool->getOpLatency(opClass);
  int operandLatency = inst->getOperandLatency();
  int totalDelay = operandLatency + opLatency;
  for (size_t i = 0; i < inst->numDestRegs(); ++i) {
    int regIdx = inst->renamedDestIdx(i)->index();
    setReadyTime(regIdx, totalDelay);
  }
}

void TimingTable::calculateOperandLatency(const DynInstPtr &inst) {
  int operandLatency = 0;

  for (int i = 0; i < inst->numSrcRegs(); ++i) {
    int regReadyTime = getReadyTime(inst->renamedSrcIdx(i)->index());
    operandLatency = std::max(operandLatency, regReadyTime);
  }
  inst->setOperandLatency(operandLatency);
}

} // namespace cyclone
} // namespace o3
} // namespace gem5
