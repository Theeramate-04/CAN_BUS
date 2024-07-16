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
QueueHandle_t httpQueue;

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  init_nvs();

  httpQueue = xQueueCreate(10, sizeof(queue_msg));
  xTaskCreate(http_entry, "HTTP_SERVICE", 4096, NULL, 1, NULL);
  xTaskCreate(can_entry, "CAN_SERVICE", 8192, NULL, 1, NULL);

}

void loop() {}
