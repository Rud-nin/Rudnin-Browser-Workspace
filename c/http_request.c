#include "http_request.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int parse_http_request(char *raw, HttpRequest *req) {
    if (!raw || !req) return 0;
    char *cur = raw, *next = strstr(raw, "\r\n");
    if (!next) return 0;

    req->header_count = 0;
    req->body = NULL;

    char request_line[512];
    strncpy(request_line, cur, next - cur);
    request_line[next - cur] = '\0';
    if (
        sscanf(
            request_line, "%7s %255s %15s",
            req->method,
            req->resource,
            req->version
        ) != 3
    ) return 0;

    url_decode(req->resource, req->resource);

    char *body_start = strstr(raw, "\r\n\r\n");
    if (!body_start) return 0;

    while (next < body_start) {
        cur = next + 2; // skip \r\n
        next = strstr(cur, "\r\n");
        strncpy(request_line, cur, next - cur);
        request_line[next - cur] = '\0';

        char *colon = strchr(request_line, ':');
        if (!colon) continue;
        *colon = '\0';

        strcpy(req->headers[req->header_count].key, request_line);
        strcpy(req->headers[req->header_count].value, colon + 2);
        req->header_count++;

        if (req->header_count >= MAX_HEADER) break;
    }

    if (*body_start == '\0') req->body = NULL;
    else {
        body_start += 4;
        req->body = malloc(strlen(body_start) + 1);
        strcpy(req->body, body_start);
    }

    return 1;
}

char *get_header(const char *key, HttpRequest *req) {
    for (int i = 0; i < req->header_count; i++) {
        if (strcmp(req->headers[i].key, key) == 0) {
            return req->headers[i].key;
        }
    }
    return NULL;
}

void free_http_request(HttpRequest *req) {
    if (req && req->body) {
        free(req->body);
        req->body = NULL;
    }
}