#include "control.h"
#include "app_state.h"
#include "config.h"
#include "control_logic.h"
#include "diagnostics.h"

namespace {
void writeRelayPin(uint8_t pin, bool on) {
  digitalWrite(pin, RELAY_ACTIVE_LOW ? (on ? LOW : HIGH) : (on ? HIGH : LOW));
}
}

void applyRelays(bool relay1On, bool relay2On) {
  writeRelayPin(PIN_RELAY1, relay1On);
  writeRelayPin(PIN_RELAY2, relay2On);
}

void forceRelaysOff() { applyRelays(false, false); }

void setupPins() {
  pinMode(PIN_DHT, INPUT);
  pinMode(PIN_HCSR04_TRIG, OUTPUT);
  pinMode(PIN_HCSR04_ECHO, INPUT);
  pinMode(PIN_LDR_DO, INPUT);
  pinMode(PIN_RELAY1, OUTPUT);
  pinMode(PIN_RELAY2, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  forceRelaysOff();
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED, LOW);
}

void updateControl(const SensorSnapshot& sensor) {
  static ControlMemory memory;
  static State previousState = State::BOOT;
  static bool previousRelay1 = false;
  static bool previousRelay2 = false;

  SystemSnapshot sys;
  if (!copySystemSnapshot(sys)) {
    forceRelaysOff();
    markCriticalFault("Control state unavailable");
    logEvent("Control state unavailable");
    return;
  }

  const DiagnosticsSnapshot diag = getDiagnosticsSnapshot();
  bool sensorFault = false;
  String faultReason;
  if (diag.sensor.dhtConsecutiveFailures >= 5) { sensorFault = true; faultReason = "DHT11 failed 5 times"; }
  if (diag.sensor.ultrasonicConsecutiveFailures >= 5) {
    sensorFault = true;
    if (faultReason.length() > 0) faultReason += "; ";
    faultReason += "HC-SR04 timeout 5 times";
  }

  ControlDecision decision = computeControlDecision(sensor, sys, memory, millis());
  sys = decision.system;
  if (sensorFault) {
    sys.state = State::FAULT;
    sys.faultReason = faultReason;
    sys.relay1On = false;
    sys.relay2On = false;
  }

  applyRelays(sys.relay1On, sys.relay2On);
  if (sys.state != previousState) { logEvent("State changed to " + String(stateName(sys.state))); previousState = sys.state; }
  if (sys.relay1On != previousRelay1) { logEvent(String("Relay 1 ") + (sys.relay1On ? "ON" : "OFF")); previousRelay1 = sys.relay1On; }
  if (sys.relay2On != previousRelay2) { logEvent(String("Relay 2 ") + (sys.relay2On ? "ON" : "OFF")); previousRelay2 = sys.relay2On; }
  if (!updateSystemSnapshot(sys)) {
    forceRelaysOff();
    markCriticalFault("Control state update failed");
  }
}

bool setMode(Mode newMode) {
  SystemSnapshot sys;
  if (!copySystemSnapshot(sys)) return false;
  sys.mode = newMode;
  if (newMode == Mode::MANUAL) sys.state = State::MANUAL_OVERRIDE;
  sys.timestampMs = millis();
  if (!updateSystemSnapshot(sys)) return false;
  logEvent("Mode changed to " + String(modeName(newMode)));
  return true;
}

bool setManualRelay(uint8_t relayNumber, bool on) {
  SystemSnapshot sys;
  if (!copySystemSnapshot(sys)) return false;
  sys.mode = Mode::MANUAL;
  sys.state = State::MANUAL_OVERRIDE;
  if (relayNumber == 1) sys.relay1On = on;
  else if (relayNumber == 2) sys.relay2On = on;
  else return false;
  sys.timestampMs = millis();
  applyRelays(sys.relay1On, sys.relay2On);
  if (!updateSystemSnapshot(sys)) return false;
  logEvent("Manual Relay " + String(relayNumber) + " " + String(on ? "ON" : "OFF"));
  return true;
}
