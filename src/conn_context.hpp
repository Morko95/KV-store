#pragma once
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
