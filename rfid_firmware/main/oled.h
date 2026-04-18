#ifndef OLED_H
#define OLED_H
#include <stdint.h>

void oled_init(void);
void oled_clear(void);
void oled_draw_string(uint8_t x, uint8_t page, const char *str);
void oled_update(void);

#endif