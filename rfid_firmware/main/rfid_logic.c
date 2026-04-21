#include <stdio.h>      // Thư viện C gốc (để dùng hàm sprintf nối chuỗi)
#include "esp_log.h"    // Thư viện in ra màn hình Terminal
#include "esp_event.h" 
#include "rc522.h"      // Thư viện RFID 
#include "rfid_logic.h" 
#include "cJSON.h"
#include "http_client_logic.h"

static const char* TAG = "RFID_LOGIC"; 

// 1. Khai báo các chân cắm dây
#define RFID_MISO_PIN 19
#define RFID_MOSI_PIN 23
#define RFID_CLK_PIN  18
#define RFID_CS_PIN   5 //SDA
// RFID_RESET_PIN & RFID_VCC_PIN 3.3v
// RFID_GND_PIN GND

// Hàm bắt sự kiện. Khi có thẻ quẹt, nó sẽ tự động chạy vào hàm này.
static void rfid_event_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data) {
    
    // Nó có một sự kiện tên là "RC522_EVENT_TAG_SCANNED" (Đã quét thẻ)
    if (event_id == RC522_EVENT_TAG_SCANNED) {
        
        // --- THÊM LOGIC COOLDOWN ---
        static uint32_t last_scan_time = 0; // Biến nhớ thời gian lần quẹt trước đó
        uint32_t current_time = esp_log_timestamp(); // Lấy thời gian hiện tại 

        if (current_time - last_scan_time < 2000) {
            return; 
        }
        last_scan_time = current_time; // Cập nhật lại mốc thời gian cho lần quẹt này
        // ------------------------------------------------
        
        // Lấy dữ liệu thẻ từ cục event_data
        rc522_event_data_t* data = (rc522_event_data_t*) event_data;
        rc522_tag_t* tag = (rc522_tag_t*) data->ptr;

        ESP_LOGI(TAG, "PHAT HIEN THE!");
        ESP_LOGI(TAG, "Ma UID cua the la: %llX", tag->serial_number);
        
        // Kỹ thuật thao tác mảng Byte (chuyển số thành chữ)
        char uid_str[20];  // Tạo một cái mảng 20 ký tự để chứa chuỗi
        snprintf(uid_str, sizeof(uid_str), "%llX", tag->serial_number); // Hàm in số vào chuỗi

        cJSON *root = cJSON_CreateObject(); // Tạo ngoặc nhọn {}
        cJSON_AddStringToObject(root, "action", "verify_access"); 
        cJSON_AddStringToObject(root, "auth_type", "rfid");
        cJSON_AddStringToObject(root, "credential", uid_str);

        // Dùng cJSON_PrintUnformatted để nó in gọn trên 1 dòng
        char *json_string = cJSON_PrintUnformatted(root); 
        ESP_LOGI(TAG, "Da dong goi JSON: %s", json_string);

        http_send_auth_request("rfid", uid_str);
        
        cJSON_free(json_string); 
        cJSON_Delete(root);      
    }
}

// Hàm khởi tạo
void rfid_init() {
    ESP_LOGI(TAG, "Dang khoi tao RFID...");

    // Cấu hình chân
    rc522_config_t config = {
        .spi.host = SPI3_HOST,     // Chọn bộ điều khiển SPI số 3 của ESP32
        .spi.miso_gpio = RFID_MISO_PIN,
        .spi.mosi_gpio = RFID_MOSI_PIN,
        .spi.sck_gpio = RFID_CLK_PIN,
        .spi.sda_gpio = RFID_CS_PIN,
    };
    
    // Khởi tạo một "con trỏ" để quản lý module
    rc522_handle_t scanner;
    rc522_create(&config, &scanner); // Nạp biểu mẫu vào hệ thống
    
    // Đăng ký sự kiện
    rc522_register_events(scanner, RC522_EVENT_ANY, rfid_event_handler, NULL);
    
    rc522_start(scanner); // Bật điện cho module chạy
}
