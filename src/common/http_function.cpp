#include <WebServer.h>
#include <ArduinoJson.h>
#include <Arduino.h>

#include "cfg/can_struct.h"
#include "cfg/host.h"

extern WebServer server;
extern CanMessage periodicMessages[30];
extern CanResponse responseMessages[30];
extern CanResponseCheck responseCheck;
static int periodicCount = 0;
static int responseCount = 0;
static int Mode = 3; 
static int enable = 3;

void hexStringToBytes(String hexString, uint8_t *byteArray);
String bytesToHexString(const uint8_t* byteArray, size_t length);

void getMode(void) {
  if (Mode == 0 || Mode == 1){
    String response = "{\"mode\":" + String(Mode) + "}";
    server.send(200, "application/json", response);
  }
  else{
    server.send(200, "application/json", "{\"error\":\"Missing mode_num parameter\"}");
  }
}

void setMode(void) {
  if (server.hasArg("mode_num")) {
    Mode = server.arg("mode_num").toInt();
    server.send(200, "application/json", "{\"Set mode\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing mode_num parameter\"}");
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
    String body = server.arg("config");
    JsonDocument doc;
    deserializeJson(doc, body);

    if (Mode == 0 && enable == 1) {
        Serial.println("Periodic mode in process");
        periodicCount = doc["messages"].size();
        for (int i = 0; i < periodicCount; i++) {
          periodicMessages[i].id = strtoul(doc["messages"][i]["id"], NULL, 16);
          String dataStr = doc["messages"][i]["data"];
          hexStringToBytes(dataStr, periodicMessages[i].data);
          periodicMessages[i].period = doc["messages"][i]["period"];
          periodicMessages[i].lastSent = 0;
        }
    }
  
  server.send(200, "application/json", "{\"Periodic Mode \":\"ok\"}");
}

void set_req_res_cfg(void) {
    String body = server.arg("config");
    JsonDocument doc;
    deserializeJson(doc, body);

    if (Mode == 1 && enable == 1) {
        Serial.println("Request-Response mode in process");
        responseCount = doc["messages"].size();
        for (int i = 0; i < responseCount; i++) {
          responseMessages[i].id = strtoul(doc["messages"][i]["id"], NULL, 16);
          String dataStr = doc["messages"][i]["data"];
          Serial.println(dataStr);
          hexStringToBytes(dataStr, responseMessages[i].data);
          responseMessages[i].responseId = strtoul(doc["messages"][i]["responseId"], NULL, 16);
          String responseDataStr = doc["messages"][i]["responseData"];
          hexStringToBytes(responseDataStr, responseMessages[i].responseData);
        }
    }

  server.send(200, "application/json", "{\"Request-Response Mode 2\":\"ok\"}");
}

void get_periodic_cfg(void) {
  JsonDocument doc;
  JsonArray messages = doc["messages"].to<JsonArray>();

  for (int i = 0; i < periodicCount; i++) {
      JsonObject message = messages.createNestedObject();
      message["id"] = String(periodicMessages[i].id, HEX);
      message["data"] = bytesToHexString(periodicMessages[i].data, sizeof(periodicMessages[i].data));
      message["period"] = periodicMessages[i].period;
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
}

void get_req_res_cfg(void) {
  JsonDocument doc;
  JsonArray messages = doc["messages"].to<JsonArray>();

  for (int i = 0; i < responseCount; i++) {
      JsonObject message = messages.createNestedObject();
      message["id"] = String(responseMessages[i].id, HEX);
      message["data"] = bytesToHexString(responseMessages[i].data, sizeof(responseMessages[i].data));
      message["responseId"] = String(responseMessages[i].responseId, HEX);
      message["responseData"] = bytesToHexString(responseMessages[i].responseData, sizeof(responseMessages[i].responseData));
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void start_stop_program(void){
  if (server.hasArg("enable")) {
    Mode = server.arg("enable").toInt();
    server.send(200, "application/json", "{\"Set enable\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing enable parameter\"}");
  }
}

void get_program_running(void){
  if (enable == 0 || enable == 1){
    String response = "{\"enable\":" + String(enable) + "}";
    server.send(200, "application/json", response);
  }
  else{
    server.send(200, "application/json", "{\"error\":\"Missing enable parameter\"}");
  }
}