#pragma once
#include <unordered_map>
#include <string>
#include <liburing.h>
enum class OpType {
    ACCEPT,
    RECV,
    SEND
};
struct ConnContext {
    int fd;
    OpType type;
    char buffer[1024];
};
struct ThreadContext{
    struct io_uring ring;
    int thread_fd;
    std::unordered_map<std::string, std::string> thread_map;
};

void socket_init(ThreadContext *context);
void uring_init(ThreadContext *context);

ThreadContext thread_init();
