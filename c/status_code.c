#include "status_code.h"
#include "http_request.h"
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

void send_message(int status_code, HttpRequest *req) {
    char *res = get_message(status_code, req);
    write(req->__fd, res, strlen(res));
    free(res);
}

void _204(HttpRequest *req) {
    send_message(204, req);
}

void _400(HttpRequest *req) {
    send_message(400, req);
}

void _401(HttpRequest *req) {
    send_message(401, req);
}

void _403(HttpRequest *req) {
    send_message(403, req);
}

void _404(HttpRequest *req) {
    send_message(404, req);
}

void _500(HttpRequest *req) {
    send_message(500, req);
}

void _501(HttpRequest *req) {
    send_message(501, req);
}

void _503(HttpRequest *req) {
    send_message(503, req);
}

void _505(HttpRequest *req) {
    send_message(505, req);
}