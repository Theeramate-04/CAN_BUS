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
extern QueueHandle_t httpQueue;
setUp_cfg setup_cfg;
CanMessage periodicMessages[30];
CanResponse responseMessages[30];
Queue_msg out_msg;

static char key[20]; 
static int rc;

void get_mode(void) {
  NVS_Read("mode_s", &mode_s);
  if (mode_s == 0 || mode_s == 1){
    String response = "{\"mode\":" + String(mode_s) + "}";
    Serial.println(response);
    server.send(200, "application/json", response);
  }
  else{
    server.send(200, "application/json", "{\"error\":\"Error mode_num parameter\"}");
  }
}

void set_mode(void) {
  if (setup_cfg.enable_cfg == 1) {
    String body = server.arg("plain");
    JsonDocument doc;
    deserializeJson(doc, body);

    int Mode = doc["mode_num"];
    if (Mode == 0 || Mode == 1){
      NVS_Write("mode_s", Mode);
      setup_cfg.mode_cfg = Mode;
      server.send(200, "application/json", "{\"Set mode\":\"ok\"}");
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
  if (setup_cfg.enable_cfg == 1 && setup_cfg.mode_cfg == 0) {
    String body = server.arg("plain");
    JsonDocument doc;
    deserializeJson(doc, body);
    Serial.println(body);
    Serial.println("Periodic mode in process");
    int periodicCount = doc["messages"].size();
    setup_cfg.periodic_cfg = periodicCount;
    NVS_Write("periodic_s", periodicCount);
    for (int i = 0; i < periodicCount; i++) {
      periodicMessages[i].id = strtoul(doc["messages"][i]["id"], NULL, 16);
      std::string dataStr = doc["messages"][i]["data"];
      dataStr = dataStr.substr(2);
      hexStringToBytes(dataStr.c_str(), periodicMessages[i].data);
      periodicMessages[i].period = doc["messages"][i]["period"];
      periodicMessages[i].lastSent = 0;
      sprintf(key, "peri_struct%d", i + 1);
      NVS_Write_Struct(key, &periodicMessages[i], sizeof(CanMessage));
    }
    server.send(200, "application/json", "{\"Periodic Mode \":\"ok\"}");
  }
  else {
    server.send(200, "application/json", "{\"error\":\"program doesn't work.\"}");
  }
}

void set_req_res_cfg(void) {
  if (setup_cfg.enable_cfg == 1 && setup_cfg.mode_cfg == 1 ) {
    String body = server.arg("plain");
    JsonDocument doc;
    deserializeJson(doc, body);
    Serial.println(body);
    Serial.println("Request-Response mode in process");
    int responseCount = doc["messages"].size();
    setup_cfg.response_cfg = responseCount;
    NVS_Write("response_s", responseCount);
    for (int i = 0; i < responseCount; i++) {
      responseMessages[i].id = strtoul(doc["messages"][i]["id"], NULL, 16);
      std::string dataStr = doc["messages"][i]["data"];
      dataStr = dataStr.substr(2);
      hexStringToBytes(dataStr.c_str(), responseMessages[i].data);
      responseMessages[i].responseId = strtoul(doc["messages"][i]["responseId"], NULL, 16);
      std::string responseDataStr = doc["messages"][i]["responseData"];
      responseDataStr = responseDataStr.substr(2);
      hexStringToBytes(responseDataStr.c_str(), responseMessages[i].responseData);
      sprintf(key, "res_struct%d", i + 1);
      NVS_Write_Struct(key, &responseMessages[i], sizeof(CanResponse));
    }
    server.send(200, "application/json", "{\"Request-Response Mode 2\":\"ok\"}");
    
  }
  else {
    server.send(200, "application/json", "{\"error\":\"program doesn't work.\"}");
  }
}

void get_periodic_cfg(void) {
  NVS_Read("mode_s", &mode_s);
  NVS_Read("periodic_s", &periodic_s);
  if (mode_s == 0) {
    JsonDocument doc;
    JsonArray messages = doc["messages"].to<JsonArray>();
    for (int i = 0; i < periodic_s; i++) {
      sprintf(key, "peri_struct%d", i + 1);
      if (NVS_Read_Struct(key, &periodicMessages[i], sizeof(CanMessage)) == ESP_OK) {
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
  NVS_Read("mode_S", &mode_s);
  NVS_Read("response_s", &response_s);
  if (mode_s == 1) {
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
  if (enable == 0 || enable == 1){
    NVS_Write("enable_s", enable);
    setup_cfg.enable_cfg = enable;
    server.send(200, "application/json", "{\"Set enable\":\"ok\"}");
  }
  else{
    server.send(200, "application/json", "{\"error\":\"Error enable parameter\"}");
  }
}

void get_program_running(void){
  NVS_Read("enable_s", &enable_s);
  if (enable_s == 0 || enable_s == 1){
    String response = "{\"enable\":" + String(enable_s) + "}";
    Serial.println(response);
    server.send(200, "application/json", response);
  }
  else{
    server.send(200, "application/json", "{\"error\":\"Error enable parameter\"}");
  }
}

void get_bitrate(void) {
  NVS_Read("bit_s", &bit_s);
  if (bit_s != 0){
    String response = "{\"bitrate\":" + String(bit_s) + "}";
    Serial.println(response);
    server.send(200, "application/json", response);
  }
  else{
    server.send(200, "application/json", "{\"error\":\"Error bitrates parameter\"}");
  }
}

void set_bitrate(void) {
  if (setup_cfg.enable_cfg == 1) {
    String body = server.arg("plain");
    JsonDocument doc;
    deserializeJson(doc, body);

    int Bit = doc["bitrate"];
    Serial.println(Bit);
    std::vector<int> vec = {10000, 20000, 40000, 50000, 80000, 100000, 125000, 200000, 250000, 500000, 1000000};
    if (std::find(vec.begin(), vec.end(), Bit) != vec.end()){
      double send_bit = Bit;
      NVS_Write("bit_s", send_bit);
      setup_cfg.bit_cfg = send_bit;
      server.send(200, "application/json", "{\"Set bitrate\":\"ok\"}");
      out_msg.check_change = true;
      rc = xQueueSend(httpQueue, &out_msg, 100);
      if (rc == pdTRUE) {
        Serial.println("TSK_HTTP:Report data OK");
      }
      else {
        Serial.printf("TSK_HTTP:Report data FAIL %d\r\n", rc);
      }
      out_msg.check_change = false;

    }
    else{
      server.send(200, "application/json", "{\"error\":\"Error bitrates parameter\"}");
    }
  }
  else {
    server.send(200, "application/json", "{\"error\":\"program doesn't work.\"}");
  }
}

void http_entry(void *pvParameters){
  server.on("/enable", HTTP_GET, get_program_running);
  server.on("/enable", HTTP_POST, start_stop_program);
  server.on("/mode", HTTP_GET, get_mode);
  server.on("/mode", HTTP_POST, set_mode);
  server.on("/period_cfg", HTTP_GET, get_periodic_cfg);
  server.on("/period_cfg", HTTP_POST, set_periodic_cfg);
  server.on("/req_res_cfg", HTTP_GET, get_req_res_cfg);
  server.on("/req_res_cfg", HTTP_POST, set_req_res_cfg);
  server.on("/bitrate", HTTP_GET, get_bitrate);
  server.on("/bitrate", HTTP_POST, set_bitrate);
  server.begin();
  while (1){
    server.handleClient();
  }
}