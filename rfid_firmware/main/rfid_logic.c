#include <stdio.h>      // Thư viện C gốc (để dùng hàm sprintf nối chuỗi)
#include "esp_log.h"    // Thư viện in ra màn hình Terminal
#include "esp_event.h" 
#include "rc522.h"      // Thư viện RFID em vừa tải về
#include "rfid_logic.h" // File header của chính nó
#include "cJSON.h"
#include "http_client_logic.h"

static const char* TAG = "RFID_LOGIC"; // Đặt tên thẻ để lúc in log dễ nhìn
//static tức chỉ dùng được trong file hiện tại

// 1. Khai báo các chân cắm dây
#define RFID_MISO_PIN 19
#define RFID_MOSI_PIN 23
#define RFID_CLK_PIN  18
#define RFID_CS_PIN   5 //SDA
// RFID_RESET_PIN & RFID_VCC_PIN 3.3v
// RFID_GND_PIN GND

// Đây là cái "bẫy". Khi có thẻ quẹt, nó sẽ tự động chạy vào đây.
static void rfid_event_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data) {
    
    // Nó có một sự kiện tên là "RC522_EVENT_TAG_SCANNED" (Đã quét thẻ)
    if (event_id == RC522_EVENT_TAG_SCANNED) {
        
        // --- THÊM LOGIC COOLDOWN (CHỐNG QUẸT LIÊN TỤC) ---
        static uint32_t last_scan_time = 0; // Biến nhớ thời gian lần quẹt trước đó
        uint32_t current_time = esp_log_timestamp(); // Lấy thời gian hiện tại (chính là con số 46211 kia)

        // Nếu khoảng cách giữa 2 lần quẹt < 2000 mili-giây (2 giây) thì return (bỏ qua, không làm gì cả)
        if (current_time - last_scan_time < 2000) {
            return; 
        }
        last_scan_time = current_time; // Cập nhật lại mốc thời gian cho lần quẹt này
        // ------------------------------------------------
        
        // 1. Lấy dữ liệu thẻ từ cục event_data
        rc522_event_data_t* data = (rc522_event_data_t*) event_data;
        rc522_tag_t* tag = (rc522_tag_t*) data->ptr;

        // 2. Kỹ thuật thao tác mảng Byte (chuyển số thành chữ)
        // Thẻ Mifare 1K có UID dài 4 byte, ta cần in nó ra cho đẹp
        ESP_LOGI(TAG, "PHAT HIEN THE!");

        // 3. In kết quả cuối cùng ra màn hình
        ESP_LOGI(TAG, "Ma UID cua the la: %llX", tag->serial_number);
        
        char uid_str[20];  // Tạo một cái mảng 20 ký tự để chứa chuỗi
        snprintf(uid_str, sizeof(uid_str), "%llX", tag->serial_number); // Hàm in số vào chuỗi

        cJSON *root = cJSON_CreateObject();          // Tạo ngoặc nhọn {}
        cJSON_AddStringToObject(root, "action", "verify_access"); 
        cJSON_AddStringToObject(root, "auth_type", "rfid");
        cJSON_AddStringToObject(root, "credential", uid_str); // Kết quả: {"uid": "C8605D47B2"}

        // Dùng cJSON_PrintUnformatted để nó in gọn trên 1 dòng
        char *json_string = cJSON_PrintUnformatted(root); 
        ESP_LOGI(TAG, "Da dong goi JSON: %s", json_string);

        // GỌI ÔNG GIAO HÀNG MANG JSON LÊN SERVER!
        http_send_auth_request("rfid", uid_str);
        
        //Dọn rác
        cJSON_free(json_string); // Đốt bỏ chuỗi chữ vừa in
        cJSON_Delete(root);      // Phá hủy cái vỏ bọc JSON trả lại RAM
    }
}

// 2. Viết hàm khởi tạo (Hàm này sẽ được main.c gọi 1 lần duy nhất)
void rfid_init() {
    ESP_LOGI(TAG, "Dang khoi tao RFID...");

    // 3. Lấy biểu mẫu ra và điền số chân vào
    rc522_config_t config = {
        .spi.host = SPI3_HOST,     // Chọn bộ điều khiển SPI số 3 của ESP32
        .spi.miso_gpio = RFID_MISO_PIN,
        .spi.mosi_gpio = RFID_MOSI_PIN,
        .spi.sck_gpio = RFID_CLK_PIN,
        .spi.sda_gpio = RFID_CS_PIN,
    };
    
    // 4. Khởi tạo một "con trỏ" để quản lý module
    rc522_handle_t scanner;
    rc522_create(&config, &scanner); // Nạp biểu mẫu vào hệ thống
    
    // Đăng ký sự kiện (Bước 3 bên dưới)
    rc522_register_events(scanner, RC522_EVENT_ANY, rfid_event_handler, NULL);
    
    rc522_start(scanner); // Bật điện cho module chạy
}
