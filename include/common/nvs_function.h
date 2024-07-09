#ifndef NVS_FUNCTION_H
#define NVS_FUNCTION_H

void init_nvs(void);
esp_err_t NVS_Read(const char *data, const char *Get_Data);
esp_err_t NVS_Read(const char *data, int *Get_Data);
esp_err_t NVS_Read_Struct(const char *data, void *Get_Data, size_t size);
void NVS_Write(const char *data,const char *write_string);
void NVS_Write(const char *data,int write_int);
void NVS_Write_Struct(const char *key, const void *data, size_t size);

#endif 