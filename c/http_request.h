#ifndef HTTP_HEADER_H
#define HTTP_HEADER_H

#include <sys/types.h>
#define MAX_HEADER 20

typedef struct {
    char key[256];
    char value[256];
} str_pair;

typedef struct {
    char method[8];
    char resource[256];
    char version[16];
    char *body;
    str_pair headers[MAX_HEADER];
    size_t header_count;
    int __fd;
} HttpRequest;

int parse_http_request(char *raw, HttpRequest *req);

char *get_header(const char *key, HttpRequest *req);

void free_http_request(HttpRequest *req);

#endif