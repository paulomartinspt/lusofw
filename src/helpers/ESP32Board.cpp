#ifdef ESP_PLATFORM

#include "ESP32Board.h"
#include <target.h>

#if defined(ADMIN_PASSWORD) && !defined(DISABLE_WIFI_OTA)   // Repeater or Room Server only
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include <SPIFFS.h>

bool ESP32Board::startOTAUpdate(const char* id, char reply[]) {
  // Keep a single OTA server/session alive; repeated CLI requests return the
  // active credentials instead of creating another listener on port 80.
  static AsyncWebServer* ota_server = nullptr;
  static char wifi_password[17];
  static char http_password[17];
  static char id_buf[60];
  static char home_buf[90];

  auto generate_password = [](char* password) {
    uint8_t random_bytes[8];
    for (size_t i = 0; i < sizeof(random_bytes); i += sizeof(uint32_t)) {
      uint32_t random_value = esp_random();
      memcpy(&random_bytes[i], &random_value, sizeof(random_value));
    }
    for (size_t i = 0; i < sizeof(random_bytes); i++) {
      snprintf(&password[i * 2], 3, "%02x", random_bytes[i]);
    }
  };

  if (ota_server) {
    String ip = WiFi.softAPIP().toString();
    snprintf(reply, 161,
             "OTA already running: http://%s/update Wi-Fi password: %s HTTP user: meshcore HTTP password: %s",
             ip.c_str(), wifi_password, http_password);
    return true;
  }

  inhibit_sleep = true;   // prevent sleep during OTA
  // Enable the Wi-Fi radio before requesting random bytes from the ESP32 RNG.
  if (!WiFi.mode(WIFI_AP)) {
    WiFi.mode(WIFI_OFF);
    inhibit_sleep = false;
    strcpy(reply, "Error: could not enable Wi-Fi for OTA");
    return false;
  }
  delay(1);

  // Use independent high-entropy credentials for Wi-Fi association and HTTP auth.
  generate_password(wifi_password);
  generate_password(http_password);

  if (!WiFi.softAP("MeshCore-OTA", wifi_password)) {
    WiFi.mode(WIFI_OFF);
    inhibit_sleep = false;
    strcpy(reply, "Error: could not start secure OTA access point");
    return false;
  }

  String ip = WiFi.softAPIP().toString();
  snprintf(reply, 161,
           "Started: http://%s/update Wi-Fi password: %s HTTP user: meshcore HTTP password: %s",
           ip.c_str(), wifi_password, http_password);
  // Never put either temporary credential in serial/debug logs.
  MESH_DEBUG_PRINTLN("startOTAUpdate: secure OTA access point started at %s", ip.c_str());

  snprintf(id_buf, sizeof(id_buf), "%s (%s)", id, getManufacturerName());
  snprintf(home_buf, sizeof(home_buf), "<H2>Hi! I am a MeshCore Repeater. ID: %s</H2>", id);

  AsyncWebServer* server = new AsyncWebServer(80);
  if (!server) {
    WiFi.softAPdisconnect(true);
    inhibit_sleep = false;
    strcpy(reply, "Error: could not allocate OTA web server");
    return false;
  }

  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate("meshcore", http_password)) {
      request->requestAuthentication();
      return;
    }
    request->send(200, "text/html", home_buf);
  });
  server->on("/log", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate("meshcore", http_password)) {
      request->requestAuthentication();
      return;
    }
    request->send(SPIFFS, "/packet_log", "text/plain");
  });

  AsyncElegantOTA.setID(id_buf);
  AsyncElegantOTA.begin(server, "meshcore", http_password); // Require HTTP authentication
  server->begin();
  ota_server = server;

  return true;
}
#else
bool ESP32Board::startOTAUpdate(const char* id, char reply[]) {
  return false; // not supported
}
#endif

void ESP32Board::powerOff() {
  enterDeepSleep(0); // Do not wakeup
}

void ESP32Board::enterDeepSleep(uint32_t secs) {
  // Power off the display if any
#ifdef DISPLAY_CLASS
  display.turnOff();
#endif

  // Power off LoRa
  radio_driver.powerOff();

  // Keep LoRa inactive during deepsleep
  digitalWrite(P_LORA_NSS, HIGH);
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
  gpio_hold_en((gpio_num_t)P_LORA_NSS);
#else
  rtc_gpio_hold_en((gpio_num_t)P_LORA_NSS);
#endif

  // Power off GPS if any
  if (sensors.getLocationProvider() != NULL) {
    sensors.getLocationProvider()->stop();
  }

  // Flush serial buffers
  Serial.flush();
  delay(100);

  // Clear stale wakeup sources to avoid ghost wakeup
  // This is required when Power Management and automatic lightsleep are enabled
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

  if (secs > 0) {
    esp_sleep_enable_timer_wakeup(secs * 1000000ULL);
  }

  // Finally set ESP32 into deepsleep
  esp_deep_sleep_start(); // CPU halts here and never returns!
}
#endif
