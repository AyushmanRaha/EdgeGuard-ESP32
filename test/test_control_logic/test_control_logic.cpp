#include <unity.h>
#include "control_logic.h"
#include "config.h"

namespace {
constexpr uint32_t kNow = 10000;

SensorSnapshot makeSensor(uint32_t timestampMs = kNow) {
  SensorSnapshot sensor;
  sensor.temperatureC = 24.0f;
  sensor.humidity = 50.0f;
  sensor.dhtOk = true;
  sensor.distanceCm = 100;
  sensor.distanceOk = true;
  sensor.ldrRawHigh = false;
  sensor.lightIsDark = false;
  sensor.timestampMs = timestampMs;
  return sensor;
}

SystemSnapshot makeSystem(Mode mode = Mode::AUTO) {
  SystemSnapshot system;
  system.mode = mode;
  system.state = State::AUTO_MONITORING;
  system.occupied = false;
  system.relay1On = false;
  system.relay2On = false;
  system.temperatureAlert = false;
  system.faultReason = "";
  system.timestampMs = kNow - 100;
  return system;
}

ControlMemory makeMemory() { return ControlMemory{}; }

ControlDecision decide(const SensorSnapshot& sensor, const SystemSnapshot& system, ControlMemory& memory, uint32_t nowMs = kNow) {
  return computeControlDecision(sensor, system, memory, nowMs);
}
}

void test_auto_dark_and_occupied_turns_relay1_on() {
  auto sensor = makeSensor();
  sensor.lightIsDark = true;
  sensor.distanceCm = OCCUPIED_DISTANCE_CM;
  auto system = makeSystem(Mode::AUTO);
  auto memory = makeMemory();

  auto decision = decide(sensor, system, memory);

  TEST_ASSERT_TRUE(decision.instantOccupied);
  TEST_ASSERT_TRUE(decision.system.occupied);
  TEST_ASSERT_TRUE(decision.system.relay1On);
  TEST_ASSERT_EQUAL(static_cast<int>(State::AUTO_MONITORING), static_cast<int>(decision.system.state));
}

void test_auto_bright_and_occupied_keeps_relay1_off() {
  auto sensor = makeSensor();
  sensor.lightIsDark = false;
  sensor.distanceCm = OCCUPIED_DISTANCE_CM;
  auto system = makeSystem(Mode::AUTO);
  auto memory = makeMemory();

  auto decision = decide(sensor, system, memory);

  TEST_ASSERT_TRUE(decision.system.occupied);
  TEST_ASSERT_FALSE(decision.system.relay1On);
}

void test_auto_dark_without_valid_occupancy_keeps_relay1_off() {
  auto sensor = makeSensor();
  sensor.lightIsDark = true;
  sensor.distanceOk = false;
  sensor.distanceCm = OCCUPIED_DISTANCE_CM;
  auto system = makeSystem(Mode::AUTO);
  auto memory = makeMemory();

  auto decision = decide(sensor, system, memory);

  TEST_ASSERT_FALSE(decision.instantOccupied);
  TEST_ASSERT_FALSE(decision.system.occupied);
  TEST_ASSERT_FALSE(decision.system.relay1On);
}

void test_auto_relay2_follows_temperature_alert_latch() {
  auto sensor = makeSensor();
  sensor.temperatureC = TEMP_ALERT_ON_C;
  auto system = makeSystem(Mode::AUTO);
  auto memory = makeMemory();

  auto decision = decide(sensor, system, memory);

  TEST_ASSERT_TRUE(decision.system.temperatureAlert);
  TEST_ASSERT_TRUE(decision.system.relay2On);
  TEST_ASSERT_EQUAL(static_cast<int>(State::ALERT), static_cast<int>(decision.system.state));
}

