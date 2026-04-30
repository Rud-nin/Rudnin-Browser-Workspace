#include "status_code.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char *get_reason(int status_code) {
    switch (status_code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 503: return "Service Unavailable";
        case 505: return "Version Not Supported";
        default: return "Unknown";
    }
}

char *get_message(int status_code, HttpRequest *req) {
    char *message = malloc(MESSAGE_SIZE);
    char *connection = get_header("Connection", req);
    if (!connection) connection = "keep-alive";
    int size = snprintf(
        message,
        MESSAGE_SIZE,
        "HTTP/1.1 %d %s\r\n"
        "Content-Length: 0\r\n"
        "Connection: %s\r\n"
        "\r\n",
        status_code,
        get_reason(status_code),
        connection
    );
    return message;
}

void _204(HttpRequest *req) {
    char *res = get_message(204, req);
    write(req->__fd, res, strlen(res));
    free(res);
}

void _400(HttpRequest *req) {
    char *res = get_message(400, req);
    write(req->__fd, res, strlen(res));
    free(res);
}

void _401(HttpRequest *req) {
    char *res = get_message(401, req);
    write(req->__fd, res, strlen(res));
    free(res);
}

void _403(HttpRequest *req) {
    char *res = get_message(403, req);
    write(req->__fd, res, strlen(res));
    free(res);
}

void _404(HttpRequest *req) {
    char *res = get_message(404, req);
    write(req->__fd, res, strlen(res));
    free(res);
}

void _500(HttpRequest *req) {
    char *res = get_message(500, req);
    write(req->__fd, res, strlen(res));
    free(res);
}

void _501(HttpRequest *req) {
    char *res = get_message(501, req);
    write(req->__fd, res, strlen(res));
}

void _503(HttpRequest *req) {
    char *res = get_message(503, req);
    write(req->__fd, res, strlen(res));
    free(res);
}

void _505(HttpRequest *req) {
    char *res = get_message(505, req);
    write(req->__fd, res, strlen(res));
    free(res);
}