#include <WebServer.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <nvs_flash.h>
#include <nvs.h>

#include "common/http_function.h"
#include "common/nvs_function.h"
#include "cfg/can_cfg.h"
#include "cfg/host.h"

extern WebServer server;
extern CanMessage periodicMessages[30];
extern CanResponse responseMessages[30];

void getMode(void) {
  NVS_Read("Mode_S", &Mode_S);
  if (Mode_S == 0 || Mode_S == 1){
    String response = "{\"mode\":" + String(Mode_S) + "}";
    Serial.println(response);
    server.send(200, "application/json", response);
  }
  else{
    server.send(200, "application/json", "{\"error\":\"Error mode_num parameter\"}");
  }
}

void setMode(void) {
  NVS_Read("Enable_S", &Enable_S);
  if (Enable_S == 1) {
    String body = server.arg("plain");
    JsonDocument doc;
    deserializeJson(doc, body);

    int Mode = doc["mode_num"];
    Serial.println(Mode);
    if (Mode == 0 || Mode == 1){
      server.send(200, "application/json", "{\"Set mode\":\"ok\"}");
      NVS_Write("Mode_S", Mode);
    }
    else{
      server.send(200, "application/json", "{\"error\":\"Error mode_num parameter\"}");
    }
  }
  else {
    server.send(200, "application/json", "{\"error\":\"program doesn't work.\"}");
  }
}

void hexStringToBytes(String hexString, uint8_t *byteArray) {
  for (int i = 0; i < 8; i++) {
    byteArray[i] = strtoul(hexString.substring(i*2, i*2+2).c_str(), NULL, 16);
  }
}

String bytesToHexString(const uint8_t* byteArray, size_t length) {
    String hexString;
    for (size_t i = 0; i < length; i++) {
        if (byteArray[i] < 16) {
            hexString += "0";
        }
        hexString += String(byteArray[i], HEX);
    }
    return hexString;
}

void set_periodic_cfg(void) {
  NVS_Read("Enable_S", &Enable_S);
  NVS_Read("Mode_S", &Mode_S);
  if (Enable_S == 1) {
    String body = server.arg("plain");
    JsonDocument doc;
    deserializeJson(doc, body);
    Serial.println(body);
    if (Mode_S == 0) {
        Serial.println("Periodic mode in process");
        int periodicCount = doc["messages"].size();
        NVS_Write("periodic_S", periodicCount);
        for (int i = 0; i < periodicCount; i++) {
          periodicMessages[i].id = strtoul(doc["messages"][i]["id"], NULL, 16);
          String dataStr = doc["messages"][i]["data"];
          hexStringToBytes(dataStr, periodicMessages[i].data);
          periodicMessages[i].period = doc["messages"][i]["period"];
          periodicMessages[i].lastSent = 0;
          NVS_Write_Struct("peri_struct", &periodicMessages[i], sizeof(CanMessage));
        }
      server.send(200, "application/json", "{\"Periodic Mode \":\"ok\"}");
    }
  }
  else {
    server.send(200, "application/json", "{\"error\":\"program doesn't work.\"}");
  }
}

void set_req_res_cfg(void) {
  NVS_Read("Enable_S", &Enable_S);
  NVS_Read("Mode_S", &Mode_S);
  if (Enable_S == 1) {
    String body = server.arg("plain");
    JsonDocument doc;
    deserializeJson(doc, body);
    Serial.println(body);
    if (Mode_S == 1 ) {
        Serial.println("Request-Response mode in process");
        int responseCount = doc["messages"].size();
        NVS_Write("response_S", responseCount);
        for (int i = 0; i < responseCount; i++) {
          responseMessages[i].id = strtoul(doc["messages"][i]["id"], NULL, 16);
          String dataStr = doc["messages"][i]["data"];
          Serial.println(dataStr);
          hexStringToBytes(dataStr, responseMessages[i].data);
          responseMessages[i].responseId = strtoul(doc["messages"][i]["responseId"], NULL, 16);
          String responseDataStr = doc["messages"][i]["responseData"];
          hexStringToBytes(responseDataStr, responseMessages[i].responseData);
          NVS_Write_Struct("res_struct", &responseMessages[i], sizeof(CanResponse));
        }
        server.send(200, "application/json", "{\"Request-Response Mode 2\":\"ok\"}");
    }
  }
  else {
    server.send(200, "application/json", "{\"error\":\"program doesn't work.\"}");
  }
}

void get_periodic_cfg(void) {
  NVS_Read("Mode_S", &Mode_S);
  NVS_Read("periodic_S", &periodic_S);
  if (Mode_S == 0) {
    JsonDocument doc;
    JsonArray messages = doc["messages"].to<JsonArray>();
    for (int i = 0; i < periodic_S; i++) {
      if (NVS_Read_Struct("peri_struct", &periodicMessages[i], sizeof(CanMessage)) == ESP_OK) {
        JsonObject message = messages.add<JsonObject>();
        message["id"] = String(periodicMessages[i].id, HEX);
        message["data"] = bytesToHexString(periodicMessages[i].data, sizeof(periodicMessages[i].data));
        message["period"] = periodicMessages[i].period;
      }
    }

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);  
  }
  else {
    server.send(200, "application/json", "{\"error\":\"don't have periodic config.\"}");
  }

}

void get_req_res_cfg(void) {
  NVS_Read("Mode_S", &Mode_S);
  NVS_Read("response_S", &response_S);
  if (Mode_S == 1) {
    JsonDocument doc;
    JsonArray messages = doc["messages"].to<JsonArray>();

    for (int i = 0; i < response_S; i++) {
      if (NVS_Read_Struct("res_struct", &responseMessages[i], sizeof(CanResponse)) == ESP_OK) {
        JsonObject message = messages.add<JsonObject>();
        message["id"] = String(responseMessages[i].id, HEX);
        message["data"] = bytesToHexString(responseMessages[i].data, sizeof(responseMessages[i].data));
        message["responseId"] = String(responseMessages[i].responseId, HEX);
        message["responseData"] = bytesToHexString(responseMessages[i].responseData, sizeof(responseMessages[i].responseData));
      }
    }

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  }
  else {
    server.send(200, "application/json", "{\"error\":\"don't have request/response config.\"}");
  }
}

void start_stop_program(void){
  String body = server.arg("plain");
  JsonDocument doc;
  deserializeJson(doc, body);

  int enable = doc["enable"];
  Serial.println(enable);
  if (enable == 0 || enable == 1){
    NVS_Write("Enable_S", enable);
    server.send(200, "application/json", "{\"Set enable\":\"ok\"}");
  }
  else{
    server.send(200, "application/json", "{\"error\":\"Error enable parameter\"}");
  }
}

void get_program_running(void){
  NVS_Read("Enable_S", &Enable_S);
  if (Enable_S == 0 || Enable_S == 1){
    String response = "{\"enable\":" + String(Enable_S) + "}";
    Serial.println(response);
    server.send(200, "application/json", response);
  }
  else{
    server.send(200, "application/json", "{\"error\":\"Error enable parameter\"}");
  }
}
