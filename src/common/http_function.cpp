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
extern QueueHandle_t can_queue;
setUp_cfg setup_cfg;
http_periodic http_periodic_messages[30];
http_response http_response_messages[30];
queue_msg out_msg;

static char key[20]; 
static int rc;
char errorMessage[100];
/**
/*!
  @brief Gets the current mode (either 0 or 1) from NVS and sends it as a JSON response.
  @returns JSON response with the current mode if valid, otherwise an error message to user via http.
*/
void get_mode(void) {
  esp_err_t err = NVS_Read("mode_s", &mode_s);
  if(err == ESP_OK){
    if (mode_s == 0 || mode_s == 1){
      String response = "{\"mode\":" + String(mode_s) + "}";
      Serial.println(response);
      server.send(200, "application/json", response);
    }
    else{
      server.send(200, "application/json", "{\"error\":\"Error mode_num parameter\"}");
    }
  }
  else {
    snprintf(errorMessage, sizeof(errorMessage), "{\"error from NVS ot read mode parameter\":\"%s\"}", esp_err_to_name(err));
    server.send(200, "application/json", errorMessage);
  }
}
/**
/*!
  @brief Sets the mode (0 or 1) based on the JSON request if configuration is enabled.
  @returns JSON response indicating success or an error message to user via http.
*/
void set_mode(void) {
  if (setup_cfg.enable_cfg == 1) {
    String body = server.arg("plain");
    JsonDocument doc;
    deserializeJson(doc, body);

    int Mode = doc["mode_num"];
    if (Mode == 0 || Mode == 1){
      esp_err_t err = NVS_Write("mode_s", Mode);
      if(err == ESP_OK){
        setup_cfg.mode_cfg = Mode;
        server.send(200, "application/json", "{\"Set mode\":\"ok\"}");
      }
      else {
      snprintf(errorMessage, sizeof(errorMessage), "{\"error from NVS ot write mode parameter\":\"%s\"}", esp_err_to_name(err));
      server.send(200, "application/json", errorMessage);
      }
    }
    else{
      server.send(200, "application/json", "{\"error\":\"Error mode_num parameter\"}");
    }
  }
  else {
    server.send(200, "application/json", "{\"error\":\"program doesn't work.\"}");
  }
}
/**
/*!
  @brief Converts a hexadecimal string to a byte array.
  @param hexString The hexadecimal string to convert.
  @param byteArray The byte array to store the converted bytes.
*/
void hexStringToBytes(String hexString, uint8_t *byteArray) {
  for (int i = 0; i < 8; i++) {
    byteArray[i] = strtoul(hexString.substring(i*2, i*2+2).c_str(), NULL, 16);
  }
}
/**
/*!
  @brief Converts a byte array to a hexadecimal string.
  @param byteArray The byte array to convert.
  @param length The length of the byte array.
  @returns A hexadecimal string representation of the byte array.
*/
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
/**
/*!
  @brief Sets periodic mode configuration based on the JSON request if configuration is enabled and mode is 0.
  @returns JSON response indicating success or an error message to user via http.
*/
void set_periodic_cfg(void) {
  if (setup_cfg.enable_cfg == 1 && setup_cfg.mode_cfg == 0) {
    String body = server.arg("plain");
    JsonDocument doc;
    deserializeJson(doc, body);
    Serial.println(body);
    Serial.println("Periodic mode in process");
    int periodic_count = doc["messages"].size();
    setup_cfg.periodic_count_cfg = periodic_count;
    esp_err_t err_periodic_count = NVS_Write("periodic_s", periodic_count);
    if(err_periodic_count == ESP_OK){
      for (int i = 0; i < periodic_count; i++) {
        http_periodic_messages[i].id = strtoul(doc["messages"][i]["id"], NULL, 16);
        std::string dataStr = doc["messages"][i]["data"];
        dataStr = dataStr.substr(2);
        hexStringToBytes(dataStr.c_str(), http_periodic_messages[i].data);
        http_periodic_messages[i].period = doc["messages"][i]["period"];
        http_periodic_messages[i].lastSent = 0;
        sprintf(key, "peri_struct%d", i + 1);
        esp_err_t err_struct = NVS_Write_Struct(key, &http_periodic_messages[i], sizeof(http_periodic));
        if(err_struct != ESP_OK){
          snprintf(errorMessage, sizeof(errorMessage), "{\"error from NVS ot write periodic mode config\":\"%s\"}", esp_err_to_name(err_struct));
          server.send(200, "application/json", errorMessage);
        }
      }
      server.send(200, "application/json", "{\"Periodic Mode \":\"ok\"}");
    }
    else {
      snprintf(errorMessage, sizeof(errorMessage), "{\"error from NVS ot write response count parameter\":\"%s\"}", esp_err_to_name(err_periodic_count));
      server.send(200, "application/json", errorMessage);
    }
  }
  else {
    server.send(200, "application/json", "{\"error\":\"program doesn't work.\"}");
  }
}
/**
/*!
  @brief Sets request-response mode configuration based on the JSON request if configuration is enabled and mode is 1.
  @returns JSON response indicating success or an error message to user via http.
*/
void set_req_res_cfg(void) {
  if (setup_cfg.enable_cfg == 1 && setup_cfg.mode_cfg == 1 ) {
    String body = server.arg("plain");
    JsonDocument doc;
    deserializeJson(doc, body);
    Serial.println(body);
    Serial.println("Request-Response mode in process");
    int response_count = doc["messages"].size();
    setup_cfg.response_count_cfg = response_count;
    esp_err_t err_response_count = NVS_Write("response_s", response_count);
    if(err_response_count == ESP_OK){
      for (int i = 0; i < response_count; i++) {
        http_response_messages[i].id = strtoul(doc["messages"][i]["id"], NULL, 16);
        std::string dataStr = doc["messages"][i]["data"];
        dataStr = dataStr.substr(2);
        hexStringToBytes(dataStr.c_str(), http_response_messages[i].data);
        http_response_messages[i].responseId = strtoul(doc["messages"][i]["responseId"], NULL, 16);
        std::string responseDataStr = doc["messages"][i]["responseData"];
        responseDataStr = responseDataStr.substr(2);
        hexStringToBytes(responseDataStr.c_str(), http_response_messages[i].responseData);
        sprintf(key, "res_struct%d", i + 1);
        esp_err_t err_struct = NVS_Write_Struct(key, &http_response_messages[i], sizeof(http_response));
        if(err_struct != ESP_OK){
          snprintf(errorMessage, sizeof(errorMessage), "{\"error from NVS ot write request-response mode config\":\"%s\"}", esp_err_to_name(err_struct));
          server.send(200, "application/json", errorMessage);
        }
      }
      server.send(200, "application/json", "{\"Request-Response Mode 2\":\"ok\"}");
    }
    else {
      snprintf(errorMessage, sizeof(errorMessage), "{\"error from NVS ot write response count parameter\":\"%s\"}", esp_err_to_name(err_response_count));
      server.send(200, "application/json", errorMessage);
    }
  }
  else {
    server.send(200, "application/json", "{\"error\":\"program doesn't work.\"}");
  }
}
/**
/*!
  @brief Retrieves the periodic mode configuration if mode is 0.
  @returns JSON response with the periodic mode configuration or an error message to user via http.
*/
void get_periodic_cfg(void) {
  esp_err_t err_mode = NVS_Read("mode_s", &mode_s);
  if(err_mode == ESP_OK){
    esp_err_t err_periodic = NVS_Read("periodic_s", &periodic_s);
    if(err_periodic == ESP_OK){
      if (mode_s == 0) {
        JsonDocument doc;
        JsonArray messages = doc["messages"].to<JsonArray>();
        for (int i = 0; i < periodic_s; i++) {
          sprintf(key, "peri_struct%d", i + 1);
          esp_err_t err_struct = NVS_Read_Struct(key, &http_periodic_messages[i], sizeof(http_periodic));
          if(err_struct == ESP_OK){
            JsonObject message = messages.add<JsonObject>();
            message["id"] = String(http_periodic_messages[i].id, HEX);
            message["data"] = bytesToHexString(http_periodic_messages[i].data, sizeof(http_periodic_messages[i].data));
            message["period"] = http_periodic_messages[i].period;
          }
          else {
            snprintf(errorMessage, sizeof(errorMessage), "{\"error from NVS ot read periodic mode config\":\"%s\"}", esp_err_to_name(err_struct));
            server.send(200, "application/json", errorMessage);
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
    else {
      snprintf(errorMessage, sizeof(errorMessage), "{\"error from NVS ot read periodic count parameter\":\"%s\"}", esp_err_to_name(err_periodic));
      server.send(200, "application/json", errorMessage);
    }
  }
  else {
    snprintf(errorMessage, sizeof(errorMessage), "{\"error from NVS ot read mode parameter\":\"%s\"}", esp_err_to_name(err_mode));
    server.send(200, "application/json", errorMessage);
  }

}
/**
/*!
  @brief Retrieves the request-response mode configuration if mode is 1.
  @returns JSON response with the request-response mode configuration or an error message to user via http.
*/
void get_req_res_cfg(void) {
  esp_err_t err_mode = NVS_Read("mode_s", &mode_s);
  if(err_mode == ESP_OK){
    esp_err_t err_response = NVS_Read("response_s", &response_s);
    if (mode_s == 1) {
      if(err_response == ESP_OK){
        JsonDocument doc;
        JsonArray messages = doc["messages"].to<JsonArray>();
        for (int i = 0; i < response_s; i++) {
          sprintf(key, "res_struct%d", i + 1);
          esp_err_t err_struct = NVS_Read_Struct(key, &http_response_messages[i], sizeof(http_response));
          if(err_struct == ESP_OK){
            JsonObject message = messages.add<JsonObject>();
            message["id"] = String(http_response_messages[i].id, HEX);
            message["data"] = bytesToHexString(http_response_messages[i].data, sizeof(http_response_messages[i].data));
            message["responseId"] = String(http_response_messages[i].responseId, HEX);
            message["responseData"] = bytesToHexString(http_response_messages[i].responseData, sizeof(http_response_messages[i].responseData));
          }
          else {
            snprintf(errorMessage, sizeof(errorMessage), "{\"error from NVS ot read request-response mode config\":\"%s\"}", esp_err_to_name(err_struct));
            server.send(200, "application/json", errorMessage);
          }
        }

        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
      }
      else {
        server.send(200, "application/json", "{\"error\":\"don't have request-response config.\"}");
      }
    }
    else{
      snprintf(errorMessage, sizeof(errorMessage), "{\"error from NVS ot read response count parameter\":\"%s\"}", esp_err_to_name(err_response));
      server.send(200, "application/json", errorMessage);
    }
  }
  else {
    snprintf(errorMessage, sizeof(errorMessage), "{\"error from NVS ot read mode parameter\":\"%s\"}", esp_err_to_name(err_mode));
    server.send(200, "application/json", errorMessage);
  }
}
/**
/*!
  @brief Starts or stops the program based on the JSON request.
  @returns JSON response indicating success or an error message to user via http.
*/
void start_stop_program(void){
  String body = server.arg("plain");
  JsonDocument doc;
  deserializeJson(doc, body);

  int enable = doc["enable"];
  if (enable == 0 || enable == 1){
    esp_err_t err =  NVS_Write("enable_s", enable);
    if(err == ESP_OK){
      setup_cfg.enable_cfg = enable;
      if (enable == 0){
        out_msg.check_stop = true;
        rc = xQueueSend(can_queue, &out_msg, 100);
        if (rc == pdTRUE) {
          Serial.println("TSK_HTTP:Report data OK");
        }
        else {
          Serial.printf("TSK_HTTP:Report data FAIL %d\r\n", rc);
        }
      }
      out_msg.check_stop = false;
      server.send(200, "application/json", "{\"Set enable\":\"ok\"}");
    }
    else {
      snprintf(errorMessage, sizeof(errorMessage), "{\"error from NVS ot write enable parameter\":\"%s\"}", esp_err_to_name(err));
      server.send(200, "application/json", errorMessage);
    }
  }
  else{
    server.send(200, "application/json", "{\"error\":\"Error enable parameter\"}");
  }
}
/**
/*!
  @brief Retrieves the current program running state (either 0 or 1) from NVS and sends it as a JSON response.
  @returns JSON response with the current program running state if valid, otherwise an error message to user via http.
*/
void get_program_running(void){
  esp_err_t err = NVS_Read("enable_s", &enable_s);
  if(err == ESP_OK){
    if (enable_s == 0 || enable_s == 1){
      String response = "{\"enable\":" + String(enable_s) + "}";
      Serial.println(response);
      server.send(200, "application/json", response);
    }
    else{
      server.send(200, "application/json", "{\"error\":\"Error enable parameter\"}");
    }
  }
  else {
    snprintf(errorMessage, sizeof(errorMessage), "{\"error from NVS ot read enable parameter\":\"%s\"}", esp_err_to_name(err));
    server.send(200, "application/json", errorMessage);
  }
}
/**
/*!
  @brief Retrieves the current bitrate from NVS and sends it as a JSON response.
  @returns JSON response with the current bitrate if valid, otherwise an error message to user via http.
*/
void get_bitrate(void) {
  esp_err_t err = NVS_Read("bit_s", &bit_s);
  if(err == ESP_OK){
    if (bit_s != 0){
      String response = "{\"bitrate\":" + String(bit_s) + "}";
      Serial.println(response);
      server.send(200, "application/json", response);
    }
    else{
      server.send(200, "application/json", "{\"error\":\"Error bitrates parameter\"}");
    }
  }
  else {
    snprintf(errorMessage, sizeof(errorMessage), "{\"error from NVS ot read bitrate parameter\":\"%s\"}", esp_err_to_name(err));
    server.send(200, "application/json", errorMessage);
  }
}
/**
/*!
  @brief Sets the bitrate of CAN protocol based on the JSON request if configuration is enabled.
  @returns JSON response indicating success or an error message to user via http.
*/
void set_bitrate(void) {
  if (setup_cfg.enable_cfg == 1) {
    String body = server.arg("plain");
    JsonDocument doc;
    deserializeJson(doc, body);

    int Bit = doc["bitrate"];
    std::vector<int> vec = {25000, 50000, 100000, 125000, 250000, 500000, 800000, 1000000};
    if (std::find(vec.begin(), vec.end(), Bit) != vec.end()){
      double send_bit = Bit;
      esp_err_t err =  NVS_Write("bit_s", send_bit);
      if(err == ESP_OK){
        setup_cfg.bit_cfg = send_bit;
        server.send(200, "application/json", "{\"Set bitrate\":\"ok\"}");
        out_msg.check_change = true;
        rc = xQueueSend(can_queue, &out_msg, 100);
        if (rc == pdTRUE) {
          Serial.println("TSK_HTTP:Report data OK");
        }
        else {
          Serial.printf("TSK_HTTP:Report data FAIL %d\r\n", rc);
        }
        out_msg.check_change = false;
      }
      else {
        snprintf(errorMessage, sizeof(errorMessage), "{\"error from NVS ot write bitrate parameter\":\"%s\"}", esp_err_to_name(err));
        server.send(200, "application/json", errorMessage);
      }
    }
    else{
      server.send(200, "application/json", "{\"error\":\"Error bitrates parameter\"}");
    }
  }
  else {
    server.send(200, "application/json", "{\"error\":\"program doesn't work.\"}");
  }
}
/**
/*!
  @brief Sets up HTTP server routes and starts the server to handle client requests.
*/
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