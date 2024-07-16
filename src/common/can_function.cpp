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

extern http_periodic http_periodic_messages[30];
extern http_response http_response_messages[30];
extern QueueHandle_t httpQueue;
can_periodic can_periodic_messages[30];
can_response can_response_messages[30];
mode_event mode_evt;
can_response_check responseCheck;
setUp_cfg_new setup_cfg_new;

bool change_cfg = false;
static String response;
volatile bool messageReceived = false;
/**
/*!
    @brief  Interrupt service routine to handle CAN message reception.
*/
void on_receive(int packetSize) {
  messageReceived = true;
}
/**
/*!
    @brief  Handles CAN message reception and processes received messages.
*/
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
/**
/*!
    @brief  Configures CAN settings such as pins and bitrate.
*/
void setup_can(void){
  CAN.setPins (RX_GPIO_NUM, TX_GPIO_NUM);
  if (!CAN.begin(setup_cfg_new.bit_cfg)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }
  CAN.onReceive(on_receive);
}
/**
/*!
    @brief  Sets up CAN configuration from NVS storage.
*/
void setup_can_cfg(void){
  NVS_Read("mode_s", &mode_s);
  NVS_Read("enable_s", &enable_s);
  NVS_Read("bit_s", &bit_s);
  setup_cfg_new.mode_cfg = mode_s;
  setup_cfg_new.enable_cfg = enable_s;
  setup_cfg_new.bit_cfg = bit_s;
  char key[20];
  if (mode_s == 0) {
    mode_evt = mode_event::PERIOD_MODE;
    NVS_Read("periodic_s", &periodic_s);
    setup_cfg_new.periodic_count_cfg = periodic_s;
    for (int i = 0; i < periodic_s; i++) {
      sprintf(key, "peri_struct%d", i + 1);
      if (NVS_Read_Struct(key, &http_periodic_messages[i], sizeof(http_periodic)) == ESP_OK) {
        can_periodic_messages[i].id = http_periodic_messages[i].id;
        memcpy(can_periodic_messages[i].data, http_periodic_messages[i].data, sizeof(can_periodic_messages[i].data));
        can_periodic_messages[i].period = http_periodic_messages[i].period;
        can_periodic_messages[i].lastSent = http_periodic_messages[i].lastSent;
      }
    }
  }
  else if (mode_s == 1) {
    mode_evt = mode_event::REQ_RES_MODE;
    NVS_Read("response_s", &response_s);
    setup_cfg_new.response_count_cfg = response_s;
    for (int i = 0; i < response_s; i++) {
      sprintf(key, "res_struct%d", i + 1);
      if (NVS_Read_Struct(key, &http_response_messages[i], sizeof(http_response)) == ESP_OK) {
        can_response_messages[i].id = http_response_messages[i].id;
        memcpy(can_response_messages[i].data, http_response_messages[i].data, sizeof(can_response_messages[i].data));
        can_response_messages[i].responseId = http_response_messages[i].responseId;
        memcpy(can_response_messages[i].responseData, http_response_messages[i].responseData, sizeof(can_response_messages[i].responseData));
      }
    }
  }
}
/**
/*!
    @brief  Handles periodic mode CAN message transmission.
*/
void mode1(void){
  uint32_t now = millis();
  if (setup_cfg_new.enable_cfg == 1) {
        for (int i = 0; i < setup_cfg_new.periodic_count_cfg; i++) {
        if (now - can_periodic_messages[i].lastSent >= can_periodic_messages[i].period) {
          Serial.println(can_periodic_messages[i].id);
          CAN.beginExtendedPacket(can_periodic_messages[i].id);
          CAN.write(can_periodic_messages[i].data, 8);
          CAN.endPacket();
          can_periodic_messages[i].lastSent = now;
        }
    }
  }
}
/**
/*!
    @brief  Handles request-response mode CAN message transmission.
*/
void mode2(void){
  can_receive();
  if (setup_cfg_new.enable_cfg == 1) {
      for (int i = 0; i < setup_cfg_new.response_count_cfg; i++) {
        if (can_response_messages[i].id == responseCheck.id && memcmp(can_response_messages[i].data, responseCheck.data, 8) == 0) {
          CAN.beginExtendedPacket(can_response_messages[i].responseId);
          CAN.write(can_response_messages[i].responseData, 8);
          CAN.endPacket();
        }
      }
    }
}
/**
/*!
    @brief  Main CAN task that handles configuration and mode switching.
*/
void can_entry(void *pvParameters){
  queue_msg in_msg;
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

