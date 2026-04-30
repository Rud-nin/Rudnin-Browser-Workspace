#include "http_request.h"
#include "utils.h"
#include "status_code.h"
#include "api.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* gcc server.c utils.c http_request.c status_code.c api.c -o server -lssl -lcrypto */

#define PORT 1603
#define ADDRESS "127.0.0.1"

int handleAPI(HttpRequest *req);
void *handle_request(void *arg);

int main(int argc, char *argv[]) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    inet_pton(AF_INET, ADDRESS, &server_address.sin_addr);

    if (bind(
        server_socket,
        (struct sockaddr*)&server_address,
        sizeof(server_address)
    ) != 0) {
        puts("Bind address failed");
        return 1;
    }

    if (listen(server_socket, 10) != 0) {
        puts("Listen failed");
        return 1;
    }

    printf("Server is running on port %d\n", PORT);

    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        int client_socket = accept(
            server_socket, 
            (struct sockaddr*)&client_address,
            &client_len
        );

        if (client_socket == -1) continue;
        // CORS: only allow local request
        if (
            strcmp(
                inet_ntoa(client_address.sin_addr),
                "127.0.0.1"
            ) != 0
        ) continue;

        int *pclient = (int*)malloc(sizeof(int));
        *pclient = client_socket;
        pthread_t t;
        pthread_create(&t, NULL, handle_request, pclient);
        pthread_detach(t);
    }

    close(server_socket);
    return 0;
}

void *handle_request(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);

    while (1) {
        char *raw_request = read_request(client_socket);
        if (!raw_request) break;
        
        HttpRequest req;
        req.__fd = client_socket;
        int success = parse_http_request(raw_request, &req);
        free(raw_request);
        if (!success) {
            // fail to parse for any reasons
            _500(&req);
        }

        if (strcmp(req.version, "HTTP/1.1") != 0) {
            // HTTP 1.1 only server
            _505(&req);
        }

        if (strncmp(req.resource, "/api", 4) == 0) {
            handleAPI(&req);
        } else {
            // static resource - public folder
            if (strstr(req.resource, "..")) {
                _403(&req);
                continue;
            }

            if (strcmp(req.resource, "/") == 0) {
                strcpy(req.resource, "/index.html");
            }

            send_file(&req);
            free_http_request(&req);
        }

        if (strcmp(get_header("Connection", &req), "close") == 0) {
            break;
        }
    }

    close(client_socket);
}

int handleAPI(HttpRequest *req) {
    char *method = req->method;
    char *resource = req->resource;
    if (strcmp(method, "GET") == 0 && strcmp(resource, "/api/quotes") == 0) {
        random_quote(req);
    } else if (strcmp(method, "GET") == 0 && strcmp(resource, "/api/neofetch") == 0) {
        neofetch(req);
    } else if (strcmp(method, "GET") == 0 && strcmp(resource, "/api/top") == 0) {
        top(req);
    } else if (strcmp(method, "GET") == 0 && strcmp(resource, "/api/0x48616e67") == 0) {
        _0x48616e67(req);
    } else { _404(req); return 0; }
    return 1;
}