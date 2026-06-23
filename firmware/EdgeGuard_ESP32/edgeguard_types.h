#pragma once

#include <stdint.h>

#ifndef EDGEGUARD_NAN
#define EDGEGUARD_NAN (0.0f / 0.0f)
#endif

enum class Mode : uint8_t { AUTO, MANUAL, AWAY };

enum class State : uint8_t { BOOT, CALIBRATING, AUTO_MONITORING, MANUAL_OVERRIDE, ALERT, FAULT };

enum class FaultCode : uint8_t { NONE, DHT, ULTRASONIC, DHT_AND_ULTRASONIC, TASK_WATCHDOG };

struct SensorSnapshot {
  float temperatureC = EDGEGUARD_NAN;
  float humidity = EDGEGUARD_NAN;
  bool dhtOk = false;
  uint16_t distanceCm = 0;
  bool distanceOk = false;
  bool ldrRawHigh = false;
  bool lightIsDark = false;
  uint32_t timestampMs = 0;
};

struct SystemSnapshot {
  Mode mode = Mode::AUTO;
  State state = State::BOOT;
  bool occupied = false;
  bool relay1On = false;
  bool relay2On = false;
  bool temperatureAlert = false;
  FaultCode faultCode = FaultCode::NONE;
  uint32_t timestampMs = 0;
};

struct ControlConfig {
  uint16_t occupiedDistanceCm = 120;
  uint32_t unoccupiedTimeoutMs = 15000;
  float tempAlertOnC = 35.0f;
  float tempAlertOffC = 33.0f;
  uint8_t maxConsecutiveSensorFailures = 5;
};

struct ControlContext {
  uint8_t dhtFailCount = 0;
  uint8_t ultrasonicFailCount = 0;
  bool temperatureAlertLatched = false;
  uint32_t lastOccupiedMs = 0;
  bool hasSeenOccupancy = false;
};

struct ControlResult {
  SystemSnapshot system;
  bool instantOccupied = false;
  bool sensorFault = false;
};
