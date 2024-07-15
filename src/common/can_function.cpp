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

extern CanMessage periodicMessages[30];
extern CanResponse responseMessages[30];
extern QueueHandle_t httpQueue;
ModeEvent mode_evt;
CanResponseCheck responseCheck;
setUp_cfg_new setup_cfg_new;

bool change_cfg = false;
static String response;
volatile bool messageReceived = false;

void on_receive(int packetSize) {
  messageReceived = true;
}

void can_receive() {
  if (messageReceived) {
    messageReceived = false;
    int packetSize = CAN.parsePacket();
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
    } 
    else {
      Serial.print(" and length ");
      Serial.println(packetSize);
        
      int index = 0;
      memset(responseCheck.data, 0, sizeof(responseCheck.data)); 
      while (CAN.available() && index < sizeof(responseCheck.data)) {
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
}

void setup_can(void){
  CAN.setPins (RX_GPIO_NUM, TX_GPIO_NUM);
  if (!CAN.begin(setup_cfg_new.bit_cfg)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }
  CAN.onReceive(on_receive);
}

void setup_can_cfg(void){
  NVS_Read("mode_s", &mode_s);
  NVS_Read("enable_s", &enable_s);
  NVS_Read("bit_s", &bit_s);
  setup_cfg_new.mode_cfg = mode_s;
  setup_cfg_new.enable_cfg = enable_s;
  setup_cfg_new.bit_cfg = bit_s;
  char key[20];
  if (mode_s == 0) {
    mode_evt = ModeEvent::PERIOD_MODE;
    NVS_Read("periodic_s", &periodic_s);
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
    mode_evt = ModeEvent::REQ_RES_MODE;
    NVS_Read("response_s", &response_s);
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
  uint32_t now = millis();
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
  }
  if (setup_cfg_new.enable_cfg == 1) {
        JsonArray messages = doc["messages"];
        for (JsonObject message : messages) {
            unsigned long id = strtoul(message["id"], NULL, 16);
            const char* dataStr = message["data"];
            unsigned long period = message["period"];
            unsigned long lastSent = message["lastSent"];
            uint8_t data[8];
            hexStringToBytes(dataStr, data);
            if (now - lastSent >= period) {
              CAN.beginExtendedPacket(id);
              CAN.write(data, 8);
              CAN.endPacket();
              message["lastSent"] = now; 
            }
        }
    }
  serializeJson(doc, response);
}

void mode2(void){
  can_receive();
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
  }
  if (setup_cfg_new.enable_cfg == 1) {
        JsonArray messages = doc["messages"];
        for (JsonObject message : messages) {
            unsigned long id = strtoul(message["id"], NULL, 16);
            const char* dataStr = message["data"];
            unsigned long responseId = strtoul(message["responseId"], NULL, 16);
            const char* responseDataStr = message["responseData"];
            uint8_t data[8];
            hexStringToBytes(dataStr, data);
            uint8_t responseData[8];
            hexStringToBytes(responseDataStr, responseData);
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
    switch (mode_evt) {
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
      if(in_msg.check_change){
      change_cfg = true;
      Serial.println("Receive new config");
      }
    }
    if(change_cfg){
      setup_can_cfg();
      change_cfg = false;
    }
  }
}

