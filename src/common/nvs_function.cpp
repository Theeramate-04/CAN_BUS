#include <Arduino.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <string>

#include "common/nvs_function.h"

SemaphoreHandle_t Mutex;

void init_nvs(void) {
	Mutex = xSemaphoreCreateMutex();
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

esp_err_t NVS_Read(const char *data, const char *Get_Data){
	if (xSemaphoreTake(Mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
	    printf("\n");
	    printf("Opening Non-Volatile Storage (NVS) handle... ");
	    nvs_handle_t my_handle;
	    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
	    if (err != ESP_OK) {
	        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
			return err;
	    } 
		else {
	        printf("Done\n");

	        size_t required_size = 0;
	        nvs_get_str(my_handle, data, NULL, &required_size);
	        char *server_name = (char*)malloc(required_size);
	        err = nvs_get_str(my_handle, data, server_name, &required_size);
			
	        switch (err) {
	            case ESP_OK:
	                printf("Done\n");
	                printf("Read data: %s\n", server_name);
					strcpy((char*)Get_Data,server_name);
	                break;
	            case ESP_ERR_NVS_NOT_FOUND:
	                printf("The value is not initialized yet!\n");
	                break;
	            default :
	                printf("Error (%s) reading!\n", esp_err_to_name(err));
	        }
         }
	    nvs_close(my_handle);
		xSemaphoreGive(Mutex);
		return err;
	}
	return ESP_ERR_TIMEOUT;
}

esp_err_t NVS_Read(const char *data, int *Get_Data){  
	if (xSemaphoreTake(Mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
	    printf("\n");
	    printf("Opening Non-Volatile Storage (NVS) handle... ");
	    nvs_handle_t my_handle;
	    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
	    if (err != ESP_OK) {
	        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
			return err;
	    } 
		else {
	        printf("Done\n");

	        int32_t value = 0;
        	err = nvs_get_i32(my_handle, data, &value);
			
	        switch (err) {
	            case ESP_OK:
	                printf("Done\n");
	                printf("Read data: %d\n", value);
                	*Get_Data = value;
	                break;
	            case ESP_ERR_NVS_NOT_FOUND:
	                printf("The value is not initialized yet!\n");
	                break;
	            default :
	                printf("Error (%s) reading!\n", esp_err_to_name(err));
	        }
         }
	    nvs_close(my_handle);
		xSemaphoreGive(Mutex);
		return err;
	}
	return ESP_ERR_TIMEOUT;
}

esp_err_t NVS_Read_Struct(const char *data, void *Get_Data, size_t size) {
	if (xSemaphoreTake(Mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
		printf("\n");
		printf("Opening Non-Volatile Storage (NVS) handle... ");
		nvs_handle_t my_handle;
		esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
		if (err != ESP_OK) {
			printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
			return err;
		} else {
			printf("Done\n");

			size_t required_size = size;
			err = nvs_get_blob(my_handle, data, Get_Data, &required_size);

			switch (err) {
				case ESP_OK:
					printf("Done\n");
					printf("Read struct data.\n");
					break;
				case ESP_ERR_NVS_NOT_FOUND:
					printf("The value is not initialized yet!\n");
					break;
				default :
					printf("Error (%s) reading!\n", esp_err_to_name(err));
			}
		}
		nvs_close(my_handle);
		xSemaphoreGive(Mutex);
		return err;
	}
	return ESP_ERR_TIMEOUT;
}

void NVS_Write(const char *data,const char *write_string){
	if (xSemaphoreTake(Mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
	    printf("\n");
	    printf("Opening Non-Volatile Storage (NVS) handle... ");
	    nvs_handle_t my_handle;
	    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
	    if (err != ESP_OK) {
	        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
	    } 
		else {
			err = nvs_set_str(my_handle, data, write_string);
			if (err != ESP_OK) {
				printf("Error (%s) writing!\n", esp_err_to_name(err));
			}
			else {
				printf("Write data: %s\n", write_string);
				printf("Committing updates in NVS ... ");
				err = nvs_commit(my_handle);
				if (err != ESP_OK) {
					printf("Error (%s) commit!\n", esp_err_to_name(err));
					printf("Failed!\n");
				} else {
					printf("Done\n");
				}
			}
	    }
	    nvs_close(my_handle);
		xSemaphoreGive(Mutex);
	}
}

void NVS_Write(const char *data,int write_int){
	if (xSemaphoreTake(Mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
	    printf("\n");
	    printf("Opening Non-Volatile Storage (NVS) handle... ");
	    nvs_handle_t my_handle;
	    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
	    if (err != ESP_OK) {
	        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
	    } 
		else {
			err = nvs_set_i32(my_handle, data, write_int);
			if (err != ESP_OK) {
				printf("Error (%s) writing!\n", esp_err_to_name(err));
			}
			else {
				printf("Write data: %d\n", write_int);
				printf("Committing updates in NVS ... ");
				err = nvs_commit(my_handle);
				if (err != ESP_OK) {
					printf("Error (%s) commit!\n", esp_err_to_name(err));
					printf("Failed!\n");
				} else {
					printf("Done\n");
				}
			}
	    }
	    nvs_close(my_handle);
		xSemaphoreGive(Mutex);
	}
}

void NVS_Write_Struct(const char *key, const void *data, size_t size) {
	if (xSemaphoreTake(Mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
		printf("\n");
		printf("Opening Non-Volatile Storage (NVS) handle... ");
		nvs_handle_t my_handle;
		esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
		if (err != ESP_OK) {
			printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
		} else {
			err = nvs_set_blob(my_handle, key, data, size);
			if (err != ESP_OK) {
				printf("Error (%s) writing!\n", esp_err_to_name(err));
			} else {
				printf("Write struct data.\n");
				printf("Committing updates in NVS ... ");
				err = nvs_commit(my_handle);
				if (err != ESP_OK) {
					printf("Error (%s) commit!\n", esp_err_to_name(err));
					printf("Failed!\n");
				} else {
					printf("Done\n");
				}
			}
		}
		nvs_close(my_handle);
		xSemaphoreGive(Mutex);
	}
}
