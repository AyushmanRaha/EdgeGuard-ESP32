#include "diagnostics.h"
#include <esp_system.h>

namespace {
SensorHealth gSensorHealth;
TaskHealth gTaskHealth;
WiFiHealth gWiFiHealth;
RuntimeHealth gRuntimeHealth;
portMUX_TYPE gDiagMux = portMUX_INITIALIZER_UNLOCKED;

uint16_t inc16(uint16_t value) { return value < UINT16_MAX ? value + 1 : UINT16_MAX; }
void copyText(char* dst, size_t dstSize, const char* src) {
  if (dstSize == 0) return;
  strncpy(dst, src ? src : "", dstSize - 1);
  dst[dstSize - 1] = '\0';
}
const char* resetReasonText(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "power_on";
    case ESP_RST_EXT: return "external";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "interrupt_wdt";
    case ESP_RST_TASK_WDT: return "task_wdt";
    case ESP_RST_WDT: return "other_wdt";
    case ESP_RST_DEEPSLEEP: return "deep_sleep";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_SDIO: return "sdio";
    default: return "unknown";
  }
}
void refreshRuntimeLocked() {
  gRuntimeHealth.freeHeap = ESP.getFreeHeap();
  if (gRuntimeHealth.minFreeHeap == 0 || gRuntimeHealth.freeHeap < gRuntimeHealth.minFreeHeap) gRuntimeHealth.minFreeHeap = gRuntimeHealth.freeHeap;
  gRuntimeHealth.uptimeSeconds = millis() / 1000;
}
}

void initDiagnostics() {
  portENTER_CRITICAL(&gDiagMux);
  gSensorHealth = SensorHealth();
  gTaskHealth = TaskHealth();
  gWiFiHealth = WiFiHealth();
  gRuntimeHealth = RuntimeHealth();
  copyText(gRuntimeHealth.resetReason, sizeof(gRuntimeHealth.resetReason), resetReasonText(esp_reset_reason()));
  refreshRuntimeLocked();
  portEXIT_CRITICAL(&gDiagMux);
}

void noteDhtResult(bool ok, uint32_t nowMs) { portENTER_CRITICAL(&gDiagMux); if (ok) { gSensorHealth.dhtConsecutiveFailures = 0; gSensorHealth.lastDhtOkMs = nowMs; } else { gSensorHealth.dhtConsecutiveFailures = inc16(gSensorHealth.dhtConsecutiveFailures); gSensorHealth.dhtTotalFailures++; } portEXIT_CRITICAL(&gDiagMux); }
void noteDistanceResult(bool ok, uint32_t nowMs) { portENTER_CRITICAL(&gDiagMux); if (ok) { gSensorHealth.ultrasonicConsecutiveFailures = 0; gSensorHealth.lastDistanceOkMs = nowMs; } else { gSensorHealth.ultrasonicConsecutiveFailures = inc16(gSensorHealth.ultrasonicConsecutiveFailures); gSensorHealth.ultrasonicTotalFailures++; } portEXIT_CRITICAL(&gDiagMux); }
void noteSensorTaskHeartbeat(uint32_t nowMs) { portENTER_CRITICAL(&gDiagMux); gTaskHealth.lastSensorTaskHeartbeatMs = nowMs; portEXIT_CRITICAL(&gDiagMux); }
void noteControlTaskHeartbeat(uint32_t nowMs) { portENTER_CRITICAL(&gDiagMux); gTaskHealth.lastControlTaskHeartbeatMs = nowMs; portEXIT_CRITICAL(&gDiagMux); }
void noteWebTaskHeartbeat(uint32_t nowMs) { portENTER_CRITICAL(&gDiagMux); gTaskHealth.lastWebTaskHeartbeatMs = nowMs; portEXIT_CRITICAL(&gDiagMux); }
void noteHeartbeatTaskHeartbeat(uint32_t nowMs) { portENTER_CRITICAL(&gDiagMux); gTaskHealth.lastHeartbeatTaskHeartbeatMs = nowMs; portEXIT_CRITICAL(&gDiagMux); }
void setWiFiDiagnostics(const char* mode, const char* reconnectState) { portENTER_CRITICAL(&gDiagMux); copyText(gWiFiHealth.mode, sizeof(gWiFiHealth.mode), mode); copyText(gWiFiHealth.reconnectState, sizeof(gWiFiHealth.reconnectState), reconnectState); portEXIT_CRITICAL(&gDiagMux); }
void setWatchdogEnabled(bool enabled) { portENTER_CRITICAL(&gDiagMux); gRuntimeHealth.watchdogEnabled = enabled; portEXIT_CRITICAL(&gDiagMux); }
DiagnosticsSnapshot getDiagnosticsSnapshot() { DiagnosticsSnapshot out; portENTER_CRITICAL(&gDiagMux); refreshRuntimeLocked(); out.sensor = gSensorHealth; out.tasks = gTaskHealth; out.wifi = gWiFiHealth; out.runtime = gRuntimeHealth; portEXIT_CRITICAL(&gDiagMux); return out; }
