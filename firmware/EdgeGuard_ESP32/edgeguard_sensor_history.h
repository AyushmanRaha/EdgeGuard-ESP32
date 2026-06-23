#pragma once
#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "edgeguard_types.h"

struct SensorHistoryEntry {
  uint32_t timestampMs = 0;
  float temperatureC = EDGEGUARD_NAN;
  float humidity = EDGEGUARD_NAN;
  bool dhtOk = false;
  uint16_t distanceCm = 0;
  bool distanceOk = false;
  bool lightIsDark = false;
  bool occupied = false;
  bool relay1On = false;
  bool relay2On = false;
  State state = State::BOOT;
  Mode mode = Mode::AUTO;
};

class EdgeGuardSensorHistory {
 public:
  static constexpr size_t kCapacity = SENSOR_HISTORY_SIZE;
  void push(const SensorHistoryEntry& entry);
  size_t count() const;
  bool get(size_t chronologicalIndex, SensorHistoryEntry* out) const;
  void clear();

 private:
  SensorHistoryEntry entries_[kCapacity] = {};
  size_t head_ = 0;
  size_t count_ = 0;
};
