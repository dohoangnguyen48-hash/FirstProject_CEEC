// 1. Khai báo thư viện (Includes)
#include <stdio.h>
#include "freertos/FreeRTOS.h" // Bắt buộc cho hệ điều hành
#include "freertos/task.h"     // Bắt buộc để dùng hàm delay
#include "driver/gpio.h"       // Thư viện điều khiển chân vào/ra
#include "esp_log.h"           // Thư viện in ra màn hình (thay cho Serial.print)
#include "rfid_logic.h"
#include "rc522.h"
#include "nvs_flash.h"      // Để dùng nvs_flash_init
#include "esp_netif.h"      // Để dùng esp_netif_init
#include "wifi_logic.h"
#include "oled.h"
#include "keypad_logic.h"

void app_main(void) {
    // KHỞI TẠO BỘ NHỚ NVS (Bắt buộc cho Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // KHỞI TẠO LÕI MẠNG VÀ SỰ KIỆN
    esp_netif_init();
    esp_event_loop_create_default();

    // KHỞI TẠO CÁC MODULE CỦA DỰ ÁN
    wifi_init_sta(); 
    rfid_init();
    keypad_init();
    
    oled_init();
    oled_clear(); 
    oled_draw_string(10, 3, "Quet hoac nhap PIN"); 
    oled_update(); 
    
    printf("HE THONG: Khoi tao hoan tat. San sang...\n");

    while(1) {
        keypad_handle_input();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}