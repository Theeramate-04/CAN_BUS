#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "driver/gpio.h"
#include "driver/twai.h"
#include <Arduino.h>
#include <esp_intr_alloc.h>
#include <nvs_flash.h>
#include <nvs.h>

#include "cfg/can_cfg.h"
#include "cfg/host.h"
#include "common/can_function.h"
#include "common/http_function.h"
#include "common/nvs_function.h"

extern http_periodic http_periodic_messages[30];
extern http_response http_response_messages[30];
extern QueueHandle_t can_queue;
can_periodic can_periodic_messages[30];
can_response can_response_messages[30];
mode_event mode_evt;
can_response_check responseCheck;
setUp_cfg_new setup_cfg_new;
twai_message_t can_msg;

/**
/*!
    @brief  Configures CAN settings such as pins and bitrate.
*/
void setup_can(void){
  twai_general_config_t g_config = { 
    .mode = TWAI_MODE_NORMAL,
    .tx_io = GPIO_NUM_27, //set up TX pin as 27
    .rx_io = GPIO_NUM_26, //set up RX pin as 26
    .clkout_io = ((gpio_num_t) - 1),
    .bus_off_io = ((gpio_num_t) - 1),
    .tx_queue_len = 5,
    .rx_queue_len = 5,
    .alerts_enabled = TWAI_ALERT_NONE,
    .clkout_divider = 0
  };
  twai_timing_config_t t_config;
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  
  switch (setup_cfg_new.bit_cfg) {
      case 1000000:
        t_config = TWAI_TIMING_CONFIG_1MBITS();
        break;
      case 800000:
        t_config = TWAI_TIMING_CONFIG_800KBITS();
        break;
      case 500000:
        t_config = TWAI_TIMING_CONFIG_500KBITS();
        break;
      case 250000:
        t_config = TWAI_TIMING_CONFIG_250KBITS();
        break;
      case 125000:
        t_config = TWAI_TIMING_CONFIG_125KBITS();
        break;
      case 100000:
        t_config = TWAI_TIMING_CONFIG_100KBITS();
        break;
      case 50000:
        t_config = TWAI_TIMING_CONFIG_50KBITS();
        break;
      case 25000:
        t_config = TWAI_TIMING_CONFIG_25KBITS();
        break;
      default:
        break;
  }

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    Serial.println("Failed to setup CAN");
    return;
  }

  if (twai_start() != ESP_OK) {
    Serial.println("Failed to start CAN");
    return;
  }
}
/**
/*!
    @brief  Sets up CAN configuration from NVS storage.
*/
void setup_can_cfg(void){
  NVS_Read("enable_s", &enable_s);
  NVS_Read("mode_s", &mode_s);
  NVS_Read("bit_s", &bit_s);
  setup_cfg_new.mode_cfg = mode_s;
  setup_cfg_new.enable_cfg = enable_s;
  setup_cfg_new.bit_cfg = bit_s;
  char key[20];
  if (mode_s == 0 && enable_s == 1) {
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
  else if (mode_s == 1 && enable_s == 1) {
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
  else{
    mode_evt = mode_event::STOP_MODE;
  }
}
/**
/*!
    @brief  Handles CAN message reception and processes received messages.
*/
void on_receive(void) {
  if (twai_receive(&can_msg, pdMS_TO_TICKS(10000)) != ESP_OK) {
    return;
  }
  responseCheck.id = can_msg.identifier;
  memcpy(responseCheck.data, can_msg.data, sizeof(can_msg.data));
  Serial.printf("Message received ID 0x%02x: ", can_msg.identifier);
  for (int i = 0; i < can_msg.data_length_code; i++) {
    Serial.printf("%02X ", can_msg.data[i]);
  }
  Serial.println();
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
          can_msg.identifier = can_periodic_messages[i].id;
          can_msg.flags = TWAI_MSG_FLAG_EXTD; 
          can_msg.data_length_code = 8; 
          memcpy(can_msg.data, can_periodic_messages[i].data, sizeof(can_msg.data));
          if (twai_transmit(&can_msg, pdMS_TO_TICKS(1000)) == ESP_OK) {
            delay(2);
            //.println("Message from mode 1 has been sent.");
          } 
          else {
            //Serial.println("Failed to sent message from mode 1.");
          }
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
  on_receive();
  if (setup_cfg_new.enable_cfg == 1) {
      for (int i = 0; i < setup_cfg_new.response_count_cfg; i++) {
        if (can_response_messages[i].id == responseCheck.id && memcmp(can_response_messages[i].data, responseCheck.data, 8) == 0) {
          
          can_msg.identifier = can_response_messages[i].responseId;
          can_msg.flags = TWAI_MSG_FLAG_EXTD; 
          can_msg.data_length_code = 8; 
          memcpy(can_msg.data, can_response_messages[i].responseData, sizeof(can_msg.data));
      
          if (twai_transmit(&can_msg, pdMS_TO_TICKS(1000)) == ESP_OK) {
            memset(responseCheck.data, 0, sizeof(responseCheck.data)); 
            delay(2);
            //Serial.println("Message from mode 2 has been sent.");
          } 
          else {
            //Serial.println("Failed to sent message from mode 2.");
          }
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
  setup_can();
  while (1){
    int rc;
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
    rc = xQueueReceive(can_queue, &in_msg, 100);
    if (rc == pdPASS) {
      if(in_msg.check_change){
        Serial.println("Receive new config");
        if (twai_stop() != ESP_OK) {
          Serial.println("Failed to stop CAN");
        }
        if (twai_driver_uninstall() != ESP_OK) {
          Serial.println("Failed to reset CAN setup");
        }
        setup_can_cfg();
        setup_can(); 
      }
      else if(in_msg.check_stop) {
        Serial.println("Received command to stop the program.");
        mode_evt = mode_event::STOP_MODE;
      }
    }
  }
}

