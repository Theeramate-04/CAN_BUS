#ifndef CAN_CFG_H
#define CAN_CFG_H

#include <Arduino.h>

struct setUp_cfg {
  int enable_cfg =3;
  int bit_cfg;
  int mode_cfg = 3;
  int periodic_count_cfg;
  int response_count_cfg;
};

struct setUp_cfg_new {
  int enable_cfg =3;
  int bit_cfg;
  int mode_cfg = 3;
  int periodic_count_cfg;
  int response_count_cfg;
};

struct http_periodic {
  uint32_t id;
  uint8_t data[8];
  uint32_t period;
  uint32_t lastSent;
};

struct http_response {
  uint32_t id;
  uint8_t data[8];
  uint32_t responseId;
  uint8_t responseData[8];
};

struct can_periodic {
  uint32_t id;
  uint8_t data[8];
  uint32_t period;
  uint32_t lastSent;
};

struct can_response {
  uint32_t id;
  uint8_t data[8];
  uint32_t responseId;
  uint8_t responseData[8];
};

struct can_response_check {
  uint32_t id;
  uint8_t data[8];
};

enum mode_event {
  PERIOD_MODE,
  REQ_RES_MODE,
  STOP_MODE
};

struct queue_msg {
  bool check_stop = false;
  bool check_change = false;
};

static int mode_s = 3;
static int enable_s = 3;
static int bit_s = 0;
static int periodic_s = 0;
static int response_s = 0;

#endif 