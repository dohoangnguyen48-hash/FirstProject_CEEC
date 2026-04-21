#include "wifi_logic.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "WIFI_LOGIC";

// Hàm bắt sự kiện. Khi wifi thay đổi, nó sẽ tự động chạy vào hàm này.
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // Mạch vừa khởi động Wi-Fi xong -> Ra lệnh kết nối
        esp_wifi_connect();
        ESP_LOGI(TAG, "Dang thu ket noi den Wi-Fi...");
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // Nếu rớt mạng hoặc sai mật khẩu -> Tự động thử kết nối lại
        ESP_LOGW(TAG, "Mat ket noi Wi-Fi. Dang thu lai...");
        esp_wifi_connect();
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // Nhận được IP từ cục Router -> Kết nối thành công 100%
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "KET NOI THANH CONG! IP cua ESP32 la: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

// Hàm khởi tạo Wi-Fi chính
void wifi_init_sta(void) {
    // Tạo giao diện mạng mặc định
    esp_netif_create_default_wifi_sta();

    // Khởi tạo cấu hình Wi-Fi với thông số mặc định
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Đăng ký các bẫy sự kiện (Bắt lỗi rớt mạng, bắt sự kiện có IP)
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    // Nạp tên và mật khẩu Wi-Fi từ file .h
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK, // Chuẩn bảo mật phổ biến nhất
        },
    };

    //Cài đặt chế độ Trạm (Station - Đi kết nối vào cục Router)
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    
    esp_wifi_start();
}