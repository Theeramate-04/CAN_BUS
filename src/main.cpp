#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <CAN.h>
#include <Arduino.h>
#include <esp_intr_alloc.h>
#include <nvs_flash.h>
#include <nvs.h>

#include "cfg/host.h"
#include "cfg/can_struct.h"
#include "common/nvs_function.h"
#include "common/http_function.h"
#include "common/can_function.h"

extern WebServer server;
extern int Mode;

void setup() {
  Serial.begin(115200);
  setupAP();
  server.begin();
}

void loop() {
  server.handleClient();
  switch (Mode) {
    case 1:
      mode1();
      break;
    case 2:
      mode2();
      break;
    default:
      Serial.println("Mode Error");
      break;
  }
}