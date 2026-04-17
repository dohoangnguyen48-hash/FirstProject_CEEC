#include "oled.h"
#include "driver/i2c.h" // ĐỔI SANG THƯ VIỆN I2C CŨ
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "OLED_DRIVER";

// ==========================================
// HÀM GỬI LỆNH (Chuẩn I2C Cũ)
// ==========================================
static void oled_send_cmd(uint8_t cmd) {
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd_handle, 0x00, true); // 0x00: byte tiếp theo là Lệnh
    i2c_master_write_byte(cmd_handle, cmd, true);
    i2c_master_stop(cmd_handle);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd_handle, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd_handle);
}

// ==========================================
// HÀM KHỞI TẠO (Chuẩn I2C Cũ)
// ==========================================
esp_err_t oled_init(void) {
    // 1. Cấu hình I2C cũ
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    
    // 2. Cài đặt Driver
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    // 3. Đánh thức màn hình
    vTaskDelay(100 / portTICK_PERIOD_MS); 
    oled_send_cmd(0xAE); // Tắt màn hình
    oled_send_cmd(0x8D); // Bộ bơm điện áp
    oled_send_cmd(0x14); // Bật bơm điện áp
    oled_send_cmd(0x20); // Chế độ vẽ
    oled_send_cmd(0x00); // Chiều ngang
    oled_send_cmd(0xAF); // Bật màn hình

    ESP_LOGI(TAG, "Da danh thuc OLED (Dung I2C Legacy)!");
    return ESP_OK;
}

// ==========================================
// HÀM XÓA MÀN HÌNH (Chuẩn I2C Cũ)
// ==========================================
void oled_clear(void) {
    for (uint8_t page = 0; page < 8; page++) {
        oled_send_cmd(0xB0 + page); 
        oled_send_cmd(0x00);        
        oled_send_cmd(0x10);        
        
        i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
        i2c_master_start(cmd_handle);
        i2c_master_write_byte(cmd_handle, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd_handle, 0x40, true); // 0x40: Chuẩn bị nhận dữ liệu

        for(int i = 0; i < 128; i++) {
            i2c_master_write_byte(cmd_handle, 0x00, true); // Gửi 128 số 0
        }

        i2c_master_stop(cmd_handle);
        i2c_master_cmd_begin(I2C_MASTER_NUM, cmd_handle, 1000 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd_handle);
    }
}

// ==========================================
// HÀM BẬT SÁNG TOÀN MÀN HÌNH (Để Test)
// ==========================================
void oled_fill_all(void) {
    for (uint8_t page = 0; page < 8; page++) {
        oled_send_cmd(0xB0 + page); 
        oled_send_cmd(0x00);        
        oled_send_cmd(0x10);        
        
        i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
        i2c_master_start(cmd_handle);
        i2c_master_write_byte(cmd_handle, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd_handle, 0x40, true); // Chuẩn bị nhận dữ liệu

        // Bơm 128 số 0xFF (Sáng toàn bộ) thay vì 0x00
        for(int i = 0; i < 128; i++) {
            i2c_master_write_byte(cmd_handle, 0xFF, true); 
        }

        i2c_master_stop(cmd_handle);
        i2c_master_cmd_begin(I2C_MASTER_NUM, cmd_handle, 1000 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd_handle);
    }
}