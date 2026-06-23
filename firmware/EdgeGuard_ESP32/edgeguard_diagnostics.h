#pragma once
#include <stddef.h>
#include <stdint.h>

#include "edgeguard_types.h"

enum class TaskId : uint8_t { SENSOR, CONTROL, WEB, HEARTBEAT, COUNT };

struct TaskDiagnostics {
  uint32_t heartbeatCount = 0;
  uint32_t lastHeartbeatMs = 0;
  uint32_t lastLoopDurationMs = 0;
  uint32_t maxLoopDurationMs = 0;
  uint32_t expectedPeriodMs = 0;
  uint32_t maxJitterMs = 0;
  uint32_t deadlineMissCount = 0;
  bool healthy = false;
};

struct DiagnosticsSnapshot {
  uint32_t uptimeMs = 0;
  uint32_t freeHeapBytes = 0;
  uint32_t minFreeHeapBytes = 0;
  uint32_t resetReason = 0;
  bool wifiConnected = false;
  bool apMode = false;
  uint8_t dhtFailCount = 0;
  uint8_t ultrasonicFailCount = 0;
  FaultCode activeFault = FaultCode::NONE;
  bool watchdogFault = false;
  TaskDiagnostics tasks[(size_t)TaskId::COUNT];
};

void diagnosticsInit();
void diagnosticsTaskHeartbeat(TaskId id, uint32_t nowMs, uint32_t loopDurationMs);
void diagnosticsSetExpectedPeriod(TaskId id, uint32_t periodMs);
void diagnosticsUpdateSensorFailures(uint8_t dhtFailCount, uint8_t ultrasonicFailCount);
void diagnosticsSetWatchdogFault(bool enabled);
DiagnosticsSnapshot diagnosticsSnapshot(const SystemSnapshot& sys, const ControlContext& ctx);
const char* taskName(TaskId id);
