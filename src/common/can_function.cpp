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
extern setUp_cfg setup_cfg;
extern CanMessage periodicMessages[30];
extern CanResponse responseMessages[30];
extern CanResponseCheck responseCheck; 

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
    while (setup_cfg.bit_cfg == 0){
      Serial.println("Please enter bitrates");
    }
    if (!CAN.begin(setup_cfg.bit_cfg)) {
        Serial.println("Starting CAN failed!");
        while (1);
    }
    CAN.onReceive(onReceive);
}

void mode1(void){
  if(setup_cfg.enable_cfg == 1){
    for (int i = 0; i < setup_cfg.periodic_cfg; i++) {
       if (now - periodicMessages[i].lastSent >= periodicMessages[i].period) {
          CAN.beginExtendedPacket(periodicMessages[i].id);
          CAN.write(periodicMessages[i].data, 8);
          CAN.endPacket();
          periodicMessages[i].lastSent = now;
        }
    }
  }
}

void mode2(void){
  if(setup_cfg.enable_cfg == 1){
    for (int i = 0; i < setup_cfg.response_cfg; i++) {
       if (responseMessages[i].id == responseCheck.id && memcmp(responseMessages[i].data, responseCheck.data, 8) == 0) {
          CAN.beginExtendedPacket(responseMessages[i].responseId);
          CAN.write(responseMessages[i].responseData, 8);
          CAN.endPacket();
        }
    }
  }
}


