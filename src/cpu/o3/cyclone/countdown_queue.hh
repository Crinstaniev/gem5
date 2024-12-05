#ifndef __CPU_O3_CYCLONE_COUNTDOWN_QUEUE_HH__
#define __CPU_O3_CYCLONE_COUNTDOWN_QUEUE_HH__

#include <random>
#include <vector>

#include "base/types.hh"
#include "cpu/o3/comm.hh"
#include "cpu/o3/dep_graph.hh"
#include "cpu/o3/dyn_inst_ptr.hh"
#include "cpu/o3/mem_dep_unit.hh"
#include "cpu/o3/store_set.hh"
#include "cpu/op_class.hh"
#include "cpu/timebuf.hh"

namespace gem5 {
namespace o3 {
namespace cyclone {

struct DynInstPtrHash {
  std::size_t operator()(const DynInstPtr &inst) const {
    return std::hash<const void *>()(inst.get());
  }
};

struct DynInstPtrEqual {
  bool operator()(const DynInstPtr &lhs, const DynInstPtr &rhs) const {
    return lhs.get() == rhs.get();
  }
};

/**
 * Assume the size of countdown queue to be infinite.
 */
struct CountdownQueueEntry {
  DynInstPtr inst;
  int countdown_half;
  int countdown;
};

// test
class CountdownQueue {
private:
  std::vector<CountdownQueueEntry> queue;

  // not very sure
  DependencyGraph<DynInstPtr> dependGraph;
  // MemDepUnit memDepUnit[MaxThreads];

public:
  // constructor
  CountdownQueue();

  // add instruction to the queue
  void addInstruction(const DynInstPtr &inst, int countdown);

  // decrement countdown for all entries in the queue
  void tick();

  // return instructions whose countdown has reached zero
  std::vector<CountdownQueueEntry> fetchReadyInstructions();

  // return if the queue is empty
  bool isEmpty() const { return this->queue.size() == 0; };

  int getSize() const { return this->queue.size(); }

  bool inQueue(const DynInstPtr &inst) const {
    for (const auto &entry : this->queue) {
      if (entry.inst == inst) {
        return true;
      }
    }
    return false;
  }

  /**
   * Debug Functions
   */
  void printQueue() const;

  int getRandomCountdown(int lo = 1, int hi = 10);
};

} // namespace cyclone
} // namespace o3
} // namespace gem5

#endif
