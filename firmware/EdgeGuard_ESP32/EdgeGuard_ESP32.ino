#include <Arduino.h>
#include <DHT.h>
#include <WebServer.h>
#include <WiFi.h>
#include <stdarg.h>
#include <stdio.h>

#include "config.h"
#include "edgeguard_control.h"
#include "edgeguard_diagnostics.h"
#include "edgeguard_event_log.h"
#include "edgeguard_sensor_history.h"
#include "secrets.h"

DHT dht(PIN_DHT, DHT11);
WebServer server(80);
SemaphoreHandle_t gMutex;

ControlConfig gControlConfig;
ControlContext gControlContext;
SensorSnapshot gSensor;
SystemSnapshot gSystem;
EdgeGuardEventLog gEventLog;
EdgeGuardSensorHistory gHistory;
bool gWatchdogFault = false;

static bool appendJson(char* out, size_t outSize, size_t* used, const char* fmt, ...) {
  if (*used >= outSize) return false;
  va_list args; va_start(args, fmt);
  const int n = vsnprintf(out + *used, outSize - *used, fmt, args);
  va_end(args);
  if (n < 0) return false;
  if ((size_t)n >= outSize - *used) { *used = outSize - 1; out[*used] = '\0'; return false; }
  *used += (size_t)n; return true;
}

static void appendEscaped(char* out, size_t outSize, size_t* used, const char* s) {
  for (; s && *s && *used + 2 < outSize; ++s) {
    if (*s == '"' || *s == '\\') { out[(*used)++] = '\\'; out[(*used)++] = *s; }
    else if (*s == '\n') { out[(*used)++] = '\\'; out[(*used)++] = 'n'; }
    else { out[(*used)++] = *s; }
  }
  out[*used] = '\0';
}

