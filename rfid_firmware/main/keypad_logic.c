#include "keypad_logic.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "http_client_logic.h"
#include <string.h>
#include "oled.h"

// Định nghĩa chân cắm
const int ROW_PINS[4] = {13, 12, 14, 27};
const int COL_PINS[4] = {26, 25, 33, 32};

const char KEYMAP[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
static char pin_buffer[10] = ""; //Lưu các số đã được bấm cho việc kiểm tra
static int pin_index = 0; //Biến đếm (con trỏ) chỉ xem người dùng đã bấm được bao nhiêu số rồi (đồng thời nó chỉ ra vị trí của ký tự kết thúc \0)
static uint32_t last_keypress_time = 0; // Lưu lại thời điểm cuối cùng có người chạm vào bàn phím.

typedef enum {
    MODE_NORMAL,            // Chờ mở cửa bình thường
    MODE_PROMPT_CHANGE_PIN, // Chờ 3s xem có bấm * để đổi PIN không
    MODE_INPUT_OLD_PIN,     // Đang nhập PIN cũ
    MODE_INPUT_NEW_PIN,      // Đang nhập PIN mới
    MODE_WIFI_CONNECTING    // Trạng thái wifi
} app_mode_t;

static app_mode_t current_mode = MODE_WIFI_CONNECTING;
static uint32_t mode_timer = 0; // Bộ đếm giờ cho lời chào

// Các biến lưu trữ tạm thời
static char saved_uid[20] = ""; // Lưu UID của người muốn đổi PIN
static char old_pin[5] = ""; //Tương tự pin_buffer và pin_index
static char new_pin[5] = "";
static int old_len = 0;
static int new_len = 0;

// Khởi tạo các chân GPIO
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

//Hàm đọc phím, trả về \0 nếu không nhấn
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

void reset_to_normal_mode() {
    current_mode = MODE_NORMAL;
    pin_index = 0;
    memset(pin_buffer, 0, sizeof(pin_buffer)); 
    oled_show_default_screen(); 
}

void keypad_set_wifi_connecting(void) {
    current_mode = MODE_WIFI_CONNECTING;
    oled_show_wifi_connecting(); 
}

void keypad_set_wifi_connected(void) {
    reset_to_normal_mode(); 
}

void keypad_trigger_change_pin_prompt(const char* uid, const char* name) {
    current_mode = MODE_PROMPT_CHANGE_PIN;
    strncpy(saved_uid, uid, sizeof(saved_uid)); 
    mode_timer = xTaskGetTickCount() * portTICK_PERIOD_MS;
    oled_show_change_pin_prompt(name);
}

//Hàm xử lí logic chính
void keypad_handle_input(void) {
    
    if (current_mode == MODE_WIFI_CONNECTING) {
        return; // Đang mất mạng, thoát hàm luôn, không cho bấm phím gì cả!
    }
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

    // --- XỬ LÝ TIMEOUT CHO TỪNG TRẠNG THÁI ---
    if (current_mode == MODE_PROMPT_CHANGE_PIN) {
        if (current_time - mode_timer > 3000) reset_to_normal_mode(); // Hết 3s không bấm * thì về mặc định
    } 
    else if (current_mode == MODE_INPUT_OLD_PIN || current_mode == MODE_INPUT_NEW_PIN) {
        if (current_time - last_keypress_time > 10000) reset_to_normal_mode(); // 10s ko bấm thì hủy đổi PIN
    }
    else if (current_mode == MODE_NORMAL && pin_index > 0) {
        if (current_time - last_keypress_time > 5000) reset_to_normal_mode(); // 5s ko bấm thì hủy nhập PIN mở cửa
    }

    char key = keypad_get_key();
    if (key != '\0') {
        last_keypress_time = current_time; // Reset đồng hồ ngay khi có người bấm

        // === ĐANG HIỆN LỜI CHÀO -> BẤM * ===
        if (current_mode == MODE_PROMPT_CHANGE_PIN) {
            if (key == '*') {
                current_mode = MODE_INPUT_OLD_PIN;
                old_len = 0; new_len = 0;
                memset(old_pin, 0, 5); memset(new_pin, 0, 5);
                oled_show_change_pin_step(1, old_len, new_len);
            }
        }
        // === ĐANG NHẬP PIN CŨ ===
        else if (current_mode == MODE_INPUT_OLD_PIN) {
            if (key >= '0' && key <= '9' && old_len < 4) { // Tối đa 4 số, bấm dư không nhận
                old_pin[old_len++] = key; old_pin[old_len] = '\0';
                oled_show_change_pin_step(1, old_len, new_len);
            } else if (key == '*') { // Nút xóa lùi
                if (old_len > 0) old_pin[--old_len] = '\0';
                oled_show_change_pin_step(1, old_len, new_len);
            } else if (key == '#') {
                if (old_len == 4) { // Đủ 4 số mới cho xuống dòng
                    current_mode = MODE_INPUT_NEW_PIN;
                    oled_show_change_pin_step(2, old_len, new_len);
                }
            }
        }
        // === ĐANG NHẬP PIN MỚI ===
        else if (current_mode == MODE_INPUT_NEW_PIN) {
            if (key >= '0' && key <= '9' && new_len < 4) {
                new_pin[new_len++] = key; new_pin[new_len] = '\0';
                oled_show_change_pin_step(2, old_len, new_len);
            } else if (key == '*') { // Nút xóa lùi
                if (new_len > 0) new_pin[--new_len] = '\0';
                oled_show_change_pin_step(2, old_len, new_len);
            } else if (key == '#') {
                if (new_len == 4) {
                    oled_show_processing_msg("DANG DOI PIN...");
                    // GỌI API ĐỔI PIN LÊN SERVER
                    http_send_change_pin_request(saved_uid, old_pin, new_pin);
                    
                    current_mode = MODE_NORMAL; // Gửi xong trả về trạng thái ẩn
                }
            }
        }
        // === TRẠNG THÁI MỞ CỬA BÌNH THƯỜNG ===
        else if (current_mode == MODE_NORMAL) {
            if (key >= '0' && key <= '9') {
                if (pin_index < 4) {
                    pin_buffer[pin_index++] = key; pin_buffer[pin_index] = '\0';
                    oled_show_normal_pin_input(pin_index);
                }
            } 
            else if (key == '*') {
                if (pin_index > 0) {
                    pin_buffer[--pin_index] = '\0';
                    oled_show_normal_pin_input(pin_index);
                }
            } 
            else if (key == '#') {
                if (pin_index == 4) {
                    oled_show_processing_msg("DANG XU LY...");
                    http_send_auth_request("pin", pin_buffer); // Gửi API mở cửa
                    pin_index = 0; pin_buffer[0] = '\0';
                }
            }
        }
    }
}