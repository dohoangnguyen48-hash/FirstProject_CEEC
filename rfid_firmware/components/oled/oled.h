#ifndef OLED_H
#define OLED_H

#include "esp_err.h"

// 1. Định nghĩa chân cắm I2C (ESP32 thường dùng 21 và 22)
#define I2C_MASTER_SCL_IO           22    
#define I2C_MASTER_SDA_IO           21    
#define I2C_MASTER_NUM              0     // Dùng luồng I2C số 0
#define OLED_I2C_ADDRESS            0x3C  // Địa chỉ phần cứng mặc định của chip SSD1306

// 2. Khai báo các hàm để main.c gọi ra dùng
esp_err_t oled_init(void);
void oled_clear(void);
void oled_fill_all(void);
void oled_update(void);
void oled_draw_string(uint8_t x, uint8_t page, const char *str);

#endif // OLED_H