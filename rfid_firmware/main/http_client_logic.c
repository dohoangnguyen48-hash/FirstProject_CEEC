#include "http_client_logic.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <string.h>
#include "cJSON.h"
#include "oled.h"

static const char *TAG = "HTTP_CLIENT";
#define SERVER_URL "http://10.117.25.132:3000/api/check"

#define MAX_HTTP_RECV_BUFFER 512
static char rcv_buffer[MAX_HTTP_RECV_BUFFER];
static int rcv_len = 0;

// Cái "Tai nghe" - Chuyên hứng dữ liệu mỗi khi Server rớt hàng xuống
esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            // Chép dữ liệu nhận được vào bộ đệm
            if (rcv_len + evt->data_len < MAX_HTTP_RECV_BUFFER) {
                memcpy(rcv_buffer + rcv_len, evt->data, evt->data_len);
                rcv_len += evt->data_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            // Chốt đuôi chuỗi khi đã nhận xong toàn bộ
            rcv_buffer[rcv_len] = '\0'; 
            break;
        default:
            break;
    }
    return ESP_OK;
}

void http_send_auth_request(const char* auth_type, const char* credential) {
    esp_http_client_config_t config = {
        .url = SERVER_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
        .event_handler = _http_event_handler
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // 1. Cấu hình Header là JSON
    esp_http_client_set_header(client, "Content-Type", "application/json");

    rcv_len = 0;
    memset(rcv_buffer, 0, sizeof(rcv_buffer));

    // 2. Đóng gói JSON
    char post_data[128];
    // sprintf sẽ thay %s bằng giá trị thật
    sprintf(post_data, "{\"auth_type\":\"%s\",\"credential\":\"%s\"}", auth_type, credential);
    
    ESP_LOGI(TAG, "Dang gui data: %s", post_data);

    // 3. Đính kèm cục JSON vào Body và bắn đi
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        // Nếu gửi thành công, tiến hành kiểm tra hộp quà
        if (rcv_len > 0) {
            ESP_LOGI(TAG, "<<< SERVER TRA VE JSON: %s", rcv_buffer);
            
            // Dùng cJSON để mổ bụng gói hàng
            cJSON *json_response = cJSON_Parse(rcv_buffer);
            if (json_response != NULL) {
                
                // Móc lấy món đồ có tên là "oled_message"
                cJSON *oled_msg_item = cJSON_GetObjectItem(json_response, "oled_message");
                
                // Nếu tìm thấy chữ bên trong, in ra thật to!
                if (oled_msg_item && oled_msg_item->valuestring) {
                // In ra Terminal để check
                ESP_LOGI(TAG, "OLED: %s", oled_msg_item->valuestring);
    
                // ĐẨY LÊN MÀN HÌNH THẬT
                oled_clear();
                oled_draw_string(0, 3, oled_msg_item->valuestring); 
                oled_update();
                }
                
                cJSON_Delete(json_response); // Đọc xong nhớ ném vỏ hộp đi để tránh tràn RAM
            } 
            else {
                ESP_LOGE(TAG, "Khong the doc duoc JSON tu Server!");
            }
        }
    } 
    else {
        ESP_LOGE(TAG, "Loi ket noi Server: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}