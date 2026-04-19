#ifndef KEYPAD_LOGIC_H
#define KEYPAD_LOGIC_H

// Khởi tạo các chân GPIO cho bàn phím
void keypad_init(void);

// Đọc phím đang bấm (trả về '\0' nếu không bấm)
char keypad_get_key(void);
void keypad_handle_input(void);
void keypad_trigger_change_pin_prompt(const char* uid, const char* name);

#endif