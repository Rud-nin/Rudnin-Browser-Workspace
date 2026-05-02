#define _POSIX_C_SOURCE 200112L

#include "utils.h"
#include "http_request.h"
#include "status_code.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <netdb.h>
#include <sys/select.h>

/**
 * get_content_type - Return Content-Type of file name
 *
 * The function read possible file extension at any position in file name
 * string, it's doesn't have to be explicit at the end of the string. This
 * is thread safe, you mostly to serve send_file()
 * 
 * Parameters:
 *   - file_name: pointer to file name string
 * 
 * Return:
 *   - static string for correspond 
 */
const char *get_content_type(const char *file_name) {
    if (strstr(file_name, ".html")) return "text/html";
    if (strstr(file_name, ".css")) return "text/css";
    if (strstr(file_name, ".js")) return "application/javascript";
    if (strstr(file_name, ".json")) return "application/json";
    if (strstr(file_name, ".png")) return "image/png";
    if (strstr(file_name, ".jpg") || strstr(file_name, ".jpeg")) return "image/jpeg";
    if (strstr(file_name, ".gif")) return "image/gif";
    if (strstr(file_name, ".webp")) return "image/webp";
    if (strstr(file_name, ".svg")) return "image/svg+xml";
    if (strstr(file_name, ".ico")) return "image/x-icon";
    if (strstr(file_name, ".woff2")) return "font/woff2";
    return "application/octet-stream"; // fallback
}

/**
 * send_file - Send a static file response for an HTTP request.
 *
 * This function builds a file path under "../public" from req->resource. If
 * the file exists, it sends an HTTP/1.1 200 response with Content-Type and
 * Content-Length headers, then streams the file body to the request socket.
 * If the file does not exist, it sends a 404 response.
 *
 * Parameters:
 *   req - Pointer to the parsed HTTP request. req->resource is used as the
 *         requested file path and req->__fd is used as the output socket.
 *
 * Returns:
 *   1 if the file was found and sent.
 *   0 if the file was not found and a 404 response was sent.
 */
int send_file(HttpRequest *req) {
    char file_path[256] = "../public";
    strcat(file_path, req->resource);

    if (access(file_path, F_OK) == 0) {
        FILE *file = fopen(file_path, "rb");
        size_t n;
        char buf[1 << 15] = {0};

        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        rewind(file);

        int header_length = snprintf(
            buf,
            sizeof(buf),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "Connection: keep-alive\r\n"
            "\r\n",
            get_content_type(req->resource),
            size
        );

        write(req->__fd, buf, header_length);
        memset(buf, 0, sizeof(buf));

        while ((n = fread(buf, 1, sizeof(buf), file)) > 0) {
            write(req->__fd, buf, n);
        }

        fclose(file);
        return 1;
    } else {
        _404(req);
        return 0;
    }
}

typedef struct {
    char protocol[8];
    char hostname[256];
    char port[8];
    char resource[256];
    int use_ssl;
} url_t;

static int __parse_url(const char *url, url_t *out) {
    memset(out, 0, sizeof(*out));
    strcpy(out->resource, "/");

    char *p = strstr(url, "://");
    if (!p) return -1;

    strncpy(out->protocol, url, p - url);

    if (strcmp(out->protocol, "http") == 0) {
        strcpy(out->port, "80");
        out->use_ssl = 0;
    } else if (strcmp(out->protocol, "https") == 0) {
        strcpy(out->port, "443");
        out->use_ssl = 1;
    } else return -1;

    char *host = p + 3;
    char *end = host;

    while (*end && *end != ':' && *end != '/')
        end++;

    strncpy(out->hostname, host, end - host);

    if (*end == ':') {
        char *port_end = end + 1;
        while (*port_end && *port_end != '/')
            port_end++;

        strncpy(out->port, end + 1, port_end - (end + 1));
        end = port_end;
    }

    if (*end == '/')
        strcpy(out->resource, end);

    return 0;
}

static int __connect_tcp(const char *hostname, const char *port) {
    struct addrinfo hints = {0}, *res;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, port, &hints, &res) != 0)
        return -1;

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) return -1;

    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
        close(sock);
        return -1;
    }

    freeaddrinfo(res);
    return sock;
}

static SSL *__setup_tls(int sock, SSL_CTX **out_ctx) {
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) return NULL;

    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);

    if (SSL_connect(ssl) != 1) {
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        return NULL;
    }

    *out_ctx = ctx;
    return ssl;
}

static char *__read_all(int sock, SSL *ssl) {
    size_t cap = 8192, len = 0;
    char *buf = malloc(cap);

    while (1) {
        if (len + 4096 > cap) {
            cap *= 2;
            buf = realloc(buf, cap);
        }

        int n = ssl
            ? SSL_read(ssl, buf + len, 4096)
            : recv(sock, buf + len, 4096, 0);

        if (n <= 0) break;

        len += n;
    }

    buf[len] = '\0';
    return buf;
}

/**
 * fetch - Send a GET request to an HTTP or HTTPS URL.
 *
 * This function parses the given URL, opens a TCP connection to the target
 * host, optionally negotiates TLS for HTTPS, sends a basic HTTP/1.1 GET
 * request, and reads the full raw server response until the connection closes.
 *
 * Parameters:
 *   url - An absolute URL beginning with "http://" or "https://".
 *
 * Returns:
 *   On success: A pointer to a heap-allocated, null-terminated string
 *               containing the full raw HTTP response (headers + body).
 *               The caller is responsible for calling free() on it.
 *
 *   On failure: NULL (e.g., invalid URL, connection failure, or TLS setup
 *               failure).
 */
