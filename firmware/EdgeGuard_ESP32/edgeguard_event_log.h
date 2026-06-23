#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config.h"

enum class EventLevel : uint8_t { INFO, WARN, FAULT, SECURITY };

struct EventLogEntry {
  uint32_t timestampMs = 0;
  EventLevel level = EventLevel::INFO;
  char message[96] = {};
};

const char* eventLevelName(EventLevel level);

class EdgeGuardEventLog {
 public:
  static constexpr size_t kCapacity = EVENT_LOG_SIZE;
  static constexpr size_t kMaxEventLength = 96;

  void push(uint32_t timestampMs, EventLevel level, const char* message);
  void push(EventLevel level, const char* message) {
    push(0, level, message);
  }
  void push(const char* message) {
    push(0, EventLevel::INFO, message);
  }
  size_t count() const;
  EventLogEntry getEntry(size_t chronologicalIndex) const;
  const char* get(size_t chronologicalIndex) const;
  void clear();

 private:
  EventLogEntry events_[kCapacity] = {};
  size_t head_ = 0;
  size_t count_ = 0;
};
