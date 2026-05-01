#include "api.h"
#include "http_request.h"
#include "status_code.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

int number_of_quotes = 12;
char quotes[][512] = {
    "It works on my machine.",
    "It's not a bug, it's a feature.",
    "If it works, don't touch it.",
    "Programming is 10%% writing code and 90%% wondering why it's not working.",
    "It worked, but why?",
    "I spent 3 hours debugging. It was a missing semicolon.",
    "Debug tool? No. Print everywhere? Hell yeah.",
    "There are no places like 127.0.0.1.",
    "sudo rm -rf /",
    "Think are fine until it wasn't.",
    "I'll fix it later (there are no later).",
    "What could be wrong about deploying at Friday night?"
};

int random_quote(HttpRequest *req) {
    char *quote = quotes[random() % number_of_quotes];
    char res[1024] = "";

    char *connection = get_header("Connection", req);
    if (!connection) connection = "keep-alive";

    int size = snprintf(
        res,
        sizeof(res),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %d\r\n"
        "Connection: %s\r\n"
        "\r\n"
        "%s",
        (int)strlen(quote),
        connection,
        quote
    );
    write(req->__fd, res, size);
    return 1;
}

int neofetch(HttpRequest *req) {
    FILE *file = popen("neofetch --stdout --off", "r");
    if (!file) {
        _500(req);
        return 0;
    }

    char *connection = get_header("Connection", req);
    if (!connection) connection = "keep-alive";

    char *output = NULL;
    size_t total = 0;

    char temp[1024];
    int n;

    while ((n = fread(temp, 1, sizeof(temp), file)) > 0) {
        output = realloc(output, total + n);
        memcpy(output + total, temp, n);
        total += n;
    }

    char buffer[4096] = "";
    int header_length = snprintf(
        buffer,
        sizeof(buffer),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %ld\r\n"
        "Connection: %s\r\n\r\n",
        total,
        connection
    );

    write(req->__fd, buffer, header_length);
    write(req->__fd, output, total);

    pclose(file);
    return 1;
}

static int read_cpu_times(unsigned long long *idle, unsigned long long *total) {
    FILE *file = fopen("/proc/stat", "r");
    if (!file) return 0;

    char label[8];
    unsigned long long user, nice, system, idle_time, iowait;
    unsigned long long irq, softirq, steal, guest, guest_nice;
    int count = fscanf(
        file,
        "%7s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
        label,
        &user,
        &nice,
        &system,
        &idle_time,
        &iowait,
        &irq,
        &softirq,
        &steal,
        &guest,
        &guest_nice
    );
    fclose(file);

    if (count < 8 || strcmp(label, "cpu") != 0) return 0;

    *idle = idle_time + iowait;
    *total = user + nice + system + idle_time + iowait + irq + softirq + steal;
    return 1;
}

static int read_mem_info(unsigned long long *total, unsigned long long *used) {
    FILE *file = fopen("/proc/meminfo", "r");
    if (!file) return 0;

    char key[64];
    unsigned long long value;
    char unit[16];
    unsigned long long mem_total = 0, mem_available = 0;

    while (fscanf(file, "%63s %llu %15s", key, &value, unit) == 3) {
        if (strcmp(key, "MemTotal:") == 0) {
            mem_total = value;
        } else if (strcmp(key, "MemAvailable:") == 0) {
            mem_available = value;
        }

        if (mem_total && mem_available) break;
    }

    fclose(file);

    if (!mem_total || !mem_available || mem_available > mem_total) return 0;

    *total = mem_total;
    *used = mem_total - mem_available;
    return 1;
}

int top(HttpRequest *req) {
    unsigned long long idle_before, total_before;
    unsigned long long idle_after, total_after;
    unsigned long long total_mem, mem_used;

    if (
        !read_cpu_times(&idle_before, &total_before) ||
        !read_mem_info(&total_mem, &mem_used)
    ) {
        _500(req);
        return 0;
    }

    usleep(100000);

    if (!read_cpu_times(&idle_after, &total_after) || total_after <= total_before) {
        _500(req);
        return 0;
    }

    unsigned long long total_delta = total_after - total_before;
    unsigned long long idle_delta = idle_after - idle_before;
    double cpu_used = 100.0 * (double)(total_delta - idle_delta) / (double)total_delta;

    char buffer[1024];
    int n = snprintf(
        buffer,
        sizeof(buffer),
        "{ \"cpu_used\": %.2f, \"total_mem\": %llu, \"mem_used\": %llu }",
        cpu_used,
        total_mem,
        mem_used
    );

    char *connection = get_header("Connection", req);
    if (!connection) connection = "keep-alive";

    char header[1024];
    int header_length = snprintf(
        header,
        sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: %s\r\n\r\n",
        n,
        connection
    );

    write(req->__fd, header, header_length);
    write(req->__fd, buffer, n);

    return 1;
}

#define MAX_FILE 32
char files[MAX_FILE][128];
int file_count = 0;

int init_0x48616e67() {
    static int init = 0;
    if (init) return 1;

    struct dirent *entry;
    DIR *dir = opendir("../private");
    if (!dir) return 0;

    while((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
        strcpy(files[file_count++], entry->d_name);
        if (file_count >= MAX_FILE) break;
    }

    closedir(dir);
    init = 1;
    return 1;
}

#undef MAX_FILE

int _0x48616e67(HttpRequest *req) {
    if (init_0x48616e67() == 0) {
        _500(req);
        return 0;
    }

    if (file_count == 0) {
        _503(req);
        return 0;
    }

    char old_resource[256];
    strcpy(old_resource, req->resource);

    snprintf(
        req->resource,
        sizeof(req->resource),
        "/../private/%s",
        files[random() % file_count]
    );

    int res = send_file(req);
    strcpy(req->resource, old_resource);
    return res;
}