char *fetch(const char *url) {
    url_t u;
    if (__parse_url(url, &u) != 0)
        return NULL;

    int sock = __connect_tcp(u.hostname, u.port);
    if (sock < 0) return NULL;

    SSL *ssl = NULL;
    SSL_CTX *ctx = NULL;

    if (u.use_ssl) {
        ssl = __setup_tls(sock, &ctx);
        if (!ssl) {
            close(sock);
            return NULL;
        }
    }

    char request[1024];
    int n = snprintf(
        request, sizeof(request),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n\r\n",
        u.resource, u.hostname
    );

    if (ssl)
        SSL_write(ssl, request, n);
    else
        send(sock, request, n, 0);

    char *response = __read_all(sock, ssl);

    if (ssl) {
        SSL_free(ssl);
        SSL_CTX_free(ctx);
    }

    close(sock);
    return response;
}

/**
 * read_request - Read a full raw HTTP request from a client socket.
 *
 * This function reads data from the given client socket until a complete
 * HTTP request is received. It first reads until the end of the HTTP headers
 * ("\r\n\r\n"), then checks for a "Content-Length" header. If present,
 * it continues reading until the full message body is received.
 *
 * The function dynamically grows its internal buffer using realloc()
 * as more data arrives.
 *
 * Parameters:
 *   client_socket - A connected socket file descriptor to read from.
 *
 * Returns:
 *   On success: A pointer to a heap-allocated, null-terminated string
 *               containing the full raw HTTP request (headers + body).
 *               The caller is responsible for calling free() on it.
 *
 *   On failure: NULL (e.g., memory allocation failure, read error,
 *               or client disconnect before full request is received).
 *
 * Notes / Limitations:
 *   - Only supports requests with "Content-Length".
 *   - Does NOT support "Transfer-Encoding: chunked".
 *   - Assumes blocking socket behavior.
 *   - Performs simple header parsing (case-sensitive, exact match).
 *   - May block indefinitely if the client declares a larger
 *     Content-Length than it actually sends.
 *
 * Safety considerations:
 *   - The caller should ensure reasonable limits to prevent excessive
 *     memory usage (e.g., very large Content-Length values).
 *   - Consider adding timeouts or using non-blocking I/O in production.
 */
char *read_request(int client_socket) {
    const static int TIMEOUT = 3000; // ms

    size_t cap = 8192, len = 0;
    char *raw_request = malloc(cap);
    if (!raw_request) return NULL;

    char *end_of_headers = NULL;
    char *content_length = NULL;

    fd_set fds;
    struct timeval timeout;

    while (!end_of_headers) {
        if (len + 4096 >= cap) {
            cap *= 2;
            char *temp = realloc(raw_request, cap);
            if (!temp) {
                // realloc failed
                free(raw_request);
                return NULL;
            }
            raw_request = temp;
        }

        FD_ZERO(&fds);
        FD_SET(client_socket, &fds);
        timeout.tv_sec = TIMEOUT / 1000;
        timeout.tv_usec = TIMEOUT % 1000;

        int s = select(client_socket + 1, &fds, NULL, NULL, &timeout);

        if (s == -1) {
            // error
            free(raw_request);
            return NULL;
        } else if (s == 0) {
            // timeout without any message being read
            raw_request[len] = '\0';
            return raw_request;
        } else {
            ssize_t n = read(client_socket, raw_request + len, 4096);

            if (n <= 0) {
                // error or client disconnect
                free(raw_request);
                return NULL;
            }
            len += n;
            raw_request[len] = '\0';

            // TODO: decline chunk transfer or transfer-encoding

            end_of_headers = strstr(raw_request, "\r\n\r\n");
        }
    }

    // check if body exist
    content_length = strstr(raw_request, "\r\nContent-Length: ");
    if (content_length) {
        // read content length
        content_length += 18;
        char *end_of_content_length = strstr(content_length, "\r\n");
        char length[16] = "";

        strncpy(length, content_length, end_of_content_length - content_length);
        size_t content_length_t = atol(length);

        // read body
        size_t content_length_read = (raw_request + len) - (end_of_headers + 4);
        while (content_length_read < content_length_t) {
            if (len + 4096 >= cap) {
                cap *= 2;
                char *temp = realloc(raw_request, cap);
                if (!temp) {
                    // realloc failed
                    free(raw_request);
                    return NULL;
                }
                raw_request = temp;
            }

            FD_ZERO(&fds);
            FD_SET(client_socket, &fds);
            timeout.tv_sec = TIMEOUT / 1000;
            timeout.tv_usec = TIMEOUT % 1000;

            int s = select(client_socket + 1, &fds, NULL, NULL, &timeout);

            if (s == -1) {
                // error
                free(raw_request);
                return NULL;
            } else if (s == 0) {
                // timeout
                raw_request[len] = '\0';
                return raw_request;
            } else  {
                int n = read(client_socket, raw_request + len, 4096);

                if (n <= 0) {
                    // error or client disconnect
                    free(raw_request);
                    return NULL;
                }

                len += n;
                content_length_read += n;
            }
        }
    }

    raw_request[len] = '\0';
    return raw_request;
}