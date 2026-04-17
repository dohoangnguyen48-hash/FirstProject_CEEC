idf_component_register(
    SRCS "keypad_logic.c" "http_client_logic.c" "main.c" 
        "rfid_logic.c" wifi_logic.c http_client_logic.c keypad_logic.c
    INCLUDE_DIRS "."
    REQUIRES cjson esp_driver_gpio esp_wifi esp_netif nvs_flash esp_http_client 
)