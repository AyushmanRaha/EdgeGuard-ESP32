#include "edgeguard_diagnostics.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>
#endif

static TaskDiagnostics gTasks[(size_t)TaskId::COUNT];
static uint8_t gDhtFails = 0;
static uint8_t gUltrasonicFails = 0;
static bool gWatchdogFault = false;

static uint32_t absDiff(uint32_t a, uint32_t b) {
  return a > b ? a - b : b - a;
}
static uint32_t maxU32(uint32_t a, uint32_t b) {
  return a > b ? a : b;
}

void diagnosticsInit() {
  for (size_t i = 0; i < (size_t)TaskId::COUNT; ++i) gTasks[i] = TaskDiagnostics{};
  gDhtFails = 0;
  gUltrasonicFails = 0;
  gWatchdogFault = false;
}

void diagnosticsSetExpectedPeriod(TaskId id, uint32_t periodMs) {
  if ((size_t)id >= (size_t)TaskId::COUNT) return;
  gTasks[(size_t)id].expectedPeriodMs = periodMs;
}

void diagnosticsTaskHeartbeat(TaskId id, uint32_t nowMs, uint32_t loopDurationMs) {
  if ((size_t)id >= (size_t)TaskId::COUNT) return;
  TaskDiagnostics& t = gTasks[(size_t)id];
  if (t.lastHeartbeatMs != 0 && t.expectedPeriodMs != 0) {
    const uint32_t actual = nowMs - t.lastHeartbeatMs;
    const uint32_t jitter = absDiff(actual, t.expectedPeriodMs);
    t.maxJitterMs = maxU32(t.maxJitterMs, jitter);
    // A deadline miss is counted only after a tolerance band to avoid false positives from normal
    // Wi-Fi/RTOS jitter.
    const uint32_t tolerance = maxU32(100, t.expectedPeriodMs / 2);
    if (actual > t.expectedPeriodMs + tolerance) ++t.deadlineMissCount;
  }
  ++t.heartbeatCount;
  t.lastHeartbeatMs = nowMs;
  t.lastLoopDurationMs = loopDurationMs;
  t.maxLoopDurationMs = maxU32(t.maxLoopDurationMs, loopDurationMs);
  t.healthy = true;
}

void diagnosticsUpdateSensorFailures(uint8_t dhtFailCount, uint8_t ultrasonicFailCount) {
  gDhtFails = dhtFailCount;
  gUltrasonicFails = ultrasonicFailCount;
}

void diagnosticsSetWatchdogFault(bool enabled) {
  gWatchdogFault = enabled;
}

DiagnosticsSnapshot diagnosticsSnapshot(const SystemSnapshot& sys, const ControlContext& ctx) {
  DiagnosticsSnapshot snap;
#ifdef ARDUINO
  snap.uptimeMs = millis();
  snap.freeHeapBytes = ESP.getFreeHeap();
  snap.minFreeHeapBytes = ESP.getMinFreeHeap();
  snap.resetReason = (uint32_t)esp_reset_reason();
  snap.wifiConnected = WiFi.status() == WL_CONNECTED;
  snap.apMode = (WiFi.getMode() & WIFI_AP) != 0;
#endif
  snap.dhtFailCount = ctx.dhtFailCount ? ctx.dhtFailCount : gDhtFails;
  snap.ultrasonicFailCount = ctx.ultrasonicFailCount ? ctx.ultrasonicFailCount : gUltrasonicFails;
  snap.activeFault = sys.faultCode;
  snap.watchdogFault = gWatchdogFault;
  for (size_t i = 0; i < (size_t)TaskId::COUNT; ++i) snap.tasks[i] = gTasks[i];
  return snap;
}

const char* taskName(TaskId id) {
  switch (id) {
    case TaskId::SENSOR:
      return "SensorTask";
    case TaskId::CONTROL:
      return "ControlTask";
    case TaskId::WEB:
      return "WebTask";
    case TaskId::HEARTBEAT:
      return "HeartbeatTask";
    case TaskId::COUNT:
      break;
  }
  return "UnknownTask";
}
