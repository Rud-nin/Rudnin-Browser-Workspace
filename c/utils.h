#ifndef UTILS_H
#define UTILS_H

#include "http_request.h"

const char *get_content_type(const char* file_name);

void url_decode(char *src, char *dst);

int send_file(HttpRequest *req);

char *fetch(const char *url);

char *read_request(int client_socket);

double read_num(char *s);

#endif