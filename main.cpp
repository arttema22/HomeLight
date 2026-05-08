#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <FastLED.h>
#include <ArduinoJson.h>
#include "config.h"

// Глобальные объекты
CRGB leds[LED_COUNT];
AsyncWebServer server(WEB_PORT);

uint8_t brightness = BRIGHTNESS_DEFAULT;
CRGB currentColor = CRGB::Blue;
String currentEffect = "solid";
unsigned long lastUpdate = 0;
uint8_t hue = 0;

// --- Web UI ---
void handleRoot(AsyncWebServerRequest *request) {
  String html = R"rawliteral(
  <!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>
  <title>HomeLight - )rawliteral" + String(ZONE_NAME) + R"rawliteral(</title>
  <style>body{font-family:system-ui,sans-serif;background:#1a1a1a;color:#e0e0e0;text-align:center;padding:20px;margin:0}
  .card{background:#2a2a2a;border-radius:12px;padding:20px;max-width:400px;margin:20px auto}
  input[type=range]{width:100%;margin:10px 0} .btn{padding:10px 16px;margin:5px;border:none;border-radius:6px;cursor:pointer;font-weight:bold}
  .btn-on{background:#4CAF50;color:#fff} .btn-off{background:#f44336;color:#fff}
  </style></head><body>
  <div class='card'>
    <h2>🏠 )rawliteral" + String(ZONE_NAME) + R"rawliteral(</h2>
    <p>LEDs: )rawliteral" + String(LED_COUNT) + R"rawliteral( | IP: <span id='ip'></span></p>
    <label>Яркость: <span id='bVal'>)rawliteral" + String(BRIGHTNESS_DEFAULT) + R"rawliteral(</span></label>
    <input type='range' min='0' max='255' value=')rawliteral" + String(BRIGHTNESS_DEFAULT) + R"rawliteral(' oninput='setB(this.value)'>
    <label>Цвет: </label><input type='color' id='cp' value='#0000ff' oninput='setC(this.value)' style='height:40px;width:100%;margin:5px 0'>
    <div>
      <button class='btn btn-on' onclick='setE("solid")'>Solid</button>
      <button class='btn btn-on' onclick='setE("rainbow")'>Rainbow</button>
      <button class='btn btn-off' onclick='setE("off")'>OFF</button>
    </div>
  </div>
  <script>
    fetch('/state').then(r=>r.json()).then(d=>document.getElementById('ip').innerText=d.ip);
    function setB(v){fetch('/api?b='+v);document.getElementById('bVal').innerText=v}
    function setC(v){fetch('/api?c='+v)}
    function setE(e){fetch('/api?e='+e)}
  </script></body></html>
  )rawliteral";
  request->send(200, "text/html", html);
}

void handleAPI(AsyncWebServerRequest *request) {
  if (request->hasParam("b")) {
    brightness = request->getParam("b")->value().toInt();
    FastLED.setBrightness(brightness);
  }
  if (request->hasParam("c")) {
    String hex = request->getParam("c")->value();
    hex.replace("#", "");
    
    // Корректное преобразование HEX строки в структуру CRGB
    long colorValue = strtol(hex.c_str(), NULL, 16);
    currentColor.r = (colorValue >> 16) & 0xFF;
    currentColor.g = (colorValue >> 8) & 0xFF;
    currentColor.b = colorValue & 0xFF;

    if (currentEffect == "off") currentEffect = "solid";
    // Сразу применяем цвет, чтобы не ждать следующего цикла loop
    fill_solid(leds, LED_COUNT, currentColor);
    FastLED.show();
  }
  if (request->hasParam("e")) {
    currentEffect = request->getParam("e")->value();
    if (currentEffect == "off") {
      FastLED.clear();
      FastLED.show();
    } else if (currentEffect == "solid") {
      // При переключении на solid сразу применяем текущий цвет
      fill_solid(leds, LED_COUNT, currentColor);
      FastLED.show();
      lastUpdate = millis(); // Сброс таймера
    }
  }
  request->send(200, "text/plain", "OK");
}

void handleState(AsyncWebServerRequest *request) {
  JsonDocument doc;
  doc["zone"] = ZONE_NAME;
  doc["leds"] = LED_COUNT;
  doc["brightness"] = brightness;
  char hexColor[8];
  sprintf(hexColor, "#%02X%02X%02X", currentColor.red, currentColor.green, currentColor.blue);
  doc["color"] = hexColor;
  doc["effect"] = currentEffect;
  doc["ip"] = WiFi.localIP().toString();
  
  String json;
  serializeJson(doc, json);
  request->send(200, "application/json", json);
}

void setupWeb() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api", HTTP_GET, handleAPI);
  server.on("/state", HTTP_GET, handleState);
  server.begin();
}

void setup() {
  Serial.begin(115200);
  Serial.printf("\n[HomeLight] Booting %s (%d LEDs, Pin %d)\n", ZONE_NAME, LED_COUNT, LED_PIN);

  // Инициализация FastLED
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, LED_COUNT);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_CURRENT_MA); // ⚡ КРИТИЧНО: защита БП
  FastLED.setBrightness(brightness);
  FastLED.clear();
  FastLED.show();

  // Wi-Fi с таймаутом подключения
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WiFi] Connecting");
  
  int attempts = 0;
  const int maxAttempts = 40; // 20 секунд таймаут
  
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] OK! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WiFi] Connection failed!");
    // Можно добавить режим точки доступа или перезагрузку
  }

  setupWeb();
  Serial.println("[Web] Server running.");
}

void loop() {
  unsigned long now = millis();

  if (currentEffect == "rainbow") {
    if (now - lastUpdate > 20) {
      lastUpdate = now;
      fill_rainbow(leds, LED_COUNT, hue, 7);
      FastLED.show();
      hue++;
    }
  } 
  else if (currentEffect == "solid") {
    // Для solid цвета обновление не требуется каждый кадр, 
    // так как цвет устанавливается сразу при изменении через API
    // Здесь можно оставить пустым или добавить проверку на изменение яркости
  }
  // Для "off" ничего не делаем, светодиоды выключены
}
