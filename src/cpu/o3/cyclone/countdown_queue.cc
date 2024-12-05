#include "cpu/o3/cyclone/countdown_queue.hh"

#include "base/logging.hh"
// #include "cpu/o3/cyclone/utilities.hh"
#include "cpu/o3/dyn_inst.hh"
#include "cpu/o3/fu_pool.hh"
#include "cpu/o3/limits.hh"
#include "debug/Cyclone.hh"
#include "debug/DCountdownQueue.hh"
#include "debug/IQ.hh"
#include "enums/OpClass.hh"
#include "params/BaseO3CPU.hh"
#include "sim/core.hh"

#include <random>

namespace gem5 {
namespace o3 {
namespace cyclone {

CountdownQueue::CountdownQueue() {}

void CountdownQueue::addInstruction(const DynInstPtr &inst, int countdown) {
  this->queue.push_back({inst, countdown / 2, countdown});

  DPRINTF(DCountdownQueue, "Added instruction %s to queue with countdown %d\n",
          inst->staticInst->getName(), countdown);
}

void CountdownQueue::tick() {
  // decrease the countdown for all entries in the queue
  for (auto &entry : this->queue) {
    entry.countdown--;
    entry.countdown_half--;
  }
}

std::vector<CountdownQueueEntry> CountdownQueue::fetchReadyInstructions() {
  std::vector<CountdownQueueEntry> readyInstructions;

  auto it = this->queue.begin();
  while (it != this->queue.end()) {
    if (it->countdown_half <= 0) {
      readyInstructions.push_back(*it);
      it = this->queue.erase(it);

      DPRINTF(DCountdownQueue, "Fetched ready instruction %s\n",
              it->inst->staticInst->getName());
    } else {
      it++;
    }
  }

  return readyInstructions;
}

/**
 * DEBUG FUNCTIONS
 */
void CountdownQueue::printQueue() const {
  DPRINTF(DCountdownQueue, "Printing countdown queue. Curr Tick: %d\n",
          curTick());

  for (const auto &entry : this->queue) {
    DPRINTF(DCountdownQueue, "inst: %s, countdown: %d\n",
            entry.inst->staticInst->getName(), entry.countdown);
  }
}

int CountdownQueue::getRandomCountdown(int lo, int hi) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(lo, hi);

  return dis(gen);
}

} // namespace cyclone
} // namespace o3
} // namespace gem5
