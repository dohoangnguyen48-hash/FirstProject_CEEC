#include "oled.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "font.h" 
#include "keypad_logic.h"

#define OLED_I2C_ADDRESS 0x3C
#define I2C_MASTER_NUM I2C_NUM_0

static uint8_t oled_buffer[1024]; 

static void oled_send_cmd(uint8_t cmd) {
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd_handle, 0x00, true);
    i2c_master_write_byte(cmd_handle, cmd, true);
    i2c_master_stop(cmd_handle);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd_handle, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd_handle);
}

void oled_init(void) {
    // Cấu hình chân
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 21,
        .scl_io_num = 22,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

    vTaskDelay(100 / portTICK_PERIOD_MS);
    uint8_t init_cmds[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x02, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
    };
    for (int i = 0; i < sizeof(init_cmds); i++) {
        oled_send_cmd(init_cmds[i]);
    }
}

void oled_update(void) {
    for (uint8_t page = 0; page < 8; page++) {
        oled_send_cmd(0xB0 + page); 
        oled_send_cmd(0x00);        
        oled_send_cmd(0x10);        

        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, 0x40, true);
        i2c_master_write(cmd, &oled_buffer[page * 128], 128, true); 
        i2c_master_stop(cmd);
        i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
    }
}

void oled_clear(void) {
    memset(oled_buffer, 0, 1024); 
    oled_update();
}

void oled_draw_string(uint8_t x, uint8_t page, const char *str) {
    while (*str) {
        char c = *str;
        if (c < 32 || c > 126) c = 32;
        for (int i = 0; i < 5; i++) {
            if (x + i < 128 && page < 8) {
                oled_buffer[page * 128 + x + i] = font5x8[c - 32][i];
            }
        }
        x += 6;
        str++;
    }
}

// Hàm vẽ Giao diện Đổi PIN
void oled_show_change_pin_step(int step, int old_len, int new_len) {
    oled_clear();
    oled_draw_string(0, 1, "Ma PIN cu: ");
    char star_old[5] = {0};
    for(int i=0; i<old_len; i++) star_old[i] = '*';
    oled_draw_string(70, 1, star_old);

    oled_draw_string(0, 4, "Ma PIN moi: ");
    char star_new[5] = {0};
    for(int i=0; i<new_len; i++) star_new[i] = '*';
    oled_draw_string(75, 4, star_new);

    // Dùng biến "step" truyền vào từ bên ngoài thay vì dùng current_mode
    if (step == 1) {
        oled_draw_string(0, 7, "-> Nhap PIN cu [#]");
    } else {
        oled_draw_string(0, 7, "-> Nhap PIN moi [#]");
    }
    oled_update();
}

// Hàm Reset hệ thống về mặc định
void oled_show_default_screen(void) {
    oled_clear();
    oled_draw_string(10, 3, "Quet hoac nhap PIN");
    oled_update();
}

// Hàm vẽ màn hình đang kết nối mạng
void oled_show_wifi_connecting(void) {
    oled_clear();
    oled_draw_string(10, 2, "DANG KET NOI");
    oled_draw_string(30, 5, "WIFI...");
    oled_update();
}

// HTTP gọi hàm này khi quét thẻ thành công
void oled_show_change_pin_prompt(const char* name) {
    oled_clear();
    oled_draw_string(20, 1, "Xin Chao,");
    oled_draw_string(0, 3, name);
    oled_draw_string(0, 6, "Bam * de doi PIN");
    oled_update();
}


void oled_show_normal_pin_input(int pin_len){
    oled_clear(); 
    oled_draw_string(10, 3, "Ma PIN: ");
    char star_str[10] = {0};
    for(int i = 0; i < pin_len; i++) star_str[i] = '*';
    oled_draw_string(60, 3, star_str); 
    oled_update();
}

void oled_show_processing_msg(const char* msg){
    oled_clear(); 
    oled_draw_string(20, 3, msg); 
    oled_update();
}

// Hàm vẽ lời chào khi nhập mã PIN thành công
void oled_show_auth_success_pin(const char* msg, const char* name) {
    oled_clear();
    oled_draw_string(20, 2, msg);
    oled_draw_string(0, 5, name);
    oled_update();
}

// Hàm vẽ các thông báo chung (Lỗi mạng, Sai mã, Đổi PIN thành công...)
void oled_show_message(const char* msg) {
    oled_clear();
    oled_draw_string(10, 3, msg);
    oled_update();
}