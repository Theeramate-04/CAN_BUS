#ifndef CAN_STRUCT_H
#define CAN_STRUCT_H

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


#endif 