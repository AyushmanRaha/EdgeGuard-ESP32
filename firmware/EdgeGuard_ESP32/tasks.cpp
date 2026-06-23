#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#if __has_include(<esp_task_wdt.h>)
#include <esp_task_wdt.h>
#include <esp_idf_version.h>
#define EDGEGUARD_HAS_TASK_WDT 1
#else
#define EDGEGUARD_HAS_TASK_WDT 0
#endif
#include "app_state.h"
#include "config.h"
#include "control.h"
#include "diagnostics.h"
#include "sensors.h"
#include "tasks.h"
#include "web_routes.h"

namespace {
bool gWatchdogAvailable = false;

void registerCurrentTaskWithWatchdog() {
#if EDGEGUARD_HAS_TASK_WDT
  if (gWatchdogAvailable) esp_task_wdt_add(nullptr);
#endif
}

void resetWatchdog() {
#if EDGEGUARD_HAS_TASK_WDT
  if (gWatchdogAvailable) esp_task_wdt_reset();
#endif
}

void SensorTask(void*) {
  registerCurrentTaskWithWatchdog();
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    noteSensorTaskHeartbeat(millis());
    const SensorSnapshot reading = readSensors();
    updateSensorSnapshot(reading);
    Serial.print("T=");
    if (reading.dhtOk) { Serial.print(reading.temperatureC, 1); Serial.print("C H="); Serial.print(reading.humidity, 1); Serial.print("%"); }
    else Serial.print("DHT_ERR");
    Serial.print(" DIST=");
    if (reading.distanceOk) { Serial.print(reading.distanceCm); Serial.print("cm"); }
    else Serial.print("NO_ECHO");
    Serial.print(" LIGHT="); Serial.println(reading.lightIsDark ? "DARK" : "BRIGHT");
    resetWatchdog();
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS));
  }
}

void ControlTask(void*) {
  registerCurrentTaskWithWatchdog();
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    noteControlTaskHeartbeat(millis());
    SensorSnapshot sensor;
    if (copySensorSnapshot(sensor)) updateControl(sensor);
    else { forceRelaysOff(); markCriticalFault("Sensor snapshot unavailable"); logEvent("Sensor snapshot unavailable"); }
    resetWatchdog();
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(CONTROL_TASK_PERIOD_MS));
  }
}

void WebTask(void*) {
  registerCurrentTaskWithWatchdog();
  for (;;) {
    noteWebTaskHeartbeat(millis());
    handleWebClient();
    resetWatchdog();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void HeartbeatTask(void*) {
  registerCurrentTaskWithWatchdog();
  bool ledState = false;
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    noteHeartbeatTaskHeartbeat(millis());
    SystemSnapshot sys;
    uint32_t periodMs = HEARTBEAT_NORMAL_MS;
    if (!copySystemSnapshot(sys)) {
      digitalWrite(PIN_LED_GREEN, LOW); digitalWrite(PIN_LED_RED, HIGH); periodMs = HEARTBEAT_FAULT_MS;
    } else {
      ledState = !ledState;
      if (sys.state == State::FAULT) {
        digitalWrite(PIN_LED_GREEN, LOW); digitalWrite(PIN_LED_RED, ledState ? HIGH : LOW); periodMs = HEARTBEAT_FAULT_MS;
      } else if (sys.state == State::ALERT) {
        digitalWrite(PIN_LED_GREEN, HIGH); digitalWrite(PIN_LED_RED, ledState ? HIGH : LOW); periodMs = HEARTBEAT_ALERT_MS;
      } else {
        digitalWrite(PIN_LED_RED, LOW); digitalWrite(PIN_LED_GREEN, ledState ? HIGH : LOW); periodMs = HEARTBEAT_NORMAL_MS;
      }
    }
    resetWatchdog();
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(periodMs));
  }
}

bool createTask(TaskFunction_t fn, const char* name, uint32_t stack, UBaseType_t priority, BaseType_t core) {
  const BaseType_t result = xTaskCreatePinnedToCore(fn, name, stack, nullptr, priority, nullptr, core);
  if (result != pdPASS) logEvent(String("Task creation failed: ") + name);
  return result == pdPASS;
}

void initWatchdog() {
#if EDGEGUARD_HAS_TASK_WDT
#if ESP_IDF_VERSION_MAJOR >= 5
  esp_task_wdt_config_t config = {};
  config.timeout_ms = WATCHDOG_TIMEOUT_S * 1000;
  config.idle_core_mask = 0;
  config.trigger_panic = true;
  const esp_err_t wdtResult = esp_task_wdt_init(&config);
  gWatchdogAvailable = (wdtResult == ESP_OK || wdtResult == ESP_ERR_INVALID_STATE);
#else
  const esp_err_t wdtResult = esp_task_wdt_init(WATCHDOG_TIMEOUT_S, true);
  gWatchdogAvailable = (wdtResult == ESP_OK || wdtResult == ESP_ERR_INVALID_STATE);
#endif
#endif
  setWatchdogEnabled(gWatchdogAvailable);
  if (!gWatchdogAvailable) logEvent("Task watchdog disabled by framework");
}
}

bool startAppTasks() {
  initWatchdog();
  bool ok = true;
  ok &= createTask(SensorTask, "SensorTask", SENSOR_TASK_STACK_WORDS, SENSOR_TASK_PRIORITY, SENSOR_TASK_CORE);
  ok &= createTask(ControlTask, "ControlTask", CONTROL_TASK_STACK_WORDS, CONTROL_TASK_PRIORITY, CONTROL_TASK_CORE);
  ok &= createTask(WebTask, "WebTask", WEB_TASK_STACK_WORDS, WEB_TASK_PRIORITY, WEB_TASK_CORE);
  ok &= createTask(HeartbeatTask, "HeartbeatTask", HEARTBEAT_TASK_STACK_WORDS, HEARTBEAT_TASK_PRIORITY, HEARTBEAT_TASK_CORE);
  if (!ok) { forceRelaysOff(); markCriticalFault("FreeRTOS task creation failed"); }
  return ok;
}
