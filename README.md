# CAN Dummy Program

CAN Dummy Program เป็นโปรเจคที่ใช้สำหรับเก็บข้อมูลและจำลองการทำงานของระบบ Controller Area Network (CAN) โดยการส่งข้อมูลการตั้งค่าต่างๆ จากเซิฟเวอร์ผ่านทาง HTTP Protocol เพื่อตั้งค่าการทำงานของระบบ อีกทั้งยังสามารถรับค่าที่ได้ตั้งค่าไว้ในระบบส่งมายังเซิฟเวอร์ได้ โดยในโปรเจคนี้ได้มีการจำลองการทำงาน 2 โหมดได้แก่

**โหมดที่ 1 Periodic Mode**
- สามารถกำหนดชุดข้อมูลที่จะให้ส่งตามช่วงเวลาที่กำหนดได้สูงสุด 30 ค่าโดยแต่ละ periodic data สามารถกำหนดช่วงเวลาแยกกันได้ตามต้องการ 	
			
**โหมดที่ 2 Request-Response Mode**
- สามารถกำหนดชุดข้อมูลที่เมื่อได้รับข้อมูลตามที่กำหนด ให้ตอบกลับไปด้วยชุดข้อมูลที่กำหนดไว้ได้ สูงสุด 30 รูปแบบการตอบสนอง									
  				
ซึ่งในการทำงานสามารถทำงานได้แค่ 1 โหมดต่อช่วงเวลา และทางเซิฟเวอร์สามารถตั้งค่าระบบได้ดังนี้
- กำหนดการเริ่มต้นหรือหยุดการทำงานของระบบ และรับข้อมูลว่าระบบนั้นทำงานอยู่หรือไม่
- กำหนดโหมดการทำงาน และรับข้อมูลโหมดการทำงานของระบบ						
- กำหนดการตั้งค่าในโหมด 1 และรับข้อมูลการตั้งค่านั้น
- กำหนดการตั้งค่าในโหมด 2 และรับข้อมูลการตั้งค่านั้น
- กำหนด Bitrate ในการส่งข้อมูลใน CAN Protocol และรับข้อมูลความเร็วในการส่งนั้น

