#ifndef KEYPAD_LOGIC_H
#define KEYPAD_LOGIC_H

void keypad_init(void);
char keypad_get_key(void);
void keypad_handle_input(void);
void keypad_set_wifi_connecting(void);
void keypad_set_wifi_connected(void);
void keypad_trigger_change_pin_prompt(const char* uid, const char* name);

#endif