void test_occupancy_hold_masks_short_ultrasonic_dropout() {
  auto sensor = makeSensor();
  sensor.distanceCm = OCCUPIED_DISTANCE_CM;
  auto system = makeSystem(Mode::AUTO);
  auto memory = makeMemory();
  auto first = decide(sensor, system, memory, kNow);
  TEST_ASSERT_TRUE(first.system.occupied);

  sensor.timestampMs = kNow + 1000;
  sensor.distanceOk = false;
  auto held = decide(sensor, system, memory, kNow + UNOCCUPIED_TIMEOUT_MS - 1);

  TEST_ASSERT_FALSE(held.instantOccupied);
  TEST_ASSERT_TRUE(held.system.occupied);
}

void test_occupancy_hold_expires_after_timeout() {
  auto sensor = makeSensor();
  sensor.distanceCm = OCCUPIED_DISTANCE_CM;
  auto system = makeSystem(Mode::AUTO);
  auto memory = makeMemory();
  decide(sensor, system, memory, kNow);

  sensor.timestampMs = kNow + UNOCCUPIED_TIMEOUT_MS;
  sensor.distanceOk = false;
  auto expired = decide(sensor, system, memory, kNow + UNOCCUPIED_TIMEOUT_MS);

  TEST_ASSERT_FALSE(expired.system.occupied);
}

void test_temperature_hysteresis_latches_holds_and_clears() {
  auto sensor = makeSensor();
  auto system = makeSystem(Mode::AUTO);
  auto memory = makeMemory();

  sensor.temperatureC = TEMP_ALERT_ON_C;
  auto latched = decide(sensor, system, memory, kNow);
  TEST_ASSERT_TRUE(latched.system.temperatureAlert);

  sensor.timestampMs = kNow + 1;
  sensor.temperatureC = (TEMP_ALERT_ON_C + TEMP_ALERT_OFF_C) / 2.0f;
  auto held = decide(sensor, system, memory, kNow + 1);
  TEST_ASSERT_TRUE(held.system.temperatureAlert);

  sensor.timestampMs = kNow + 2;
  sensor.temperatureC = TEMP_ALERT_OFF_C;
  auto cleared = decide(sensor, system, memory, kNow + 2);
  TEST_ASSERT_FALSE(cleared.system.temperatureAlert);
  TEST_ASSERT_FALSE(cleared.system.relay2On);
}

void test_away_instant_occupancy_enters_alert_and_turns_relay2_on() {
  auto sensor = makeSensor();
  sensor.lightIsDark = true;
  sensor.distanceCm = OCCUPIED_DISTANCE_CM;
  auto system = makeSystem(Mode::AWAY);
  auto memory = makeMemory();

  auto decision = decide(sensor, system, memory);

  TEST_ASSERT_FALSE(decision.system.relay1On);
  TEST_ASSERT_TRUE(decision.system.relay2On);
  TEST_ASSERT_EQUAL(static_cast<int>(State::ALERT), static_cast<int>(decision.system.state));
}

void test_manual_mode_preserves_existing_relay_states() {
  auto sensor = makeSensor();
  sensor.lightIsDark = true;
  sensor.distanceCm = OCCUPIED_DISTANCE_CM;
  auto system = makeSystem(Mode::MANUAL);
  system.relay1On = true;
  system.relay2On = false;
  auto memory = makeMemory();

  auto decision = decide(sensor, system, memory);

  TEST_ASSERT_EQUAL(static_cast<int>(State::MANUAL_OVERRIDE), static_cast<int>(decision.system.state));
  TEST_ASSERT_TRUE(decision.system.relay1On);
  TEST_ASSERT_FALSE(decision.system.relay2On);
}

void test_stale_sensor_enters_fault_and_forces_relays_off() {
  auto sensor = makeSensor(kNow - SENSOR_STALE_TIMEOUT_MS - 1);
  auto system = makeSystem(Mode::AUTO);
  system.relay1On = true;
  system.relay2On = true;
  auto memory = makeMemory();

  auto decision = decide(sensor, system, memory);

  TEST_ASSERT_TRUE(decision.sensorStale);
  TEST_ASSERT_EQUAL(static_cast<int>(State::FAULT), static_cast<int>(decision.system.state));
  TEST_ASSERT_FALSE(decision.system.relay1On);
  TEST_ASSERT_FALSE(decision.system.relay2On);
  TEST_ASSERT_EQUAL_STRING("Sensor snapshot stale", decision.system.faultReason.c_str());
}

