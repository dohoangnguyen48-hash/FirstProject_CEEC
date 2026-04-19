// 1. Khai báo thư viện (Includes)
#include <stdio.h>
#include "freertos/FreeRTOS.h" // Bắt buộc cho hệ điều hành
#include "freertos/task.h"     // Bắt buộc để dùng hàm delay
#include "driver/gpio.h"       // Thư viện điều khiển chân vào/ra
#include "esp_log.h"           // Thư viện in ra màn hình (thay cho Serial.print)
#include "rfid_logic.h"
#include "rc522.h"
#include "nvs_flash.h"      // Bắt buộc phải có để dùng nvs_flash_init
#include "esp_netif.h"      // Bắt buộc phải có để dùng esp_netif_init
#include "wifi_logic.h"
#include "oled.h"
#include "keypad_logic.h"

void app_main(void) {
    // 1. KHỞI TẠO BỘ NHỚ NVS (Bắt buộc cho Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

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

    // 2. KHỞI TẠO LÕI MẠNG VÀ SỰ KIỆN (Chỉ khởi tạo 1 lần duy nhất ở main)
    esp_netif_init();
    esp_event_loop_create_default();

    // 3. KHỞI TẠO CÁC MODULE CỦA DỰ ÁN
    wifi_init_sta();  // Bật Wi-Fi
    rfid_init();
    keypad_init();
    
    oled_init();
    oled_clear(); // Xóa sạch rác trong RAM OLED
    oled_draw_string(10, 3, "Quet hoac nhap PIN"); // In trạng thái mặc định
    oled_update(); // Bơm ra màn hình
    
    printf("HETHONG: Khoi tao hoan tat. San sang...\n");

    // 4. Việc đọc thẻ đã có Task ngầm của thư viện lo.
    // Ở đây Giám đốc rảnh rỗi, chỉ việc ngồi chơi (hoặc làm việc khác)
    while(1) {
        keypad_handle_input();
        //printf("Main van dang chay muot ma, khong bi ket...\n");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}