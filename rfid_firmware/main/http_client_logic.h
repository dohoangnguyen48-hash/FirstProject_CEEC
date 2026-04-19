#ifndef HTTP_CLIENT_LOGIC_H
#define HTTP_CLIENT_LOGIC_H

void http_send_auth_request(const char* auth_type, const char* credential);
void http_send_change_pin_request(const char* uid, const char* old_pin, const char* new_pin);

#endif