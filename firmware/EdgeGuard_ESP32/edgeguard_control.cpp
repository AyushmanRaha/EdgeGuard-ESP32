#include "edgeguard_control.h"

const char* modeName(Mode mode) {
  switch (mode) {
    case Mode::AUTO:
      return "AUTO";
    case Mode::MANUAL:
      return "MANUAL";
    case Mode::AWAY:
      return "AWAY";
  }
  return "UNKNOWN";
}

const char* stateName(State state) {
  switch (state) {
    case State::BOOT:
      return "BOOT";
    case State::CALIBRATING:
      return "CALIBRATING";
    case State::AUTO_MONITORING:
      return "AUTO_MONITORING";
    case State::MANUAL_OVERRIDE:
      return "MANUAL_OVERRIDE";
    case State::ALERT:
      return "ALERT";
    case State::FAULT:
      return "FAULT";
  }
  return "UNKNOWN";
}

const char* faultReason(FaultCode code) {
  switch (code) {
    case FaultCode::NONE:
      return "";
    case FaultCode::DHT:
      return "DHT11 failed 5 times";
    case FaultCode::ULTRASONIC:
      return "HC-SR04 timeout 5 times";
    case FaultCode::DHT_AND_ULTRASONIC:
      return "DHT11 failed 5 times; HC-SR04 timeout 5 times";
    case FaultCode::TASK_WATCHDOG:
      return "TASK_WATCHDOG: critical task heartbeat timeout";
  }
  return "Unknown fault";
}

ControlConfig defaultControlConfig() {
  return ControlConfig{};
}

ControlResult updateControlLogic(const SensorSnapshot& sensor, const SystemSnapshot& previousSystem,
                                 ControlContext& ctx, const ControlConfig& cfg, uint32_t nowMs) {
  ControlResult result;
  SystemSnapshot sys = previousSystem;
  sys.timestampMs = nowMs;
  sys.faultCode = FaultCode::NONE;

  ctx.dhtFailCount = sensor.dhtOk ? 0 : static_cast<uint8_t>(ctx.dhtFailCount + 1);
  ctx.ultrasonicFailCount =
      sensor.distanceOk ? 0 : static_cast<uint8_t>(ctx.ultrasonicFailCount + 1);

  const bool dhtFault = ctx.dhtFailCount >= cfg.maxConsecutiveSensorFailures;
  const bool ultrasonicFault = ctx.ultrasonicFailCount >= cfg.maxConsecutiveSensorFailures;
  if (dhtFault && ultrasonicFault) {
    sys.faultCode = FaultCode::DHT_AND_ULTRASONIC;
  } else if (dhtFault) {
    sys.faultCode = FaultCode::DHT;
  } else if (ultrasonicFault) {
    sys.faultCode = FaultCode::ULTRASONIC;
  }

  result.instantOccupied =
      sensor.distanceOk && sensor.distanceCm > 0 && sensor.distanceCm <= cfg.occupiedDistanceCm;
  if (result.instantOccupied) {
    ctx.lastOccupiedMs = nowMs;
    ctx.hasSeenOccupancy = true;
  }
  const bool occupiedHeld =
      result.instantOccupied ||
      (ctx.hasSeenOccupancy && (nowMs - ctx.lastOccupiedMs) < cfg.unoccupiedTimeoutMs);

  if (sensor.dhtOk) {
    if (sensor.temperatureC >= cfg.tempAlertOnC) {
      ctx.temperatureAlertLatched = true;
    } else if (sensor.temperatureC <= cfg.tempAlertOffC) {
      ctx.temperatureAlertLatched = false;
    }
  }

  sys.occupied = occupiedHeld;
  sys.temperatureAlert = ctx.temperatureAlertLatched;
  result.sensorFault = sys.faultCode != FaultCode::NONE;

  if (result.sensorFault) {
    sys.state = State::FAULT;
    sys.relay1On = false;
    sys.relay2On = false;
  } else if (sys.mode == Mode::MANUAL) {
    sys.state = State::MANUAL_OVERRIDE;
  } else if (sys.mode == Mode::AWAY) {
    sys.relay1On = false;
    if (result.instantOccupied) {
      sys.state = State::ALERT;
      sys.relay2On = true;
    } else {
      sys.state = State::AUTO_MONITORING;
      sys.relay2On = false;
    }
  } else {
    sys.state = ctx.temperatureAlertLatched ? State::ALERT : State::AUTO_MONITORING;
    sys.relay1On = sensor.lightIsDark && occupiedHeld;
    sys.relay2On = ctx.temperatureAlertLatched;
  }

  result.system = sys;
  return result;
}