void logEvent(EventLevel level, const char* message) {
  if (xSemaphoreTake(gMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    gEventLog.push(millis(), level, message);
    xSemaphoreGive(gMutex);
  }
  Serial.print('['); Serial.print(eventLevelName(level)); Serial.print("] "); Serial.println(message ? message : "");
}

void logEvent(EventLevel level, const String& message) { logEvent(level, message.c_str()); }

void writeRelayPin(uint8_t pin, bool on) { digitalWrite(pin, RELAY_ACTIVE_LOW ? (on ? LOW : HIGH) : (on ? HIGH : LOW)); }
void applyRelays(bool relay1On, bool relay2On) { writeRelayPin(PIN_RELAY1, relay1On); writeRelayPin(PIN_RELAY2, relay2On); }

SensorSnapshot readSensors() {
  SensorSnapshot s; s.timestampMs = millis();
  const float humidity = dht.readHumidity(); const float temperature = dht.readTemperature();
  if (!isnan(humidity) && !isnan(temperature)) { s.humidity = humidity; s.temperatureC = temperature; s.dhtOk = true; }
  const bool ldrRaw = digitalRead(PIN_LDR_DO) == HIGH; s.ldrRawHigh = ldrRaw; s.lightIsDark = LDR_DARK_WHEN_HIGH ? ldrRaw : !ldrRaw;
  digitalWrite(PIN_HCSR04_TRIG, LOW); delayMicroseconds(2); digitalWrite(PIN_HCSR04_TRIG, HIGH); delayMicroseconds(10); digitalWrite(PIN_HCSR04_TRIG, LOW);
  const unsigned long duration = pulseIn(PIN_HCSR04_ECHO, HIGH, ULTRASONIC_TIMEOUT_US);
  if (duration > 0) { s.distanceCm = static_cast<uint16_t>(duration / 58); s.distanceOk = true; }
  return s;
}

bool authorized() {
  const String configured = EDGEGUARD_API_TOKEN;
  if (configured.length() == 0) return true;
  const String header = server.header("X-EdgeGuard-Token");
  const String query = server.arg("token");
  const bool ok = (header == configured) || (query == configured);
  if (!ok) logEvent(EventLevel::SECURITY, "Unauthorized control request");
  return ok;
}

void sendUnauthorized() { server.send(401, "application/json", "{\"ok\":false,\"error\":\"unauthorized\"}"); }
void sendOk() { server.send(200, "application/json", "{\"ok\":true}"); }

template <typename F> void protectedPost(F fn) { if (!authorized()) { sendUnauthorized(); return; } fn(); sendOk(); }

void setMode(Mode newMode) {
  if (xSemaphoreTake(gMutex, pdMS_TO_TICKS(50)) == pdTRUE) { gSystem.mode = newMode; if (newMode == Mode::MANUAL) gSystem.state = State::MANUAL_OVERRIDE; xSemaphoreGive(gMutex); }
  logEvent(EventLevel::INFO, String("Mode changed to ") + modeName(newMode));
}

void setManualRelay(uint8_t relayNumber, bool on) {
  SystemSnapshot sys;
  if (xSemaphoreTake(gMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    gSystem.mode = Mode::MANUAL; gSystem.state = State::MANUAL_OVERRIDE;
    if (relayNumber == 1) gSystem.relay1On = on; else if (relayNumber == 2) gSystem.relay2On = on;
    sys = gSystem; xSemaphoreGive(gMutex);
  }
  if (sys.state != State::FAULT) applyRelays(sys.relay1On, sys.relay2On);
  logEvent(EventLevel::INFO, String("Manual Relay ") + relayNumber + (on ? " ON" : " OFF"));
}

void resetFault() {
  const bool wasFault = gSystem.state == State::FAULT;
  if (xSemaphoreTake(gMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    gControlContext.dhtFailCount = 0; gControlContext.ultrasonicFailCount = 0; gControlContext.temperatureAlertLatched = false;
    gWatchdogFault = false; diagnosticsSetWatchdogFault(false);
    gSystem.faultCode = FaultCode::NONE; gSystem.temperatureAlert = false; gSystem.relay1On = false; gSystem.relay2On = false;
    gSystem.state = State::CALIBRATING;
    xSemaphoreGive(gMutex);
  }
  applyRelays(false, false);
  logEvent(EventLevel::INFO, wasFault ? "Fault reset requested" : "Fault reset requested while no active fault");
}

String buildStatusJson() {
  static char out[1024]; size_t used = 0; SensorSnapshot s; SystemSnapshot y;
  if (xSemaphoreTake(gMutex, pdMS_TO_TICKS(50)) == pdTRUE) { s = gSensor; y = gSystem; xSemaphoreGive(gMutex); }
  appendJson(out, sizeof(out), &used, "{\"temperature_c\":"); s.dhtOk ? appendJson(out,sizeof(out),&used,"%.1f",s.temperatureC) : appendJson(out,sizeof(out),&used,"null");
  appendJson(out,sizeof(out),&used,",\"humidity\":"); s.dhtOk ? appendJson(out,sizeof(out),&used,"%.1f",s.humidity) : appendJson(out,sizeof(out),&used,"null");
  appendJson(out,sizeof(out),&used,",\"dht_ok\":%s,\"distance_cm\":",s.dhtOk?"true":"false"); s.distanceOk ? appendJson(out,sizeof(out),&used,"%u",s.distanceCm) : appendJson(out,sizeof(out),&used,"null");
  appendJson(out,sizeof(out),&used,",\"distance_ok\":%s,\"light_is_dark\":%s,\"occupied\":%s,\"mode\":\"%s\",\"state\":\"%s\",\"relay1\":%s,\"relay2\":%s,\"temperature_alert\":%s,\"fault_reason\":\"",s.distanceOk?"true":"false",s.lightIsDark?"true":"false",y.occupied?"true":"false",modeName(y.mode),stateName(y.state),y.relay1On?"true":"false",y.relay2On?"true":"false",y.temperatureAlert?"true":"false");
  appendEscaped(out,sizeof(out),&used,faultReason(y.faultCode)); appendJson(out,sizeof(out),&used,"\",\"uptime_s\":%lu}",(unsigned long)(millis()/1000)); return String(out);
}

String buildLogsJson() {
  static char out[4096]; size_t used = 0; bool ok = appendJson(out,sizeof(out),&used,"["); size_t n = 0;
  if (xSemaphoreTake(gMutex, pdMS_TO_TICKS(50)) == pdTRUE) { n = gEventLog.count(); for (size_t i=0;i<n;i++){ EventLogEntry e=gEventLog.getEntry(i); if(i) ok &= appendJson(out,sizeof(out),&used,","); ok &= appendJson(out,sizeof(out),&used,"{\"time_s\":%lu,\"level\":\"%s\",\"message\":\"",(unsigned long)(e.timestampMs/1000),eventLevelName(e.level)); appendEscaped(out,sizeof(out),&used,e.message); ok &= appendJson(out,sizeof(out),&used,"\"}"); } xSemaphoreGive(gMutex); }
  if (!ok) appendJson(out,sizeof(out),&used,",{\"time_s\":0,\"level\":\"WARN\",\"message\":\"truncated\"}"); appendJson(out,sizeof(out),&used,"]"); return String(out);
}

String buildDiagnosticsJson() {
  static char out[3072]; size_t used=0; SystemSnapshot sys; ControlContext ctx;
  if (xSemaphoreTake(gMutex,pdMS_TO_TICKS(50))==pdTRUE){sys=gSystem; ctx=gControlContext; xSemaphoreGive(gMutex);} DiagnosticsSnapshot d=diagnosticsSnapshot(sys,ctx);
  appendJson(out,sizeof(out),&used,"{\"uptime_ms\":%lu,\"free_heap\":%lu,\"min_free_heap\":%lu,\"reset_reason\":%lu,\"wifi_connected\":%s,\"ap_mode\":%s,\"dht_fail_count\":%u,\"ultrasonic_fail_count\":%u,\"active_fault\":\"%s\",\"watchdog_fault\":%s,\"tasks\":[",(unsigned long)d.uptimeMs,(unsigned long)d.freeHeapBytes,(unsigned long)d.minFreeHeapBytes,(unsigned long)d.resetReason,d.wifiConnected?"true":"false",d.apMode?"true":"false",d.dhtFailCount,d.ultrasonicFailCount,faultReason(d.activeFault),d.watchdogFault?"true":"false");
  for(size_t i=0;i<(size_t)TaskId::COUNT;i++){TaskDiagnostics&t=d.tasks[i]; if(i)appendJson(out,sizeof(out),&used,","); appendJson(out,sizeof(out),&used,"{\"name\":\"%s\",\"heartbeat_count\":%lu,\"last_heartbeat_ms\":%lu,\"last_loop_ms\":%lu,\"max_loop_ms\":%lu,\"expected_period_ms\":%lu,\"max_jitter_ms\":%lu,\"deadline_misses\":%lu,\"healthy\":%s}",taskName((TaskId)i),(unsigned long)t.heartbeatCount,(unsigned long)t.lastHeartbeatMs,(unsigned long)t.lastLoopDurationMs,(unsigned long)t.maxLoopDurationMs,(unsigned long)t.expectedPeriodMs,(unsigned long)t.maxJitterMs,(unsigned long)t.deadlineMissCount,t.healthy?"true":"false");}
  appendJson(out,sizeof(out),&used,"]}"); return String(out);
}

String buildHistoryJson() {
  static char out[8192]; size_t used=0; bool ok=appendJson(out,sizeof(out),&used,"[");
  if(xSemaphoreTake(gMutex,pdMS_TO_TICKS(50))==pdTRUE){ for(size_t i=0;i<gHistory.count();i++){SensorHistoryEntry e; gHistory.get(i,&e); if(i) ok &= appendJson(out,sizeof(out),&used,","); ok &= appendJson(out,sizeof(out),&used,"{\"timestamp_ms\":%lu,\"temperature_c\":",(unsigned long)e.timestampMs); e.dhtOk ? ok &= appendJson(out,sizeof(out),&used,"%.1f",e.temperatureC) : ok &= appendJson(out,sizeof(out),&used,"null"); ok &= appendJson(out,sizeof(out),&used,",\"humidity\":"); e.dhtOk ? ok &= appendJson(out,sizeof(out),&used,"%.1f",e.humidity) : ok &= appendJson(out,sizeof(out),&used,"null"); ok &= appendJson(out,sizeof(out),&used,",\"dht_ok\":%s,\"distance_cm\":",e.dhtOk?"true":"false"); e.distanceOk ? ok &= appendJson(out,sizeof(out),&used,"%u",e.distanceCm) : ok &= appendJson(out,sizeof(out),&used,"null"); ok &= appendJson(out,sizeof(out),&used,",\"distance_ok\":%s,\"light_is_dark\":%s,\"occupied\":%s,\"relay1\":%s,\"relay2\":%s,\"state\":\"%s\",\"mode\":\"%s\"}",e.distanceOk?"true":"false",e.lightIsDark?"true":"false",e.occupied?"true":"false",e.relay1On?"true":"false",e.relay2On?"true":"false",stateName(e.state),modeName(e.mode)); if(!ok)break;} xSemaphoreGive(gMutex);} appendJson(out,sizeof(out),&used,"]"); return String(out);
}

String buildHistoryCsv() {
  static char out[8192]; size_t used=0; appendJson(out,sizeof(out),&used,"timestamp_ms,temperature_c,humidity,dht_ok,distance_cm,distance_ok,light_is_dark,occupied,relay1,relay2,state,mode\n");
  if(xSemaphoreTake(gMutex,pdMS_TO_TICKS(50))==pdTRUE){ for(size_t i=0;i<gHistory.count();i++){SensorHistoryEntry e; gHistory.get(i,&e); appendJson(out,sizeof(out),&used,"%lu,",(unsigned long)e.timestampMs); if(e.dhtOk) appendJson(out,sizeof(out),&used,"%.1f,%.1f,1,",e.temperatureC,e.humidity); else appendJson(out,sizeof(out),&used,",,0,"); if(e.distanceOk) appendJson(out,sizeof(out),&used,"%u,1,",e.distanceCm); else appendJson(out,sizeof(out),&used,",0,"); appendJson(out,sizeof(out),&used,"%u,%u,%u,%u,%s,%s\n",e.lightIsDark,e.occupied,e.relay1On,e.relay2On,stateName(e.state),modeName(e.mode)); } xSemaphoreGive(gMutex);} return String(out);
}

void handleRoot() { server.send(200,"text/html",R"rawliteral(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>EdgeGuard ESP32</title><style>body{margin:0;font-family:Arial;background:#0f172a;color:#e5e7eb}.wrap{max-width:1100px;margin:auto;padding:24px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:14px}.card{background:#172036;border:1px solid #334155;border-radius:16px;padding:16px;box-shadow:0 10px 30px #0004}.value{font-size:26px;font-weight:800}.badge{display:inline-block;padding:5px 9px;border-radius:999px;background:#334155}.INFO{color:#93c5fd}.WARN{color:#fbbf24}.FAULT{color:#f87171}.SECURITY{color:#c084fc}button,input{padding:10px;border-radius:10px;border:1px solid #475569;margin:4px}button{background:#38bdf8;color:#001018;font-weight:700;cursor:pointer}pre{white-space:pre-wrap;background:#020617;padding:12px;border-radius:12px;overflow:auto}a{color:#7dd3fc}</style></head><body><div class="wrap"><h1>EdgeGuard ESP32</h1><p>Local smart-room monitoring, diagnostics, and safe low-voltage relay control.</p><div class="card"><label>Local API token <input id="tok" type="password" placeholder="token"></label></div><div id="cards" class="grid"></div><div class="card"><h2>Controls</h2><button onclick="post('/api/mode/auto')">AUTO</button><button onclick="post('/api/mode/manual')">MANUAL</button><button onclick="post('/api/mode/away')">AWAY</button><button onclick="post('/api/relay1/on')">Relay 1 ON</button><button onclick="post('/api/relay1/off')">Relay 1 OFF</button><button onclick="post('/api/relay2/on')">Relay 2 ON</button><button onclick="post('/api/relay2/off')">Relay 2 OFF</button><button onclick="post('/api/fault/reset')">Reset Fault</button></div><div class="card"><h2>Diagnostics</h2><div id="diag"></div><table id="tasks"></table><p><a href="/api/history">JSON history</a> · <a href="/api/history.csv">CSV history</a></p></div><div class="card"><h2>Event Log</h2><pre id="logs"></pre></div></div><script>const tok=document.getElementById('tok');tok.value=localStorage.edgeguardToken||'';tok.oninput=()=>localStorage.edgeguardToken=tok.value;async function post(p){const r=await fetch(p,{method:'POST',headers:{'X-EdgeGuard-Token':tok.value}});if(!r.ok)alert(await r.text());refresh()}function yn(v){return v?'YES':'NO'}function card(k,v){return `<div class=card><div>${k}</div><div class=value>${v}</div></div>`}async function refresh(){const s=await(await fetch('/api/status')).json();cards.innerHTML=card('State',`<span class=badge>${s.state}</span>`)+card('Mode',s.mode)+card('Temperature',s.temperature_c===null?'DHT ERROR':s.temperature_c+' °C')+card('Humidity',s.humidity===null?'DHT ERROR':s.humidity+' %')+card('Distance',s.distance_cm===null?'NO ECHO':s.distance_cm+' cm')+card('Light',s.light_is_dark?'DARK':'BRIGHT')+card('Occupancy',yn(s.occupied))+card('Relays','R1 '+(s.relay1?'ON':'OFF')+' / R2 '+(s.relay2?'ON':'OFF'))+card('Fault',s.fault_reason||'No fault');const d=await(await fetch('/api/diagnostics')).json();diag.innerHTML=`Uptime ${Math.round(d.uptime_ms/1000)}s · Wi-Fi ${d.wifi_connected?'connected':'not connected'} · Heap ${d.free_heap} · Min heap ${d.min_free_heap} · Reset ${d.reset_reason} · Watchdog ${d.watchdog_fault?'FAULT':'OK'}`;tasks.innerHTML='<tr><th>Task</th><th>Beat</th><th>Max loop</th><th>Jitter</th><th>Misses</th></tr>'+d.tasks.map(t=>`<tr><td>${t.name}</td><td>${t.heartbeat_count}</td><td>${t.max_loop_ms}</td><td>${t.max_jitter_ms}</td><td>${t.deadline_misses}</td></tr>`).join('');const l=await(await fetch('/api/logs')).json();logs.innerHTML=l.map(e=>`<span class="${e.level}">[${e.level}]</span> ${e.time_s}s ${e.message}`).join('\n')}setInterval(refresh,1000);refresh()</script></body></html>)rawliteral"); }

void setupWebServer(){ static const char* headerKeys[] = {"X-EdgeGuard-Token"}; server.collectHeaders(headerKeys,1); server.on("/",HTTP_GET,handleRoot); server.on("/api/status",HTTP_GET,[]{server.send(200,"application/json",buildStatusJson());}); server.on("/api/logs",HTTP_GET,[]{server.send(200,"application/json",buildLogsJson());}); server.on("/api/diagnostics",HTTP_GET,[]{server.send(200,"application/json",buildDiagnosticsJson());}); server.on("/api/history",HTTP_GET,[]{server.send(200,"application/json",buildHistoryJson());}); server.on("/api/history.csv",HTTP_GET,[]{server.send(200,"text/csv",buildHistoryCsv());}); server.on("/api/mode/auto",HTTP_POST,[]{protectedPost([]{setMode(Mode::AUTO);});}); server.on("/api/mode/manual",HTTP_POST,[]{protectedPost([]{setMode(Mode::MANUAL);});}); server.on("/api/mode/away",HTTP_POST,[]{protectedPost([]{setMode(Mode::AWAY);});}); server.on("/api/relay1/on",HTTP_POST,[]{protectedPost([]{setManualRelay(1,true);});}); server.on("/api/relay1/off",HTTP_POST,[]{protectedPost([]{setManualRelay(1,false);});}); server.on("/api/relay2/on",HTTP_POST,[]{protectedPost([]{setManualRelay(2,true);});}); server.on("/api/relay2/off",HTTP_POST,[]{protectedPost([]{setManualRelay(2,false);});}); server.on("/api/fault/reset",HTTP_POST,[]{protectedPost([]{resetFault();});}); server.begin(); }

void connectWiFiOrStartAP(){ if(String(WIFI_SSID).length()>0 && String(WIFI_SSID)!="YOUR_WIFI_NAME"){ WiFi.mode(WIFI_STA); WiFi.begin(WIFI_SSID,WIFI_PASSWORD); uint32_t start=millis(); while(WiFi.status()!=WL_CONNECTED && millis()-start<15000){delay(500); Serial.print('.');} Serial.println(); if(WiFi.status()==WL_CONNECTED){Serial.print("Dashboard: http://"); Serial.println(WiFi.localIP()); logEvent(EventLevel::INFO,"Wi-Fi connected"); return;}} WiFi.mode(WIFI_AP); WiFi.softAP(EDGEGUARD_AP_SSID,EDGEGUARD_AP_PASSWORD); Serial.print("Fallback AP SSID: "); Serial.println(EDGEGUARD_AP_SSID); Serial.print("Dashboard: http://"); Serial.println(WiFi.softAPIP()); logEvent(EventLevel::WARN,"Started fallback AP mode"); }

void updateControl(const SensorSnapshot& sensor){ static State prev=State::BOOT; static bool r1=false,r2=false; SystemSnapshot previous; if(xSemaphoreTake(gMutex,pdMS_TO_TICKS(50))==pdTRUE){previous=gSystem; xSemaphoreGive(gMutex);} ControlResult result=updateControlLogic(sensor,previous,gControlContext,gControlConfig,millis()); SystemSnapshot sys=result.system; const uint32_t now=millis(); TaskDiagnostics sd=diagnosticsSnapshot(sys,gControlContext).tasks[(size_t)TaskId::SENSOR]; if(sd.lastHeartbeatMs && now-sd.lastHeartbeatMs > (2*SENSOR_TASK_PERIOD_MS+1000)){sys.state=State::FAULT; sys.faultCode=FaultCode::TASK_WATCHDOG; sys.relay1On=false; sys.relay2On=false; gWatchdogFault=true; diagnosticsSetWatchdogFault(true); logEvent(EventLevel::FAULT,"SensorTask watchdog timeout");} if(sys.state==State::FAULT){sys.relay1On=false; sys.relay2On=false;} applyRelays(sys.relay1On,sys.relay2On); diagnosticsUpdateSensorFailures(gControlContext.dhtFailCount,gControlContext.ultrasonicFailCount); if(sys.state!=prev){logEvent(sys.state==State::FAULT?EventLevel::FAULT:EventLevel::INFO,String("State changed to ")+stateName(sys.state)); prev=sys.state;} if(sys.relay1On!=r1){logEvent(EventLevel::INFO,String("Relay 1 ")+(sys.relay1On?"ON":"OFF")); r1=sys.relay1On;} if(sys.relay2On!=r2){logEvent(EventLevel::INFO,String("Relay 2 ")+(sys.relay2On?"ON":"OFF")); r2=sys.relay2On;} if(xSemaphoreTake(gMutex,pdMS_TO_TICKS(50))==pdTRUE){gSystem=sys; SensorHistoryEntry e; e.timestampMs=now; e.temperatureC=sensor.temperatureC; e.humidity=sensor.humidity; e.dhtOk=sensor.dhtOk; e.distanceCm=sensor.distanceCm; e.distanceOk=sensor.distanceOk; e.lightIsDark=sensor.lightIsDark; e.occupied=sys.occupied; e.relay1On=sys.relay1On; e.relay2On=sys.relay2On; e.state=sys.state; e.mode=sys.mode; gHistory.push(e); xSemaphoreGive(gMutex);} }

void SensorTask(void*){for(;;){uint32_t start=millis(); SensorSnapshot reading=readSensors(); if(xSemaphoreTake(gMutex,pdMS_TO_TICKS(50))==pdTRUE){gSensor=reading; xSemaphoreGive(gMutex);} if(!reading.dhtOk) logEvent(EventLevel::WARN,"DHT read failed"); if(!reading.distanceOk) logEvent(EventLevel::WARN,"HC-SR04 echo timeout"); diagnosticsTaskHeartbeat(TaskId::SENSOR,millis(),millis()-start); vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS));}}
void ControlTask(void*){for(;;){uint32_t start=millis(); SensorSnapshot sensor; if(xSemaphoreTake(gMutex,pdMS_TO_TICKS(50))==pdTRUE){sensor=gSensor; xSemaphoreGive(gMutex);} updateControl(sensor); diagnosticsTaskHeartbeat(TaskId::CONTROL,millis(),millis()-start); vTaskDelay(pdMS_TO_TICKS(CONTROL_TASK_PERIOD_MS));}}
void WebTask(void*){for(;;){uint32_t start=millis(); server.handleClient(); diagnosticsTaskHeartbeat(TaskId::WEB,millis(),millis()-start); vTaskDelay(pdMS_TO_TICKS(10));}}
void HeartbeatTask(void*){bool led=false; for(;;){uint32_t start=millis(); SystemSnapshot sys; if(xSemaphoreTake(gMutex,pdMS_TO_TICKS(50))==pdTRUE){sys=gSystem; xSemaphoreGive(gMutex);} led=!led; if(sys.state==State::FAULT){digitalWrite(PIN_LED_GREEN,LOW); digitalWrite(PIN_LED_RED,led?HIGH:LOW); diagnosticsTaskHeartbeat(TaskId::HEARTBEAT,millis(),millis()-start); vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_FAULT_MS));} else if(sys.state==State::ALERT){digitalWrite(PIN_LED_GREEN,HIGH); digitalWrite(PIN_LED_RED,led?HIGH:LOW); diagnosticsTaskHeartbeat(TaskId::HEARTBEAT,millis(),millis()-start); vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_ALERT_MS));} else {digitalWrite(PIN_LED_RED,LOW); digitalWrite(PIN_LED_GREEN,led?HIGH:LOW); diagnosticsTaskHeartbeat(TaskId::HEARTBEAT,millis(),millis()-start); vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_NORMAL_MS));}}}

