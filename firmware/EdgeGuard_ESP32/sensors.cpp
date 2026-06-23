#include <Arduino.h>
#include <DHT.h>
#include "config.h"
#include "sensors.h"
#include "diagnostics.h"

namespace { DHT dht(PIN_DHT, DHT11); }

void initSensors() { dht.begin(); }

SensorSnapshot readSensors() {
  SensorSnapshot s;
  s.timestampMs = millis();
  const float humidity = dht.readHumidity();
  const float temperature = dht.readTemperature();
  if (!isnan(humidity) && !isnan(temperature)) {
    s.humidity = humidity; s.temperatureC = temperature; s.dhtOk = true;
  }
  noteDhtResult(s.dhtOk, s.timestampMs);
  const bool ldrRaw = digitalRead(PIN_LDR_DO) == HIGH;
  s.ldrRawHigh = ldrRaw;
  s.lightIsDark = LDR_DARK_WHEN_HIGH ? ldrRaw : !ldrRaw;
  digitalWrite(PIN_HCSR04_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_HCSR04_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_HCSR04_TRIG, LOW);
  const unsigned long duration = pulseIn(PIN_HCSR04_ECHO, HIGH, ULTRASONIC_TIMEOUT_US);
  if (duration > 0) {
    const uint16_t distanceCm = static_cast<uint16_t>(duration / 58);
    if (distanceCm >= ULTRASONIC_MIN_VALID_CM && distanceCm <= ULTRASONIC_MAX_VALID_CM) {
      s.distanceCm = distanceCm;
      s.distanceOk = true;
    }
  }
  noteDistanceResult(s.distanceOk, s.timestampMs);
  return s;
}
