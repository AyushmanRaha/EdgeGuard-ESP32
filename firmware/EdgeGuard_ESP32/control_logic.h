#pragma once
#include "types.h"

struct ControlMemory {
  uint32_t lastOccupiedMs = 0;
  bool hasEverSeenOccupancy = false;
  bool temperatureAlertLatched = false;
};

struct ControlDecision {
  SystemSnapshot system;
  bool sensorStale = false;
  bool instantOccupied = false;
};

ControlDecision computeControlDecision(const SensorSnapshot& sensor, const SystemSnapshot& previousSystem, ControlMemory& memory, uint32_t nowMs);
