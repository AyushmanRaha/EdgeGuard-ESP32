#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

#include "config.h"
#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.h.example"
#endif
#include "dashboard.h"

// -------------------- GLOBAL OBJECTS --------------------
DHT dht(PIN_DHT, DHT11);
WebServer server(80);
SemaphoreHandle_t gMutex;

// -------------------- ENUMS --------------------
enum class Mode : uint8_t {
  AUTO,
  MANUAL,
  AWAY
};

enum class State : uint8_t {
  BOOT,
  CALIBRATING,
  AUTO_MONITORING,
  MANUAL_OVERRIDE,
  ALERT,
  FAULT
};

// -------------------- DATA STRUCTURES --------------------
struct SensorSnapshot {
  float temperatureC = NAN;
  float humidity = NAN;

  bool dhtOk = false;

  uint16_t distanceCm = 0;
  bool distanceOk = false;

  bool ldrRawHigh = false;
  bool lightIsDark = false;

  uint32_t timestampMs = 0;
};

struct SystemSnapshot {
  Mode mode = Mode::AUTO;
  State state = State::BOOT;

  bool occupied = false;
  bool relay1On = false;
  bool relay2On = false;
  bool temperatureAlert = false;

  String faultReason = "";
  uint32_t timestampMs = 0;
};

SensorSnapshot gSensor;
SystemSnapshot gSystem;

// -------------------- EVENT LOG RING BUFFER --------------------
String gEvents[EVENT_LOG_SIZE];
uint8_t gEventHead = 0;
uint8_t gEventCount = 0;

// -------------------- UTILS --------------------
const char* modeName(Mode mode) {
  switch (mode) {
    case Mode::AUTO: return "AUTO";
    case Mode::MANUAL: return "MANUAL";
    case Mode::AWAY: return "AWAY";
    default: return "UNKNOWN";
  }
}

const char* stateName(State state) {
  switch (state) {
    case State::BOOT: return "BOOT";
    case State::CALIBRATING: return "CALIBRATING";
    case State::AUTO_MONITORING: return "AUTO_MONITORING";
    case State::MANUAL_OVERRIDE: return "MANUAL_OVERRIDE";
    case State::ALERT: return "ALERT";
    case State::FAULT: return "FAULT";
    default: return "UNKNOWN";
  }
}

String jsonEscape(String value) {
  value.replace("\\", "\\\\");
  value.replace("\"", "\\\"");
  value.replace("\n", "\\n");
  return value;
}

