#ifndef HTTP_FUNCTION_H
#define HTTP_FUNCTION_H

#include <Arduino.h>

void hexStringToBytes(String hexString, uint8_t *byteArray);
String bytesToHexString(const uint8_t* byteArray, size_t length);
void get_mode(void);
void set_mode(void);
void set_periodic_cfg(void);
void set_req_res_cfg(void);
void get_periodic_cfg(void);
void get_req_res_cfg(void);
void start_stop_program(void);
void get_program_running(void);
void get_bitrates(void);
void set_bitrates(void);
void http_entry(void *pvParameters);

#endif 
