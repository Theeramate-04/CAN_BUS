#ifndef CAN_CFG_H
#define CAN_CFG_H

#include <Arduino.h>

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

static int Mode_S = 3;
static int Enable_S = 3;
static int periodic_S = 0;
static int response_S = 0;

#endif 