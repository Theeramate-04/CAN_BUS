#ifndef HTTP_FUNCTION_H
#define HTTP_FUNCTION_H

void getMode(void);
void setMode(void);
void hexStringToBytes(String hexString, uint8_t *byteArray);
void set_periodic_cfg(void);
void set_req_res_cfg(void);
void get_periodic_cfg(void);
void get_req_res_cfg(void);
void start_stop_program(void);
void get_program_running(void);

#endif 