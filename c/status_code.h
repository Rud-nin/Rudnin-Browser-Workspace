#ifndef STATUS_CODE_H
#define STATUS_CODE_H

#include "http_request.h"

#define MESSAGE_SIZE 1024

const char *get_reason(int status_code);
char *get_message(int status_code, HttpRequest *req);
void send_message(int status_code, HttpRequest *req);
void _204(HttpRequest *req);
void _400(HttpRequest *req);
void _401(HttpRequest *req);
void _403(HttpRequest *req);
void _404(HttpRequest *req);
void _500(HttpRequest *req);
void _501(HttpRequest *req);
void _503(HttpRequest *req);
void _505(HttpRequest *req);

#endif