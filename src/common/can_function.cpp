#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <CAN.h>
#include <Arduino.h>
#include <esp_intr_alloc.h>
#include <nvs_flash.h>
#include <nvs.h>

#include "cfg/can_cfg.h"
#include "cfg/host.h"
#include "common/can_function.h"
#include "common/http_function.h"
#include "common/nvs_function.h"

#define TX_GPIO_NUM   27
#define RX_GPIO_NUM   26

static uint32_t now = millis();
extern CanMessage periodicMessages[30];
extern CanResponse responseMessages[30];
extern QueueHandle_t httpQueue;
CanResponseCheck responseCheck;
setUp_cfg_new setup_cfg_new;

bool change_cfg = false;
static String response;

void onReceive(int packetSize) {
  Serial.print("Received ");
  if (CAN.packetExtended()) {
    Serial.print("extended ");
  }
  if (CAN.packetRtr()) {
    Serial.print("RTR ");
  }
  Serial.print("packet with id 0x");
  Serial.print(CAN.packetId(), HEX);
  responseCheck.id = CAN.packetId();
  if (CAN.packetRtr()) {
    Serial.print(" and requested length ");
    Serial.println(CAN.packetDlc());
  } else {
    Serial.print(" and length ");
    Serial.println(packetSize);
    
    int index = 0;
    while (CAN.available()) {
      int value = CAN.read();
      Serial.print((char)value);
      if (index < sizeof(responseCheck.data)) {
        responseCheck.data[index++] = (uint8_t)value;
      }
    }
    Serial.println();
  }
  Serial.println();
}

void setup_can(void){
  CAN.setPins (RX_GPIO_NUM, TX_GPIO_NUM);
  if (!CAN.begin(setup_cfg_new.bit_cfg)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }
  CAN.onReceive(onReceive);
}

void setup_can_cfg(void){
  Queue_msg in_msg;
  NVS_Read("mode_s", &mode_s);
  NVS_Read("periodic_s", &periodic_s);
  NVS_Read("enable_s", &enable_s);
  NVS_Read("bit_s", &bit_s);
  NVS_Read("response_s", &response_s);
  setup_cfg_new.mode_cfg = mode_s;
  setup_cfg_new.enable_cfg = enable_s;
  setup_cfg_new.bit_cfg = bit_s;
  char key[20];
  if (mode_s == 0) {
    in_msg.mode_evt = ModeEvent::PERIOD_MODE;
    JsonDocument doc;
    JsonArray messages = doc["messages"].to<JsonArray>();
    for (int i = 0; i < periodic_s; i++) {
      sprintf(key, "peri_struct%d", i + 1);
      if (NVS_Read_Struct(key, &periodicMessages[i], sizeof(CanMessage)) == ESP_OK) {
        JsonObject message = messages.add<JsonObject>();
        message["id"] = String(periodicMessages[i].id, HEX);
        message["data"] = bytesToHexString(periodicMessages[i].data, sizeof(periodicMessages[i].data));
        message["period"] = periodicMessages[i].period;
        message["lastSent"] = periodicMessages[i].lastSent;
      }
    }
    serializeJson(doc, response);
  }
  else if (mode_s == 1) {
    in_msg.mode_evt = ModeEvent::REQ_RES_MODE;
    JsonDocument doc;
    JsonArray messages = doc["messages"].to<JsonArray>();
    for (int i = 0; i < response_s; i++) {
      sprintf(key, "res_struct%d", i + 1);
      if (NVS_Read_Struct(key, &responseMessages[i], sizeof(CanResponse)) == ESP_OK) {
        JsonObject message = messages.add<JsonObject>();
        message["id"] = String(responseMessages[i].id, HEX);
        message["data"] = bytesToHexString(responseMessages[i].data, sizeof(responseMessages[i].data));
        message["responseId"] = String(responseMessages[i].responseId, HEX);
        message["responseData"] = bytesToHexString(responseMessages[i].responseData, sizeof(responseMessages[i].responseData));
      }
    }
    serializeJson(doc, response);
  }
}

void mode1(void){
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, response);
  if (setup_cfg_new.enable_cfg == 1) {
        JsonArray messages = doc["messages"];
        for (JsonObject message : messages) {
            unsigned long id = strtoul(message["id"], NULL, 16);
            const char* dataStr = message["data"];
            unsigned long period = message["period"];
            unsigned long lastSent = message["lastSent"];
            uint8_t data[8];
            for (int i = 0; i < 8; i++) {
                sscanf(dataStr + 2 * i + 2, "%02hhx", &data[i]);
            }
            if (now - lastSent >= period) {
                CAN.beginExtendedPacket(id);
                CAN.write(data, 8);
                CAN.endPacket();
                message["lastSent"] = now; 
            }
        }
    }
}

void mode2(void){
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, response);
  if (setup_cfg_new.enable_cfg == 1) {
        JsonArray messages = doc["messages"];
        for (JsonObject message : messages) {
            unsigned long id = strtoul(message["id"], NULL, 16);
            const char* dataStr = message["data"];
            unsigned long responseId = strtoul(message["responseId"], NULL, 16);
            const char* responseDataStr = message["responseData"];
            uint8_t data[8];
            for (int i = 0; i < 8; i++) {
                sscanf(dataStr + 2 * i, "%02hhx", &data[i]);
            }
            uint8_t responseData[8];
            for (int i = 0; i < 8; i++) {
                sscanf(responseDataStr + 2 * i, "%02hhx", &responseData[i]);
            }
            if (responseCheck.id == id && memcmp(responseCheck.data, data, 8) == 0) {
                CAN.beginExtendedPacket(responseId);
                CAN.write(responseData, 8);
                CAN.endPacket();
            }
        }
    }
}

void can_entry(void *pvParameters){
  Queue_msg in_msg;
  setup_can_cfg();
  while (1){
    int rc;
    setup_can();
    switch (in_msg.mode_evt) {
      case PERIOD_MODE:
        mode1();
        break;
      case REQ_RES_MODE:
        mode2();
        break;
      default:
        break;
    }
    rc = xQueueReceive(httpQueue, &in_msg, 100);
    if (rc == pdPASS) {
      change_cfg = true;
      Serial.println("Receive new config");
    }
    if(change_cfg){
      setup_can_cfg();
      Serial.println("Receive new config");
      change_cfg = false;
    }
  }
}

