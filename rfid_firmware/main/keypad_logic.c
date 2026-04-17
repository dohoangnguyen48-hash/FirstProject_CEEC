#include "keypad_logic.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "http_client_logic.h"

// Định nghĩa chân cắm
const int ROW_PINS[4] = {13, 12, 14, 27};
const int COL_PINS[4] = {26, 25, 33, 32};

const char KEYMAP[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
static char pin_buffer[10] = ""; 
static int pin_index = 0;

void keypad_init(void) {
    for (int i = 0; i < 4; i++) {
        gpio_reset_pin(ROW_PINS[i]);
        gpio_set_direction(ROW_PINS[i], GPIO_MODE_OUTPUT);
        gpio_set_level(ROW_PINS[i], 1);

        gpio_reset_pin(COL_PINS[i]);
        gpio_set_direction(COL_PINS[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(COL_PINS[i], GPIO_PULLUP_ONLY);
    }
}

char keypad_get_key(void) {
    for (int r = 0; r < 4; r++) {
        gpio_set_level(ROW_PINS[r], 0);
        for (int c = 0; c < 4; c++) {
            if (gpio_get_level(COL_PINS[c]) == 0) {
                vTaskDelay(pdMS_TO_TICKS(20)); // Debounce
                if (gpio_get_level(COL_PINS[c]) == 0) {
                    char key = KEYMAP[r][c];
                    while (gpio_get_level(COL_PINS[c]) == 0) {
                        vTaskDelay(pdMS_TO_TICKS(10)); // Chờ nhả phím
                    }
                    gpio_set_level(ROW_PINS[r], 1);
                    return key;
                }
            }
        }
        gpio_set_level(ROW_PINS[r], 1);
    }
    return '\0';
}

// HÀM XỬ LÝ LOGIC NHẬP MẬT KHẨU
void keypad_handle_input(void) {
    char key = keypad_get_key();
        
    if (key != '\0') {
        // 1. Nhập số (0-9)
        if (key >= '0' && key <= '9') {
            if (pin_index < 6) { // Mật khẩu 6 số
                pin_buffer[pin_index] = key;
                pin_index++;
                pin_buffer[pin_index] = '\0';
                printf("BANPHIM: PIN hien tai: %s\n", pin_buffer);
            } else {
                printf("BANPHIM: Da max 6 so! Bam '#' gui, '*' xoa.\n");
            }
        } 
        // 2. Nút Xóa (*)
        else if (key == '*') {
            if (pin_index > 0) {
                pin_index--;
                pin_buffer[pin_index] = '\0';
                printf("BANPHIM: Da xoa. PIN hien tai: %s\n", pin_buffer);
            }
        } 
        // 3. Nút Xác nhận (#)
        else if (key == '#') {
            if (pin_index > 0) {
                printf("BANPHIM: => XAC NHAN! DANG GUI PIN [%s] LEN SERVER...\n", pin_buffer);
                
                // GỌI HÀM HTTP Ở ĐÂY
                http_send_auth_request("pin", pin_buffer);
                
                // Gửi xong xóa bộ đệm
                pin_index = 0;
                pin_buffer[0] = '\0';
            } else {
                printf("BANPHIM: Loi - Vui long nhap PIN truoc!\n");
            }
        }
    }
}