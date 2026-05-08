#pragma once

// === СЕТЬ ===
#define WIFI_SSID "Your_WiFi_SSID"
#define WIFI_PASSWORD "Your_WiFi_Password"

// === ВЫБОР ЗОНЫ ===
// 1 = Барная стойка (960 LED)
// 2 = Над диваном (1200 LED)
// 3 = Центральная (48 LED)
#define CURRENT_ZONE 1

#if CURRENT_ZONE == 1
  #define ZONE_NAME "Bar_Counter"
  #define LED_PIN 18
  #define LED_COUNT 960
  #define HAS_DISPLAY true
#elif CURRENT_ZONE == 2
  #define ZONE_NAME "Sofa_Light"
  #define LED_PIN 18
  #define LED_COUNT 1200
  #define HAS_DISPLAY false
#elif CURRENT_ZONE == 3
  #define ZONE_NAME "Central_Rings"
  #define LED_PIN 18
  #define LED_COUNT 48
  #define HAS_DISPLAY false
#else
  #error "Invalid CURRENT_ZONE. Set to 1, 2, or 3."
#endif

// === FASTLED & ПИТАНИЕ ===
// Ограничение тока: 5V × 4000mA = 20W на зону. 
// При 60% яркости 1200 LED потребуют ~43A, но в реальности ограничьте яркость в UI.
#define MAX_CURRENT_MA 4000 
#define BRIGHTNESS_DEFAULT 64 // 0-255

// === WEB SERVER ===
#define WEB_PORT 80