void logEvent(const String& message) {
  char prefix[18];
  snprintf(prefix, sizeof(prefix), "[%lus] ", static_cast<unsigned long>(millis() / 1000));
  String line;
  line.reserve(strlen(prefix) + message.length());
  line += prefix;
  line += message;

  if (xSemaphoreTake(gMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    gEvents[gEventHead] = line;
    gEventHead = (gEventHead + 1) % EVENT_LOG_SIZE;
    if (gEventCount < EVENT_LOG_SIZE) {
      gEventCount++;
    }
    xSemaphoreGive(gMutex);
  }

  Serial.println(line);
}

bool copySensorSnapshot(SensorSnapshot& out) {
  if (xSemaphoreTake(gMutex, pdMS_TO_TICKS(50)) != pdTRUE) return false;
  out = gSensor;
  xSemaphoreGive(gMutex);
  return true;
}

bool copySystemSnapshot(SystemSnapshot& out) {
  if (xSemaphoreTake(gMutex, pdMS_TO_TICKS(50)) != pdTRUE) return false;
  out = gSystem;
  xSemaphoreGive(gMutex);
  return true;
}

uint8_t copyEventLog(String (&out)[EVENT_LOG_SIZE]) {
  if (xSemaphoreTake(gMutex, pdMS_TO_TICKS(50)) != pdTRUE) return 0;
  const uint8_t count = gEventCount;
  const uint8_t start = (gEventHead + EVENT_LOG_SIZE - gEventCount) % EVENT_LOG_SIZE;
  for (uint8_t i = 0; i < count; i++) {
    out[i] = gEvents[(start + i) % EVENT_LOG_SIZE];
  }
  xSemaphoreGive(gMutex);
  return count;
}

// -------------------- RELAY DRIVER --------------------
void writeRelayPin(uint8_t pin, bool on) {
  if (RELAY_ACTIVE_LOW) {
    digitalWrite(pin, on ? LOW : HIGH);
  } else {
    digitalWrite(pin, on ? HIGH : LOW);
  }
}

void applyRelays(bool relay1On, bool relay2On) {
  writeRelayPin(PIN_RELAY1, relay1On);
  writeRelayPin(PIN_RELAY2, relay2On);
}

// -------------------- SENSOR DRIVER --------------------
SensorSnapshot readSensors() {
  SensorSnapshot s;
  s.timestampMs = millis();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (!isnan(humidity) && !isnan(temperature)) {
    s.humidity = humidity;
    s.temperatureC = temperature;
    s.dhtOk = true;
  } else {
    s.dhtOk = false;
  }

  bool ldrRaw = digitalRead(PIN_LDR_DO) == HIGH;
  s.ldrRawHigh = ldrRaw;
  s.lightIsDark = LDR_DARK_WHEN_HIGH ? ldrRaw : !ldrRaw;

  digitalWrite(PIN_HCSR04_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_HCSR04_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_HCSR04_TRIG, LOW);

  unsigned long duration = pulseIn(PIN_HCSR04_ECHO, HIGH, ULTRASONIC_TIMEOUT_US);

  if (duration > 0) {
    // HC-SR04 approximate formula: distance in cm = duration / 58
    s.distanceCm = static_cast<uint16_t>(duration / 58);
    s.distanceOk = true;
  } else {
    s.distanceOk = false;
  }

  return s;
}

// -------------------- CONTROL STATE MACHINE --------------------
void updateControl(const SensorSnapshot& sensor) {
  static uint8_t dhtFailCount = 0;
  static uint8_t ultrasonicFailCount = 0;

  static bool temperatureAlertLatched = false;
  static uint32_t lastOccupiedMs = 0;
  static bool hasEverSeenOccupancy = false;

  static State previousState = State::BOOT;
  static bool previousRelay1 = false;
  static bool previousRelay2 = false;

  SystemSnapshot sys;

  if (xSemaphoreTake(gMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    sys = gSystem;
    xSemaphoreGive(gMutex);
  }

  if (sensor.dhtOk) {
    dhtFailCount = 0;
  } else {
    dhtFailCount++;
  }

  if (sensor.distanceOk) {
    ultrasonicFailCount = 0;
  } else {
    ultrasonicFailCount++;
  }

  bool sensorFault = false;
  String faultReason = "";

  if (dhtFailCount >= 5) {
    sensorFault = true;
    faultReason = "DHT11 failed 5 times";
  }

  if (ultrasonicFailCount >= 5) {
    sensorFault = true;
    if (faultReason.length() > 0) {
      faultReason += "; ";
    }
    faultReason += "HC-SR04 timeout 5 times";
  }

  bool instantOccupied = sensor.distanceOk && sensor.distanceCm > 0 && sensor.distanceCm <= OCCUPIED_DISTANCE_CM;

  if (instantOccupied) {
    lastOccupiedMs = millis();
    hasEverSeenOccupancy = true;
  }

  bool occupiedHeld =
    instantOccupied ||
    (hasEverSeenOccupancy && ((millis() - lastOccupiedMs) < UNOCCUPIED_TIMEOUT_MS));

  if (sensor.dhtOk) {
    if (sensor.temperatureC >= TEMP_ALERT_ON_C) {
      temperatureAlertLatched = true;
    } else if (sensor.temperatureC <= TEMP_ALERT_OFF_C) {
      temperatureAlertLatched = false;
    }
  }

  sys.occupied = occupiedHeld;
  sys.temperatureAlert = temperatureAlertLatched;
  sys.faultReason = "";
  sys.timestampMs = millis();

  if (sensorFault) {
    sys.state = State::FAULT;
    sys.faultReason = faultReason;
    sys.relay1On = false;
    sys.relay2On = false;
  } else if (sys.mode == Mode::MANUAL) {
    sys.state = State::MANUAL_OVERRIDE;
    // Keep manually selected relay states.
  } else if (sys.mode == Mode::AWAY) {
    sys.relay1On = false;

    if (instantOccupied) {
      sys.state = State::ALERT;
      sys.relay2On = true;
    } else {
      sys.state = State::AUTO_MONITORING;
      sys.relay2On = false;
    }
  } else {
    // AUTO mode
    sys.state = temperatureAlertLatched ? State::ALERT : State::AUTO_MONITORING;
    sys.relay1On = sensor.lightIsDark && occupiedHeld;
    sys.relay2On = temperatureAlertLatched;
  }

  applyRelays(sys.relay1On, sys.relay2On);

  if (sys.state != previousState) {
    logEvent("State changed to " + String(stateName(sys.state)));
    previousState = sys.state;
  }

  if (sys.relay1On != previousRelay1) {
    logEvent(String("Relay 1 ") + (sys.relay1On ? "ON" : "OFF"));
    previousRelay1 = sys.relay1On;
  }

  if (sys.relay2On != previousRelay2) {
    logEvent(String("Relay 2 ") + (sys.relay2On ? "ON" : "OFF"));
    previousRelay2 = sys.relay2On;
  }

  if (xSemaphoreTake(gMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    gSystem = sys;
    xSemaphoreGive(gMutex);
  }
}

// -------------------- WEB DASHBOARD --------------------
void sendCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
}

void sendNoStoreHeaders() {
  server.sendHeader("Cache-Control", "no-store, max-age=0");
  server.sendHeader("Pragma", "no-cache");
}

void appendJsonString(String& json, const String& value) {
  json += '"';
  json += jsonEscape(value);
  json += '"';
}

String buildStatusJson() {
  SensorSnapshot sensor;
  SystemSnapshot sys;
  copySensorSnapshot(sensor);
  copySystemSnapshot(sys);

  String json;
  json.reserve(520);
  json += "{\"temperature_c\":";
  json += sensor.dhtOk ? String(sensor.temperatureC, 1) : "null";
  json += ",\"humidity\":";
  json += sensor.dhtOk ? String(sensor.humidity, 1) : "null";
  json += ",\"dht_ok\":";
  json += sensor.dhtOk ? "true" : "false";
  json += ",\"distance_cm\":";
  json += sensor.distanceOk ? String(sensor.distanceCm) : "null";
  json += ",\"distance_ok\":";
  json += sensor.distanceOk ? "true" : "false";
  json += ",\"light_is_dark\":";
  json += sensor.lightIsDark ? "true" : "false";
  json += ",\"occupied\":";
  json += sys.occupied ? "true" : "false";
  json += ",\"mode\":\"";
  json += modeName(sys.mode);
  json += "\",\"state\":\"";
  json += stateName(sys.state);
  json += "\",\"relay1\":";
  json += sys.relay1On ? "true" : "false";
  json += ",\"relay2\":";
  json += sys.relay2On ? "true" : "false";
  json += ",\"temperature_alert\":";
  json += sys.temperatureAlert ? "true" : "false";
  json += ",\"fault_reason\":";
  appendJsonString(json, sys.faultReason);
  json += ",\"uptime_s\":";
  json += String(millis() / 1000);
  json += ",\"wifi_mode\":\"";
  json += (WiFi.getMode() == WIFI_AP ? "AP" : "STA");
  json += "\",\"ip\":\"";
  json += (WiFi.getMode() == WIFI_AP ? WiFi.softAPIP().toString() : WiFi.localIP().toString());
  json += "\",\"heap_free\":";
  json += String(ESP.getFreeHeap());
  if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED) {
    json += ",\"rssi\":";
    json += String(WiFi.RSSI());
  }
  json += "}";
  return json;
}

String buildLogsJson() {
  String events[EVENT_LOG_SIZE];
  const uint8_t count = copyEventLog(events);
  String json;
  json.reserve(384);
  json += "[";
  for (uint8_t i = 0; i < count; i++) {
    appendJsonString(json, events[i]);
    if (i < count - 1) json += ",";
  }
  json += "]";
  return json;
}

void sendJson(int status, const String& payload) {
  sendCorsHeaders();
  sendNoStoreHeaders();
  server.send(status, "application/json", payload);
}

void sendOk() {
  sendJson(200, "{\"ok\":true}");
}

void setMode(Mode newMode) {
  if (xSemaphoreTake(gMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    gSystem.mode = newMode;
    if (newMode == Mode::MANUAL) gSystem.state = State::MANUAL_OVERRIDE;
    xSemaphoreGive(gMutex);
  }
  logEvent("Mode changed to " + String(modeName(newMode)));
}

void setManualRelay(uint8_t relayNumber, bool on) {
  SystemSnapshot sys;
  if (xSemaphoreTake(gMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    gSystem.mode = Mode::MANUAL;
    gSystem.state = State::MANUAL_OVERRIDE;
    if (relayNumber == 1) gSystem.relay1On = on;
    else if (relayNumber == 2) gSystem.relay2On = on;
    sys = gSystem;
    xSemaphoreGive(gMutex);
  }
  applyRelays(sys.relay1On, sys.relay2On);
  logEvent("Manual Relay " + String(relayNumber) + " " + String(on ? "ON" : "OFF"));
}

void handleRoot() {
  server.sendHeader("Cache-Control", "no-store, max-age=0");
  server.send_P(200, "text/html", DASHBOARD_HTML);
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, []() { sendJson(200, buildStatusJson()); });
  server.on("/api/logs", HTTP_GET, []() { sendJson(200, buildLogsJson()); });
  server.on("/api/mode/auto", HTTP_POST, []() { setMode(Mode::AUTO); sendOk(); });
  server.on("/api/mode/manual", HTTP_POST, []() { setMode(Mode::MANUAL); sendOk(); });
  server.on("/api/mode/away", HTTP_POST, []() { setMode(Mode::AWAY); sendOk(); });
  server.on("/api/relay1/on", HTTP_POST, []() { setManualRelay(1, true); sendOk(); });
  server.on("/api/relay1/off", HTTP_POST, []() { setManualRelay(1, false); sendOk(); });
  server.on("/api/relay2/on", HTTP_POST, []() { setManualRelay(2, true); sendOk(); });
  server.on("/api/relay2/off", HTTP_POST, []() { setManualRelay(2, false); sendOk(); });
  server.begin();
}

// -------------------- WIFI --------------------
void connectWiFiOrStartAP() {
  String ssid = WIFI_SSID;

  if (ssid.length() > 0 && ssid != "YOUR_WIFI_NAME") {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
      delay(500);
      Serial.print(".");
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Wi-Fi connected.");
      Serial.print("Dashboard: http://");
      Serial.println(WiFi.localIP());
      logEvent("Wi-Fi connected: " + WiFi.localIP().toString());
      return;
    }
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP("EdgeGuard-ESP32", "edgeguard123");

  Serial.println("Wi-Fi connection failed or not configured.");
  Serial.println("Started fallback hotspot:");
  Serial.println("SSID: EdgeGuard-ESP32");
  Serial.println("Password: edgeguard123");
  Serial.print("Dashboard: http://");
  Serial.println(WiFi.softAPIP());

  logEvent("Started AP mode: EdgeGuard-ESP32");
}

// -------------------- FREERTOS TASKS --------------------
void SensorTask(void* parameter) {
  for (;;) {
    SensorSnapshot reading = readSensors();

    if (xSemaphoreTake(gMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      gSensor = reading;
      xSemaphoreGive(gMutex);
    }

    Serial.print("T=");
    if (reading.dhtOk) {
      Serial.print(reading.temperatureC, 1);
      Serial.print("C H=");
      Serial.print(reading.humidity, 1);
      Serial.print("%");
    } else {
      Serial.print("DHT_ERR");
    }

    Serial.print(" DIST=");
    if (reading.distanceOk) {
      Serial.print(reading.distanceCm);
      Serial.print("cm");
    } else {
      Serial.print("NO_ECHO");
    }

    Serial.print(" LIGHT=");
    Serial.println(reading.lightIsDark ? "DARK" : "BRIGHT");

    vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS));
  }
}

void ControlTask(void* parameter) {
  for (;;) {
    SensorSnapshot sensor;

    copySensorSnapshot(sensor);

    updateControl(sensor);

    vTaskDelay(pdMS_TO_TICKS(CONTROL_TASK_PERIOD_MS));
  }
}

void WebTask(void* parameter) {
  for (;;) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void HeartbeatTask(void* parameter) {
  bool ledState = false;

  for (;;) {
    SystemSnapshot sys;

    copySystemSnapshot(sys);

    ledState = !ledState;

    if (sys.state == State::FAULT) {
      digitalWrite(PIN_LED_GREEN, LOW);
      digitalWrite(PIN_LED_RED, ledState ? HIGH : LOW);
      vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_FAULT_MS));
    } else if (sys.state == State::ALERT) {
      digitalWrite(PIN_LED_GREEN, HIGH);
      digitalWrite(PIN_LED_RED, ledState ? HIGH : LOW);
      vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_ALERT_MS));
    } else {
      digitalWrite(PIN_LED_RED, LOW);
      digitalWrite(PIN_LED_GREEN, ledState ? HIGH : LOW);
      vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_NORMAL_MS));
    }
  }
}