##  แผนภาพการทำงาน
**การทำงานโดยรวมของระบบ**
![esp32_can_dummy_device-SEQ drawio (2)](https://github.com/Theeramate-04/CAN_BUS/assets/52948227/63de682a-1775-4504-b0f2-6d50f04461a1)

**การทำงานในส่วน CAN Protocol**
![esp32_can_dummy_device-TSK_CAN_STATE drawio](https://github.com/Theeramate-04/CAN_BUS/assets/52948227/ce3d9547-f4f1-4fb5-b064-617b58e67c23)

## อุปกรณ์และไลบรารีที่ต้องติดตั้ง

### อุปกรณ์ที่ต้องใช้
- ESP32 MCU 
- TJA1050 CAN transceiver module
- Jumper Wire

### ไลบรารีต้องติดตั้ง
- WiFi.h
- WebServer.h
- ArduinoJson.h
- nvs_flash.h
- nvs.h
- CAN.h

## วิธีการใช้งาน
CAN Dummy Program มีลำดับการทำงานดังนี้

1. **เซิฟเวอร์เชื่อมต่อกับ ESP32 ซึ่งเป็น Access Point**
    - สามารถกำหนด ssid และ password ของ Access Point ซึ่งจะอยู่ใน `include/cfg/host.h` ซึ่งจะมีตัวอย่างดังนี้
      ```bash
      ...
      #define ssid  "ESP32-CAN-AP"
      #define password  "12345678"
      ...
      ```
2. **ติดตั้ง PIN สำหรับการใช้งาน CAN Protocol**
    - สามารถกำหนด Pin Tx และ Pin Rx สำหรับเชื่อต่อ ESP32 เข้ากับ TJA1050 CAN transceiver module ซึ่งจะอยู่ใน `src/common/can_function.cpp` ซึ่งจะมีตัวอย่างดังนี้
      ```bash
      ...
      #define TX_GPIO_NUM   27 //Pin Tx อยู่ที่ Pin 27
      #define RX_GPIO_NUM   26 //Pin Rx อยู่ที่ Pin 26
      ...
      ```

3. **กำหนดการเริ่มต้นหรือหยุดการทำงานของโปรแกรม**
    - เซิฟเวอร์เมื่อเชื่อมต่อกับ ESP32 สามารถส่งข้อมูลการสั่งการเริ่มต้นหรือหยุดการทำงานของระบบด้วย HTTP Post และรับข้อมูลสถานะการทำงานของระบบด้วย HTTP Get ผ่าน api ดังนี้:
      ```bash
      http://<ESP32_HOST_IP>/enable
      ```
    - โดยข้อมูลที่ส่งและรับจะอยู่ในรูปแบบ Json ดังนี้
      ```bash
      {"enable":$is_enable}
      ```
      โดยที่ `$is_enable` ได้แก่ 
      - 0 แสดงถึงระบบหยุดทำงาน 
      - 1 แสดงถึงระบบทำงาน

4. **การกำหนดโหมดการทำงาน**
    - เซิฟเวอร์เมื่อเชื่อมต่อกับ ESP32 สามารถส่งข้อมูลการสั่งการทำงานของระบบต่างๆ ของระบบด้วย HTTP Post และรับข้อมูลสถานะโหมดการทำงานของระบบด้วย HTTP Get ผ่าน api ดังนี้:
      ```bash
      http://<ESP32_HOST_IP>/mode
      ```
    - โดยข้อมูลที่ส่งและรับจะอยู่ในรูปแบบ Json ดังนี้
      ```bash
      {"mode":$mode_num}
      ```
      โดยที่ `$mode_num` ได้แก่ 
      - 0 แสดงถึง Periodic Mode
      - 1 แสดงถึง Request-Response Mode

5. **การกำหนดการตั้งค่าใน Periodic Mode**
    - เซิฟเวอร์เมื่อเชื่อมต่อกับ ESP32 และมีการทำงานใน Periodic Mode จะสามารถกำหนดชุดข้อมูล และช่วงเวลาที่ต้องการส่งผ่าน CAN Protocol ให้แก่ระบบด้วย HTTP Post และรับข้อมูลที่ได้กำหนดไว้ข้างต้นในระบบด้วย HTTP Get ผ่าน api ดังนี้:
      ```bash
      http://<ESP32_HOST_IP>/period_cfg
      ```
    - โดยข้อมูลที่ส่งและรับจะอยู่ในรูปแบบ Json โดยมีตัวอย่างดังนี้
      ```bash
        "messages": 
        [
            {
            "id": "0x08F000A0",
            "data": "0x0000000000000011",
            "period": 1000
            },
            {
            "id": "0x18FFA1F3",
            "data": "0x0000000000001234",
            "period": 500
            },
            {
            "id": "0x18FFA3F3",
            "data": "0xFF00000000001234",
            "period": 100
            }
        ]
      ```
6. **การกำหนดการตั้งค่าใน Request-Response Mode**
    - เซิฟเวอร์เมื่อเชื่อมต่อกับ ESP32 และมีการทำงานใน Request-Response Mode จะ สามารถกำหนดชุดข้อมูลที่เมื่อได้รับข้อมูลตามที่กำหนด ให้ตอบกลับไปด้วยชุดข้อมูลที่กำหนดไว้ผ่าน CAN Protocol ให้แก่ระบบด้วย HTTP Post และรับข้อมูลที่ได้กำหนดไว้ข้างต้นในระบบด้วย HTTP Get ผ่าน api ดังนี้:
      ```bash
      http://<ESP32_HOST_IP>/req_res_cfg
      ```
    - โดยข้อมูลที่ส่งและรับจะอยู่ในรูปแบบ Json โดยมีตัวอย่างดังนี้
      ```bash
        "messages": 
        [
            {
            "id": "0x0C20A0A6",
            "data": "0x1234000000000000",
            "responseId": "0x0C20A6A0",
            "responseData": "0000000000004321"
            }
        ]
      ```
      
7. **กำหนด Bitrate ในการส่งข้อมูลใน CAN Protocol**
    - เซิฟเวอร์เมื่อเชื่อมต่อกับ ESP32 สามารถกำหนด Bitrate ในการส่งข้อมูลใน CAN Protocol ให้แก่ระบบด้วย HTTP Post และรับข้อมูลที่ได้กำหนดไว้ข้างต้นในระบบด้วย HTTP Get ผ่าน api ดังนี้:
      ```bash
      http://<ESP32_HOST_IP>/bitrate_cfg
      ```
    - โดยข้อมูลที่ส่งและรับจะอยู่ในรูปแบบ Json โดยมีตัวอย่างดังนี้
      ```bash
        {"bitrate":$is_bitrate}
      ```
      โดยที่ `$is_bitrate` มีค่าดังนี้
      - หากต้องการส่งที่ 1000 Kb/s `$is_bitrate` จะมีค่า 1000000
      - หากต้องการส่ง 500 Kb/s `$is_bitrate` จะมีค่า 500000
      - หากต้องการส่ง 250 Kb/s `$is_bitrate` จะมีค่า 250000
      - หากต้องการส่ง 200 Kb/s `$is_bitrate` จะมีค่า 200000
      - หากต้องการส่ง 125 Kb/s `$is_bitrate` จะมีค่า 125000
      - หากต้องการส่ง 100 Kb/s `$is_bitrate` จะมีค่า 100000
      - หากต้องการส่ง 80 Kb/s `$is_bitrate` จะมีค่า 80000
      - หากต้องการส่ง 50 Kb/s `$is_bitrate` จะมีค่า 50000
      - หากต้องการส่ง 40 Kb/s `$is_bitrate` จะมีค่า 40000
      - หากต้องการส่ง 20 Kb/s `$is_bitrate` จะมีค่า 20000
      - หากต้องการส่ง 10 Kb/s `$is_bitrate` จะมีค่า 10000
---

