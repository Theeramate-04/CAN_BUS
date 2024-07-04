#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <CAN.h>
#include <Arduino.h>
#include <esp_intr_alloc.h>

#define TX_GPIO_NUM   27
#define RX_GPIO_NUM   26

const char* ssid = "ESP32-CAN-AP";
const char* password = "12345678";

WebServer server(80);

struct CanMessage {
  uint32_t id;
  uint8_t data[8];
  uint32_t period;
  uint32_t lastSent;
};
CanMessage periodicMessages[30];
int periodicCount = 0;

struct CanResponse {
  uint32_t id;
  uint8_t data[8];
  uint32_t responseId;
  uint8_t responseData[8];
};

CanResponse responseMessages[30];
int responseCount = 0;

struct CanResponseCheck {
  uint32_t id;
  uint8_t data[8];
};

CanResponseCheck responseCheck;

int Mode = 0; 

void hexStringToBytes(String hexString, uint8_t *byteArray) {
  for (int i = 0; i < 8; i++) {
    byteArray[i] = strtoul(hexString.substring(i*2, i*2+2).c_str(), NULL, 16);
  }
}

void handleConfig() {
  String body = server.arg("plain");
  JsonDocument doc;
  deserializeJson(doc, body);

  if (doc.containsKey("mode")) {
    Mode = doc["mode"];
    switch (Mode) {
      case 1: 
        Serial.println("Mode 1 in process");
        periodicCount = doc["messages"].size();
        for (int i = 0; i < periodicCount; i++) {
          periodicMessages[i].id = strtoul(doc["messages"][i]["id"], NULL, 16);
          String dataStr = doc["messages"][i]["data"];
          hexStringToBytes(dataStr, periodicMessages[i].data);
          periodicMessages[i].period = doc["messages"][i]["period"];
          periodicMessages[i].lastSent = 0;
        }
        break;

      case 2:
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
        break;

      default:
      break;
    }
  }

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  CAN.setPins (RX_GPIO_NUM, TX_GPIO_NUM);
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }

  server.on("/setConfig", HTTP_POST, handleConfig);

  server.begin();
}

void loop() {
  server.handleClient();
  uint32_t now = millis();
  switch (Mode) {
    case 1:
      for (int i = 0; i < periodicCount; i++) {
        if (now - periodicMessages[i].lastSent >= periodicMessages[i].period) {
          CAN.beginExtendedPacket(periodicMessages[i].id);
          CAN.write(periodicMessages[i].data, 8);
          CAN.endPacket();
          periodicMessages[i].lastSent = now;
        }
      }
      break;
    
    case 2:
      responseCheck.id = strtoul("0x0C20A0A6", NULL, 16);
      hexStringToBytes("1234000000000000", responseCheck.data);
      for (int i = 0; i < responseCount; i++) {
        if (responseMessages[i].id == responseCheck.id && memcmp(responseMessages[i].data, responseCheck.data, 8) == 0) {
          CAN.beginExtendedPacket(responseMessages[i].responseId);
          CAN.write(responseMessages[i].responseData, 8);
          CAN.endPacket();
        }
      }
      break;

    default:
      break;
  }
}