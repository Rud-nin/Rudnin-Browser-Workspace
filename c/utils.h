#ifndef UTILS_H
#define UTILS_H

#include "http_request.h"

const char *get_content_type(const char* file_name);

int send_file(HttpRequest *req);

char *fetch(const char *url);

char *read_request(int client_socket);

#endif