void test_missing_sensor_timestamp_is_stale_fault() {
  auto sensor = makeSensor(0);
  auto system = makeSystem(Mode::AUTO);
  auto memory = makeMemory();

  auto decision = decide(sensor, system, memory);

  TEST_ASSERT_TRUE(decision.sensorStale);
  TEST_ASSERT_EQUAL(static_cast<int>(State::FAULT), static_cast<int>(decision.system.state));
}

void test_invalid_ultrasonic_reading_does_not_count_as_instant_occupancy() {
  auto sensor = makeSensor();
  sensor.distanceOk = false;
  sensor.distanceCm = OCCUPIED_DISTANCE_CM;
  auto system = makeSystem(Mode::AWAY);
  auto memory = makeMemory();

  auto decision = decide(sensor, system, memory);

  TEST_ASSERT_FALSE(decision.instantOccupied);
  TEST_ASSERT_FALSE(decision.system.relay2On);
}

void test_distance_boundary_exact_threshold_counts_and_above_does_not() {
  auto sensor = makeSensor();
  auto system = makeSystem(Mode::AUTO);
  auto memory = makeMemory();

  sensor.distanceCm = OCCUPIED_DISTANCE_CM;
  auto exact = decide(sensor, system, memory, kNow);
  TEST_ASSERT_TRUE(exact.instantOccupied);

  auto separateMemory = makeMemory();
  sensor.timestampMs = kNow + 1;
  sensor.distanceCm = OCCUPIED_DISTANCE_CM + 1;
  auto above = decide(sensor, system, separateMemory, kNow + 1);
  TEST_ASSERT_FALSE(above.instantOccupied);
  TEST_ASSERT_FALSE(above.system.occupied);
}

void test_sensor_stale_boundary_uses_strict_greater_than_timeout() {
  auto system = makeSystem(Mode::AUTO);
  auto memory = makeMemory();

  auto exactAge = makeSensor(kNow - SENSOR_STALE_TIMEOUT_MS);
  auto exact = decide(exactAge, system, memory, kNow);
  TEST_ASSERT_FALSE(exact.sensorStale);

  auto justAboveAge = makeSensor(kNow - SENSOR_STALE_TIMEOUT_MS - 1);
  auto stale = decide(justAboveAge, system, memory, kNow);
  TEST_ASSERT_TRUE(stale.sensorStale);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_auto_dark_and_occupied_turns_relay1_on);
  RUN_TEST(test_auto_bright_and_occupied_keeps_relay1_off);
  RUN_TEST(test_auto_dark_without_valid_occupancy_keeps_relay1_off);
  RUN_TEST(test_auto_relay2_follows_temperature_alert_latch);
  RUN_TEST(test_occupancy_hold_masks_short_ultrasonic_dropout);
  RUN_TEST(test_occupancy_hold_expires_after_timeout);
  RUN_TEST(test_temperature_hysteresis_latches_holds_and_clears);
  RUN_TEST(test_away_instant_occupancy_enters_alert_and_turns_relay2_on);
  RUN_TEST(test_manual_mode_preserves_existing_relay_states);
  RUN_TEST(test_stale_sensor_enters_fault_and_forces_relays_off);
  RUN_TEST(test_missing_sensor_timestamp_is_stale_fault);
  RUN_TEST(test_invalid_ultrasonic_reading_does_not_count_as_instant_occupancy);
  RUN_TEST(test_distance_boundary_exact_threshold_counts_and_above_does_not);
  RUN_TEST(test_sensor_stale_boundary_uses_strict_greater_than_timeout);
  return UNITY_END();
}
