#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <CAN.h>
#include <Arduino.h>
#include <esp_intr_alloc.h>
#include <nvs_flash.h>
#include <nvs.h>

#include "cfg/host.h"
#include "cfg/can_cfg.h"
#include "common/nvs_function.h"
#include "common/http_function.h"
#include "common/can_function.h"

WebServer server(80);
CanMessage periodicMessages[30];
CanResponse responseMessages[30];
CanResponseCheck responseCheck;

void setup() {
  Serial.begin(115200);
  init_nvs();
  setupAP();
  server.on("/enable", HTTP_GET, get_program_running);
  server.on("/enable", HTTP_POST, start_stop_program);
  server.on("/mode", HTTP_GET, getMode);
  server.on("/mode", HTTP_POST, setMode);
  server.on("/period_cfg", HTTP_GET, get_periodic_cfg);
  server.on("/period_cfg", HTTP_POST, set_periodic_cfg);
  server.on("/req_res_cfg", HTTP_GET, get_req_res_cfg);
  server.on("/req_res_cfg", HTTP_POST, set_req_res_cfg);
  server.begin();
}

void loop() {
  server.handleClient();
  NVS_Read("Mode_S", &Mode_S);
  switch (Mode_S) {
    case 1:
      mode1();
      break;
    case 2:
      mode2();
      break;
    default:
      break;
  }
}
