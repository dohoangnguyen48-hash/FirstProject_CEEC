#include "http_client_logic.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <string.h>
#include "cJSON.h"
#include "oled.h"
#include "keypad_logic.h"

static const char *TAG = "HTTP_CLIENT";
#define SERVER_URL "http://10.117.25.132:3000/api/check"
#define CHANGE_PIN_URL "http://10.117.25.132:3000/api/change-pin"

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
                cJSON *oled_msg = cJSON_GetObjectItem(json_response, "oled_message");
                cJSON *oled_name = cJSON_GetObjectItem(json_response, "oled_name"); // Biến mới!
                if (oled_msg && oled_msg->valuestring) {
                    
                    // --- NẾU ĐĂNG NHẬP THÀNH CÔNG CÓ TÊN ---
                    if (oled_name && oled_name->valuestring) {
                        
                        // 1. Quẹt thẻ: Giao toàn quyền cho Bàn phím xử lý đổi PIN
                        if(strcmp(auth_type, "rfid") == 0) {
                            keypad_trigger_change_pin_prompt(credential, oled_name->valuestring);
                            // Xong! Không delay, không xóa gì ở đây nữa. Bàn phím tự lo.
                        }
                        
                        // 2. Nhập PIN: Chỉ in lời chào 3 giây rồi quay về mặc định
                        else {
                            oled_clear();
                            oled_draw_string(20, 2, oled_msg->valuestring);
                            oled_draw_string(0, 5, oled_name->valuestring);
                            oled_update();
                            
                            vTaskDelay(pdMS_TO_TICKS(3000));
                            
                            oled_clear();
                            oled_draw_string(10, 3, "Quet hoac nhap PIN");
                            oled_update();
                        }
                    } 
                    
                    // --- NẾU BÁO LỖI (Sai PIN, Thẻ khóa...) ---
                    else {
                        oled_clear();
                        oled_draw_string(10, 3, oled_msg->valuestring);
                        oled_update();
                        
                        vTaskDelay(pdMS_TO_TICKS(3000));
                        
                        oled_clear();
                        oled_draw_string(10, 3, "Quet hoac nhap PIN");
                        oled_update();
                    }
                }
                cJSON_Delete(json_response);
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

void http_send_change_pin_request(const char* uid, const char* old_pin, const char* new_pin) {
    esp_http_client_config_t config = {
        .url = CHANGE_PIN_URL, // Trỏ vào API 3
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
        .event_handler = _http_event_handler
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    rcv_len = 0;
    memset(rcv_buffer, 0, sizeof(rcv_buffer));

    // Đóng gói JSON chứa cả PIN cũ và PIN mới
    char post_data[128];
    snprintf(post_data, sizeof(post_data), "{\"uid\":\"%s\",\"old_pin\":\"%s\",\"new_pin\":\"%s\"}", uid, old_pin, new_pin);
    
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    if (esp_http_client_perform(client) == ESP_OK && rcv_len > 0) {
        cJSON *json_response = cJSON_Parse(rcv_buffer);
        if (json_response != NULL) {
            cJSON *oled_msg = cJSON_GetObjectItem(json_response, "oled_message");
            if (oled_msg && oled_msg->valuestring) {
                // In kết quả đổi PIN (VD: "Doi PIN OK!" hoặc "PIN cu sai!")
                oled_clear();
                oled_draw_string(10, 3, oled_msg->valuestring);
                oled_update();
                
                vTaskDelay(pdMS_TO_TICKS(3000)); // Hiện 3s
                
                // Trả về mặc định
                oled_clear();
                oled_draw_string(10, 3, "Quet hoac nhap PIN");
                oled_update();
            }
            cJSON_Delete(json_response);
        }
    }
    esp_http_client_cleanup(client);
}