#pragma once
#include <Arduino.h>

// -------------------- GPIO PIN MAP --------------------
constexpr uint8_t PIN_DHT = 4;

constexpr uint8_t PIN_HCSR04_TRIG = 5;
constexpr uint8_t PIN_HCSR04_ECHO = 18;

constexpr uint8_t PIN_LDR_DO = 34;

constexpr uint8_t PIN_RELAY1 = 26;
constexpr uint8_t PIN_RELAY2 = 27;

constexpr uint8_t PIN_LED_GREEN = 23;
constexpr uint8_t PIN_LED_RED = 22;

// -------------------- MODULE BEHAVIOR --------------------
// Most low-cost relay modules are active LOW.
// If your relay behaves opposite, change this to false.
constexpr bool RELAY_ACTIVE_LOW = true;

// LDR digital modules vary.
// If dashboard shows DARK when the room is bright, flip this value.
constexpr bool LDR_DARK_WHEN_HIGH = true;

// -------------------- SENSOR THRESHOLDS --------------------
constexpr uint16_t OCCUPIED_DISTANCE_CM = 40;
constexpr uint32_t UNOCCUPIED_TIMEOUT_MS = 5000;

constexpr float TEMP_ALERT_ON_C = 35.0;
constexpr float TEMP_ALERT_OFF_C = 33.0;

// HC-SR04 pulse timeout.
// 30000 us is roughly enough for around 5 meters.
constexpr uint32_t ULTRASONIC_TIMEOUT_US = 30000;

// -------------------- TASK PERIODS --------------------
constexpr uint32_t SENSOR_TASK_PERIOD_MS = 2000;
constexpr uint32_t CONTROL_TASK_PERIOD_MS = 500;
constexpr uint32_t HEARTBEAT_NORMAL_MS = 1000;
constexpr uint32_t HEARTBEAT_ALERT_MS = 300;
constexpr uint32_t HEARTBEAT_FAULT_MS = 150;

// -------------------- LOGGING --------------------
constexpr uint8_t EVENT_LOG_SIZE = 20;