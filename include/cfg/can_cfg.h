#ifndef CAN_CFG_H
#define CAN_CFG_H

#include <Arduino.h>

struct setUp_cfg {
  int enable_cfg;
  double bit_cfg;
  int mode_cfg = 3;
  int periodic_cfg;
  int response_cfg;
};

struct CanMessage {
  uint32_t id;
  uint8_t data[8];
  uint32_t period;
  uint32_t lastSent;
};

struct CanResponse {
  uint32_t id;
  uint8_t data[8];
  uint32_t responseId;
  uint8_t responseData[8];
};

struct CanResponseCheck {
  uint32_t id;
  uint8_t data[8];
};

enum ModeEvent {
  PERIOD_MODE,
  REQ_RES_MODE
};

static int mode_s = 3;
static int enable_s = 3;
static int bit_s = 0;
static int periodic_s = 0;
static int response_s = 0;

#endif 