void setupPins(){pinMode(PIN_DHT,INPUT); pinMode(PIN_HCSR04_TRIG,OUTPUT); pinMode(PIN_HCSR04_ECHO,INPUT); pinMode(PIN_LDR_DO,INPUT); pinMode(PIN_RELAY1,OUTPUT); pinMode(PIN_RELAY2,OUTPUT); pinMode(PIN_LED_GREEN,OUTPUT); pinMode(PIN_LED_RED,OUTPUT); applyRelays(false,false); digitalWrite(PIN_LED_GREEN,LOW); digitalWrite(PIN_LED_RED,LOW);}
void setup(){Serial.begin(115200); delay(1000); gMutex=xSemaphoreCreateMutex(); diagnosticsInit(); diagnosticsSetExpectedPeriod(TaskId::SENSOR,SENSOR_TASK_PERIOD_MS); diagnosticsSetExpectedPeriod(TaskId::CONTROL,CONTROL_TASK_PERIOD_MS); diagnosticsSetExpectedPeriod(TaskId::WEB,10); diagnosticsSetExpectedPeriod(TaskId::HEARTBEAT,HEARTBEAT_NORMAL_MS); gControlConfig=defaultControlConfig(); gControlConfig.occupiedDistanceCm=OCCUPIED_DISTANCE_CM; gControlConfig.unoccupiedTimeoutMs=UNOCCUPIED_TIMEOUT_MS; gControlConfig.tempAlertOnC=TEMP_ALERT_ON_C; gControlConfig.tempAlertOffC=TEMP_ALERT_OFF_C; setupPins(); dht.begin(); if(String(EDGEGUARD_API_TOKEN).length()==0) logEvent(EventLevel::WARN,"Control API token is disabled"); logEvent(EventLevel::INFO,"BOOT started"); if(xSemaphoreTake(gMutex,pdMS_TO_TICKS(50))==pdTRUE){gSystem.mode=Mode::AUTO; gSystem.state=State::CALIBRATING; gSystem.relay1On=false; gSystem.relay2On=false; xSemaphoreGive(gMutex);} connectWiFiOrStartAP(); setupWebServer(); logEvent(EventLevel::INFO,"Web server started"); xTaskCreatePinnedToCore(SensorTask,"SensorTask",4096,nullptr,2,nullptr,1); xTaskCreatePinnedToCore(ControlTask,"ControlTask",4096,nullptr,2,nullptr,1); xTaskCreatePinnedToCore(WebTask,"WebTask",8192,nullptr,1,nullptr,0); xTaskCreatePinnedToCore(HeartbeatTask,"HeartbeatTask",2048,nullptr,1,nullptr,1); logEvent(EventLevel::INFO,"Tasks created");}
void loop(){delay(1000);}
