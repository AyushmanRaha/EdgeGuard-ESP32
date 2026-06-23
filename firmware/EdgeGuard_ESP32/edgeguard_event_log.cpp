#include "edgeguard_event_log.h"

#include <string.h>

const char* eventLevelName(EventLevel level) {
  switch (level) {
    case EventLevel::INFO:
      return "INFO";
    case EventLevel::WARN:
      return "WARN";
    case EventLevel::FAULT:
      return "FAULT";
    case EventLevel::SECURITY:
      return "SECURITY";
  }
  return "INFO";
}

void EdgeGuardEventLog::push(uint32_t timestampMs, EventLevel level, const char* message) {
  if (message == nullptr) message = "";
  events_[head_].timestampMs = timestampMs;
  events_[head_].level = level;
  strncpy(events_[head_].message, message, kMaxEventLength - 1);
  events_[head_].message[kMaxEventLength - 1] = '\0';
  head_ = (head_ + 1) % kCapacity;
  if (count_ < kCapacity) ++count_;
}

size_t EdgeGuardEventLog::count() const {
  return count_;
}

EventLogEntry EdgeGuardEventLog::getEntry(size_t chronologicalIndex) const {
  if (chronologicalIndex >= count_) return EventLogEntry{};
  const size_t start = (head_ + kCapacity - count_) % kCapacity;
  return events_[(start + chronologicalIndex) % kCapacity];
}

const char* EdgeGuardEventLog::get(size_t chronologicalIndex) const {
  return getEntry(chronologicalIndex).message;
}

void EdgeGuardEventLog::clear() {
  head_ = 0;
  count_ = 0;
  for (size_t i = 0; i < kCapacity; ++i) events_[i] = EventLogEntry{};
}
