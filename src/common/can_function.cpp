#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <CAN.h>
#include <Arduino.h>
#include <esp_intr_alloc.h>

#include "cfg/can_struct.h"
#include "cfg/host.h"
#include "common/can_function.h"
#include "common/http_function.h"


#define TX_GPIO_NUM   27
#define RX_GPIO_NUM   26

WebServer server(80);

extern int periodicCount;
extern int responseCount;
extern int Mode; 
extern int enable;
static uint32_t now = millis();

extern CanMessage periodicMessages[30];
extern CanResponse responseMessages[30];
extern CanResponseCheck responseCheck;

void setupAP(void){
    WiFi.softAP(ssid, password);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    CAN.setPins (RX_GPIO_NUM, TX_GPIO_NUM);
    if (!CAN.begin(500E3)) {
        Serial.println("Starting CAN failed!");
        while (1);
    }
    server.on("/mode", HTTP_GET, getMode);
    server.on("/mode", HTTP_POST, setMode);
    server.on("/period_cfg", HTTP_GET, get_periodic_cfg);
    server.on("/period_cfg", HTTP_POST, set_periodic_cfg);
    server.on("/req_res_cfg", HTTP_GET, get_req_res_cfg);
    server.on("/req_res_cfg", HTTP_POST, set_req_res_cfg);
    server.on("/enable", HTTP_GET, get_program_running);
    server.on("/enable", HTTP_POST, start_stop_program);
}

void mode1(void){
  if(enable == 1){
    for (int i = 0; i < periodicCount; i++) {
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
  if(enable == 1){
    responseCheck.id = strtoul("0x0C20A0A6", NULL, 16);
    hexStringToBytes("1234000000000000", responseCheck.data);
    for (int i = 0; i < responseCount; i++) {
        if (responseMessages[i].id == responseCheck.id && memcmp(responseMessages[i].data, responseCheck.data, 8) == 0) {
          CAN.beginExtendedPacket(responseMessages[i].responseId);
          CAN.write(responseMessages[i].responseData, 8);
          CAN.endPacket();
        }
    }
  }
}


