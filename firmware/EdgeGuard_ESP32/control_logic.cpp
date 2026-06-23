#include "control_logic.h"
#include "config.h"

ControlDecision computeControlDecision(const SensorSnapshot& sensor, const SystemSnapshot& previousSystem, ControlMemory& memory, uint32_t nowMs) {
  ControlDecision decision;
  SystemSnapshot sys = previousSystem;
  decision.sensorStale = (sensor.timestampMs == 0) || ((nowMs - sensor.timestampMs) > SENSOR_STALE_TIMEOUT_MS);
  decision.instantOccupied = sensor.distanceOk && sensor.distanceCm > 0 && sensor.distanceCm <= OCCUPIED_DISTANCE_CM;

  if (decision.instantOccupied) { memory.lastOccupiedMs = nowMs; memory.hasEverSeenOccupancy = true; }
  const bool occupiedHeld = decision.instantOccupied || (memory.hasEverSeenOccupancy && ((nowMs - memory.lastOccupiedMs) < UNOCCUPIED_TIMEOUT_MS));

  if (sensor.dhtOk) {
    if (sensor.temperatureC >= TEMP_ALERT_ON_C) memory.temperatureAlertLatched = true;
    else if (sensor.temperatureC <= TEMP_ALERT_OFF_C) memory.temperatureAlertLatched = false;
  }

  sys.occupied = occupiedHeld;
  sys.temperatureAlert = memory.temperatureAlertLatched;
  sys.faultReason = "";
  sys.timestampMs = nowMs;

  if (decision.sensorStale) {
    sys.state = State::FAULT;
    sys.faultReason = "Sensor snapshot stale";
    sys.relay1On = false;
    sys.relay2On = false;
  } else if (sys.mode == Mode::MANUAL) {
    sys.state = State::MANUAL_OVERRIDE;
  } else if (sys.mode == Mode::AWAY) {
    sys.relay1On = false;
    sys.relay2On = decision.instantOccupied;
    sys.state = decision.instantOccupied ? State::ALERT : State::AUTO_MONITORING;
  } else {
    sys.state = memory.temperatureAlertLatched ? State::ALERT : State::AUTO_MONITORING;
    sys.relay1On = sensor.lightIsDark && occupiedHeld;
    sys.relay2On = memory.temperatureAlertLatched;
  }

  decision.system = sys;
  return decision;
}