// -------------------- SETUP / LOOP --------------------
void setupPins() {
  pinMode(PIN_DHT, INPUT);

  pinMode(PIN_HCSR04_TRIG, OUTPUT);
  pinMode(PIN_HCSR04_ECHO, INPUT);

  pinMode(PIN_LDR_DO, INPUT);

  pinMode(PIN_RELAY1, OUTPUT);
  pinMode(PIN_RELAY2, OUTPUT);

  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);

  applyRelays(false, false);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED, LOW);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  gMutex = xSemaphoreCreateMutex();

  setupPins();
  dht.begin();

  logEvent("BOOT started");

  if (xSemaphoreTake(gMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    gSystem.mode = Mode::AUTO;
    gSystem.state = State::CALIBRATING;
    gSystem.relay1On = false;
    gSystem.relay2On = false;
    xSemaphoreGive(gMutex);
  }

  connectWiFiOrStartAP();
  setupWebServer();

  logEvent("Web server started");

  xTaskCreatePinnedToCore(SensorTask, "SensorTask", 4096, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(ControlTask, "ControlTask", 4096, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(WebTask, "WebTask", 8192, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(HeartbeatTask, "HeartbeatTask", 2048, nullptr, 1, nullptr, 1);

  logEvent("Tasks created");
}

void loop() {
  // Empty because FreeRTOS tasks do the work.
  delay(1000);
}