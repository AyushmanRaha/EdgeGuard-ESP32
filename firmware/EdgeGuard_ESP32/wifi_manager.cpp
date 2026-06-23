#include <Arduino.h>
#include <WiFi.h>
#include "app_state.h"
#include "diagnostics.h"
#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.h.example"
#endif

void connectWiFiOrStartAP() {
  const String ssid = WIFI_SSID;
  if (ssid.length() > 0 && ssid != "YOUR_WIFI_NAME") {
    setWiFiDiagnostics("STA", "connecting");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) { delay(500); Serial.print("."); }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Wi-Fi connected.");
      Serial.print("Dashboard: http://"); Serial.println(WiFi.localIP());
      setWiFiDiagnostics("STA", "connected");
      logEvent("Wi-Fi connected: " + WiFi.localIP().toString());
      return;
    }
  }
  setWiFiDiagnostics("AP", "fallback_ap");
  WiFi.mode(WIFI_AP);
  WiFi.softAP("EdgeGuard-ESP32", "edgeguard123");
  Serial.println("Wi-Fi connection failed or not configured.");
  Serial.println("Started fallback hotspot:");
  Serial.println("SSID: EdgeGuard-ESP32");
  Serial.println("Password: edgeguard123");
  Serial.print("Dashboard: http://"); Serial.println(WiFi.softAPIP());
  logEvent("Started AP mode: EdgeGuard-ESP32");
}
