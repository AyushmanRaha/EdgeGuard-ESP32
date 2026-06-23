#include "edgeguard_sensor_history.h"

void EdgeGuardSensorHistory::push(const SensorHistoryEntry& entry) {
  entries_[head_] = entry;
  head_ = (head_ + 1) % kCapacity;
  if (count_ < kCapacity) ++count_;
}

size_t EdgeGuardSensorHistory::count() const {
  return count_;
}

bool EdgeGuardSensorHistory::get(size_t chronologicalIndex, SensorHistoryEntry* out) const {
  if (out == nullptr || chronologicalIndex >= count_) return false;
  const size_t start = (head_ + kCapacity - count_) % kCapacity;
  *out = entries_[(start + chronologicalIndex) % kCapacity];
  return true;
}

void EdgeGuardSensorHistory::clear() {
  head_ = 0;
  count_ = 0;
}
