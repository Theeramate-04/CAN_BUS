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

static int periodicCount = 0;
static int responseCount = 0;
static int Mode = 3; 
static int enable = 3;

#endif 