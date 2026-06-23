#pragma once

#include <Arduino.h>

#define EDGEGUARD_FIRMWARE_NAME "EdgeGuard ESP32 RTOS Smart Room Node"
#define EDGEGUARD_FIRMWARE_VERSION "1.1.0"
#define EDGEGUARD_BUILD_LABEL __DATE__ " " __TIME__

struct SensorHealth {
  uint16_t dhtConsecutiveFailures = 0;
  uint16_t ultrasonicConsecutiveFailures = 0;
  uint32_t dhtTotalFailures = 0;
  uint32_t ultrasonicTotalFailures = 0;
  uint32_t lastDhtOkMs = 0;
  uint32_t lastDistanceOkMs = 0;
};

struct TaskHealth {
  uint32_t lastSensorTaskHeartbeatMs = 0;
  uint32_t lastControlTaskHeartbeatMs = 0;
  uint32_t lastWebTaskHeartbeatMs = 0;
  uint32_t lastHeartbeatTaskHeartbeatMs = 0;
};

struct WiFiHealth {
  char mode[8] = "OFF";
  char reconnectState[24] = "not_started";
};

struct RuntimeHealth {
  uint32_t freeHeap = 0;
  uint32_t minFreeHeap = 0;
  uint32_t uptimeSeconds = 0;
  char resetReason[32] = "unknown";
  bool watchdogEnabled = false;
};

struct DiagnosticsSnapshot {
  SensorHealth sensor;
  TaskHealth tasks;
  WiFiHealth wifi;
  RuntimeHealth runtime;
};

void initDiagnostics();
void noteDhtResult(bool ok, uint32_t nowMs);
void noteDistanceResult(bool ok, uint32_t nowMs);
void noteSensorTaskHeartbeat(uint32_t nowMs);
void noteControlTaskHeartbeat(uint32_t nowMs);
void noteWebTaskHeartbeat(uint32_t nowMs);
void noteHeartbeatTaskHeartbeat(uint32_t nowMs);
void setWiFiDiagnostics(const char* mode, const char* reconnectState);
void setWatchdogEnabled(bool enabled);
DiagnosticsSnapshot getDiagnosticsSnapshot();
