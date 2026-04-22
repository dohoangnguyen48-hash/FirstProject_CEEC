#ifndef OLED_H
#define OLED_H
#include <stdint.h>

void oled_init(void);
void oled_update(void);
void oled_clear(void);
void oled_draw_string(uint8_t x, uint8_t page, const char *str);

void oled_show_change_pin_step(int step, int old_len, int new_len);
void oled_show_default_screen(void);
void oled_show_wifi_connecting(void);
void oled_show_change_pin_prompt(const char* name);
void oled_show_normal_pin_input(int pin_len);
void oled_show_processing_msg(const char* msg);
void oled_show_auth_success_pin(const char* msg, const char* name);
void oled_show_message(const char* msg);

#endif