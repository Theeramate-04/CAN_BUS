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
static int Mode = 0; 

void getMode(void) {
  String response = "{\"mode\":" + String(Mode) + "}";
  server.send(200, "application/json", response);
}

void setMode(void) {
  if (server.hasArg("mode_num")) {
    Mode = server.arg("mode_num").toInt();
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing mode_num parameter\"}");
  }
}

void hexStringToBytes(String hexString, uint8_t *byteArray) {
  for (int i = 0; i < 8; i++) {
    byteArray[i] = strtoul(hexString.substring(i*2, i*2+2).c_str(), NULL, 16);
  }
}

void set_periodic_cfg(void) {
  String body = server.arg("plain");
  JsonDocument doc;
  deserializeJson(doc, body);

  if (doc.containsKey("mode")) {
    Mode = doc["mode"];
    if (Mode == 1) {
        Serial.println("Mode 1 in process");
        periodicCount = doc["messages"].size();
        for (int i = 0; i < periodicCount; i++) {
          periodicMessages[i].id = strtoul(doc["messages"][i]["id"], NULL, 16);
          String dataStr = doc["messages"][i]["data"];
          hexStringToBytes(dataStr, periodicMessages[i].data);
          periodicMessages[i].period = doc["messages"][i]["period"];
          periodicMessages[i].lastSent = 0;
        }
    }
  }
  server.send(200, "application/json", "{\"Mode 1\":\"ok\"}");
}

void set_req_res_cfg(void) {
  String body = server.arg("plain");
  JsonDocument doc;
  deserializeJson(doc, body);

  if (doc.containsKey("mode")) {
    Mode = doc["mode"];
    if (Mode == 2) {
        Serial.println("Mode 2 in process");
        responseCount = doc["responses"].size();
        for (int i = 0; i < responseCount; i++) {
          responseMessages[i].id = strtoul(doc["responses"][i]["id"], NULL, 16);
          String dataStr = doc["responses"][i]["data"];
          Serial.println(dataStr);
          hexStringToBytes(dataStr, responseMessages[i].data);
          responseMessages[i].responseId = strtoul(doc["responses"][i]["responseId"], NULL, 16);
          String responseDataStr = doc["responses"][i]["responseData"];
          hexStringToBytes(responseDataStr, responseMessages[i].responseData);
        }
    }
  }

  server.send(200, "application/json", "{\"Mode 2\":\"ok\"}");
}

void get_periodic_cfg(void) {
  String body = server.arg("plain");
  JsonDocument doc;
  deserializeJson(doc, body);

  if (doc.containsKey("mode")) {
    Mode = doc["mode"];
    if (Mode == 1) {
        Serial.println("Mode 1 in process");
        periodicCount = doc["messages"].size();
        for (int i = 0; i < periodicCount; i++) {
          periodicMessages[i].id = strtoul(doc["messages"][i]["id"], NULL, 16);
          String dataStr = doc["messages"][i]["data"];
          hexStringToBytes(dataStr, periodicMessages[i].data);
          periodicMessages[i].period = doc["messages"][i]["period"];
          periodicMessages[i].lastSent = 0;
        }
    }
  }
  server.send(200, "application/json", "{\"Mode 1\":\"ok\"}");
}

void get_req_res_cfg(void) {
  String body = server.arg("plain");
  JsonDocument doc;
  deserializeJson(doc, body);

  if (doc.containsKey("mode")) {
    Mode = doc["mode"];
    if (Mode == 2) {
        Serial.println("Mode 2 in process");
        responseCount = doc["responses"].size();
        for (int i = 0; i < responseCount; i++) {
          responseMessages[i].id = strtoul(doc["responses"][i]["id"], NULL, 16);
          String dataStr = doc["responses"][i]["data"];
          Serial.println(dataStr);
          hexStringToBytes(dataStr, responseMessages[i].data);
          responseMessages[i].responseId = strtoul(doc["responses"][i]["responseId"], NULL, 16);
          String responseDataStr = doc["responses"][i]["responseData"];
          hexStringToBytes(responseDataStr, responseMessages[i].responseData);
        }
    }
  }

  server.send(200, "application/json", "{\"Mode 2\":\"ok\"}");
}